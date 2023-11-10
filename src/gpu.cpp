#include "gpu.hpp"
#include "window.hpp"

#include <cassert>
#include <vector>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <json.hpp>
#include <stb_image.h>
#include <stb_image_write.h>
#include <tiny_gltf.h>

static Model_t* Model = {};
static Camera_t* Camera = {};
static WGPUTextureFormat DepthTextureFormat = WGPUTextureFormat_Depth24Plus;
static float Frame = 0;

WGPUAdapter RequestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const* options)
{
	struct Data
	{
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	} userData;

	auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* pUserData)
		{
			Data& userData = *(Data*)pUserData;

			if (status == WGPURequestAdapterStatus_Success)
				userData.adapter = adapter;
			else
				std::cout << "Could not get WebGPU adapter: " << message << std::endl;

			userData.requestEnded = true;
		};

	wgpuInstanceRequestAdapter(
		instance,
		options,
		onAdapterRequestEnded,
		(void*)&userData
	);

	assert(userData.requestEnded);
	return userData.adapter;
}

WGPUDevice RequestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor)
{
	struct Data
	{
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	} userData;

	auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* pUserData)
		{
			Data& userData = *(Data*)pUserData;

			if (status == WGPURequestDeviceStatus_Success)
				userData.device = device;
			else
				std::cout << "Could not get WebGPU device: " << message << std::endl;

			userData.requestEnded = true;
		};

	wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded, (void*)&userData);

	assert(userData.requestEnded);
	return userData.device;
}

WGPUInstance CreateInstance()
{
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

	WGPUInstance instance = wgpuCreateInstance(&desc);

	if (!instance)
	{
		std::cout << "Failed to create instance" << std::endl;
		return nullptr;
	}
	
	return instance;
}

WGPUSwapChain CreateSwapChain(WGPUDevice device, CWindow* window)
{
	WGPUSwapChainDescriptor swapchainDesc = {
		.nextInChain = nullptr,
		.usage = WGPUTextureUsage_RenderAttachment,
		.format = WGPUTextureFormat_BGRA8Unorm, // note: Dawn only supports this as swapchain right now, `wgpuSurfaceGetPreferredFormat` not implemented
		.width = static_cast<unsigned int>(window->GetSize().x),
		.height = static_cast<unsigned int>(window->GetSize().y),
		.presentMode = WGPUPresentMode_Fifo
	};

	WGPUSurface surface = window->GetSurface();
	WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapchainDesc);

	return swapChain;
}

WGPUShaderModule CreateShader(WGPUDevice device)
{
	// todo: move
	const char* shaderSource = R"(
		struct UniformBuffer {
			modelMatrix: mat4x4f,
			viewProjMatrix: mat4x4f
		};

		@group(0) @binding(0) var<uniform> uConstants: UniformBuffer;
		@group(0) @binding(1) var colorTexture: texture_2d<f32>;
		@group(0) @binding(2) var colorSampler: sampler;

		const lightDirection: vec3f = normalize(vec3f(-1.0, 1.0, 0.0));

		struct VertexOutput {
			@builtin(position) position: vec4f,
			@location(0) uv: vec2f,
			@location(1) normal: vec3f
		};
		
		@vertex
		fn vs_main(@location(0) position: vec3f, @location(1) uv: vec2f, @location(2) normal: vec3f) -> VertexOutput
		{
			var out : VertexOutput;
			out.position = uConstants.viewProjMatrix * uConstants.modelMatrix * vec4f(position, 1.0);
			out.uv = uv;

		    // out.normal = normalize((uConstants.normalMatrix * vec4f(normal, 0.0)).xyz); // Transform the normal
			out.normal = normal;

			return out;
		}

		@fragment
		fn fs_main(in: VertexOutput) -> @location(0) vec4f
		{
			let N: vec3f = normalize(in.normal); // Normalized normal vector
			let L: vec3f = normalize(lightDirection); // Normalized light direction
			
			let diffuse: f32 = max(dot(N, L), 0.0);
			let ambient: f32 = 0.1f;
			
			let textureColor: vec4f = textureSample(colorTexture, colorSampler, in.uv);
			let shadedColor: vec3f = textureColor.rgb * (diffuse + ambient);
			
			return vec4f(shadedColor, 1.0);
		}
	)";

	WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {
		.chain = {
			.next = nullptr,
			.sType = WGPUSType_ShaderModuleWGSLDescriptor
		},
		.code = shaderSource
	};

	WGPUShaderModuleDescriptor shaderDesc = {
		.nextInChain = &shaderCodeDesc.chain
	};

	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);

	return shaderModule;
}

void SetDefaultBindGroupLayoutEntry(WGPUBindGroupLayoutEntry& bindingLayout)
{
	bindingLayout.buffer.nextInChain = nullptr;
	bindingLayout.buffer.type = WGPUBufferBindingType_Undefined;
	bindingLayout.buffer.hasDynamicOffset = false;

	bindingLayout.sampler.nextInChain = nullptr;
	bindingLayout.sampler.type = WGPUSamplerBindingType_Undefined;

	bindingLayout.storageTexture.nextInChain = nullptr;
	bindingLayout.storageTexture.access = WGPUStorageTextureAccess_Undefined;
	bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
	bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

	bindingLayout.texture.nextInChain = nullptr;
	bindingLayout.texture.multisampled = false;
	bindingLayout.texture.sampleType = WGPUTextureSampleType_Undefined;
	bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
}

void SetDefaultStencilFaceState(WGPUStencilFaceState& stencilFaceState)
{
	stencilFaceState.compare = WGPUCompareFunction_Always;
	stencilFaceState.failOp = WGPUStencilOperation_Keep;
	stencilFaceState.depthFailOp = WGPUStencilOperation_Keep;
	stencilFaceState.passOp = WGPUStencilOperation_Keep;
}

void SetDefaultDepthStencilState(WGPUDepthStencilState& depthStencilState)
{
	depthStencilState.format = WGPUTextureFormat_Undefined;
	depthStencilState.depthWriteEnabled = false;
	depthStencilState.depthCompare = WGPUCompareFunction_Always;
	depthStencilState.stencilReadMask = 0xFFFFFFFF;
	depthStencilState.stencilWriteMask = 0xFFFFFFFF;
	depthStencilState.depthBias = 0;
	depthStencilState.depthBiasSlopeScale = 0;
	depthStencilState.depthBiasClamp = 0;

	SetDefaultStencilFaceState(depthStencilState.stencilFront);
	SetDefaultStencilFaceState(depthStencilState.stencilBack);
}

GraphicsDevice_t::GraphicsDevice_t(CWindow* window)
{
	//
	// Instance
	//
	Instance = CreateInstance();

	//
	// Surface
	//
	Surface = window->CreateSurface(Instance);

	//
	// Adapter
	//
	WGPURequestAdapterOptions adapterOpts = {};
	Adapter = RequestAdapter(Instance, &adapterOpts);

	//
	// Device
	//
	WGPUDeviceDescriptor deviceDesc = {
		.nextInChain = nullptr,
		.label = "Main Device",
		.requiredFeatureCount = 0,
		.requiredLimits = nullptr,

		.defaultQueue = {
			.nextInChain = nullptr,
			.label = "Main Queue"
		}
	};
	Device = RequestDevice(Adapter, &deviceDesc);

	//
	// Error callback
	//
	auto onDeviceError = [](WGPUErrorType type, char const* message, void* pUserData)
		{
			std::cout << "Uncaptured device error: type " << type;
			if (message) std::cout << " (" << message << ")";
			std::cout << std::endl;
		};

	wgpuDeviceSetUncapturedErrorCallback(Device, onDeviceError, nullptr);
	wgpuDeviceSetDeviceLostCallback(Device, nullptr, nullptr); // Stop Dawn complaining

	//
	// Main command context
	//
	Queue = wgpuDeviceGetQueue(Device);

	//
	// Swapchain
	//
	SwapChain = CreateSwapChain(Device, window);

	//
	// Depth texture
	//
	WGPUTextureDescriptor depthTextureDesc = {
		.usage = WGPUTextureUsage_RenderAttachment,
		.dimension = WGPUTextureDimension_2D,
		.size = {(unsigned int)window->GetSize().x, (unsigned int)window->GetSize().y, 1},
		.format = DepthTextureFormat,
		.mipLevelCount = 1,
		.sampleCount = 1,
		.viewFormatCount = 1,
		.viewFormats = &DepthTextureFormat,
	};

	DepthTexture = wgpuDeviceCreateTexture(Device, &depthTextureDesc);

	WGPUTextureViewDescriptor depthTextureViewDesc = {
		.format = DepthTextureFormat,
		.dimension = WGPUTextureViewDimension_2D,
		.baseMipLevel = 0,
		.mipLevelCount = 1,
		.baseArrayLayer = 0,
		.arrayLayerCount = 1,
		.aspect = WGPUTextureAspect_DepthOnly
	};

	DepthTextureView = wgpuTextureCreateView(DepthTexture, &depthTextureViewDesc);
	
	//
	// Model
	//
	Model = new Model_t();
	Model->Init(this, "content/models/suzanne.gltf");

	Camera = new Camera_t();
	Camera->Transform = *Transform_t::MakeDefault();
	Camera->Transform.SetPosition(glm::vec3(-1, 0, 0));
}

GraphicsDevice_t::~GraphicsDevice_t()
{
	delete Model;

#define RELEASE(x) do { if(x) { wgpu##x##Release(x); x = nullptr; } } while(0)
	RELEASE(Instance);
	RELEASE(Adapter);
	RELEASE(Device);
	RELEASE(Queue);
	RELEASE(SwapChain);
#undef RELEASE

	wgpuTextureViewRelease(DepthTextureView);
	wgpuTextureDestroy(DepthTexture);
	wgpuTextureRelease(DepthTexture);
}

void Graphics::OnRender(GraphicsDevice_t* gpu)
{
	Frame++;

	float d = Frame / 144.0f; // lol

	Camera->Transform.SetPosition(glm::vec3(
		glm::sin(d) * 4.0f,
		glm::cos(d) * 4.0f,
		(glm::cos(d) * 4.0f) + 4.0f
	));

	WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(gpu->SwapChain);

	if (!nextTexture)
	{
		std::cout << "Couldn't get next texture!" << std::endl;
		assert(false);
	}

	//
	// Create command encoder
	//
	WGPUCommandEncoderDescriptor encoderDesc = {
		.nextInChain = nullptr,
		.label = "Command encoder"
	};
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(gpu->Device, &encoderDesc);

	//
	// Encode commands
	//
	wgpuCommandEncoderInsertDebugMarker(encoder, "Main render pass");

	WGPURenderPassColorAttachment renderPassColorAttachment = {
		.view = nextTexture,
		.resolveTarget = nullptr,
		.loadOp = WGPULoadOp_Clear,
		.storeOp = WGPUStoreOp_Store,
		.clearValue = WGPUColor{ 0.0, 0.0, 0.0, 1.0 }
	};

	WGPURenderPassDepthStencilAttachment renderPassDepthAttachment = {
		.view = gpu->DepthTextureView,

		.depthLoadOp = WGPULoadOp_Clear,
		.depthStoreOp = WGPUStoreOp_Store,
		.depthClearValue = 1.0f,
		.depthReadOnly = false,

		.stencilLoadOp = WGPULoadOp_Undefined,
		.stencilStoreOp = WGPUStoreOp_Undefined,
		.stencilClearValue = 0,
		.stencilReadOnly = true
	};

	WGPURenderPassDescriptor renderPassDesc = {
		.nextInChain = nullptr,
		.colorAttachmentCount = 1,
		.colorAttachments = &renderPassColorAttachment,
		.depthStencilAttachment = &renderPassDepthAttachment,
		.timestampWrites = nullptr
	};

	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

	Model->Draw(gpu, renderPass);

	wgpuRenderPassEncoderEnd(renderPass);

	//
	// Finish rendering
	//
	WGPUCommandBufferDescriptor cmdBufferDescriptor = {
		.nextInChain = nullptr,
		.label = "Command buffer"
	};
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);

	wgpuQueueSubmit(gpu->Queue, 1, &command);

	wgpuCommandEncoderRelease(encoder);
	wgpuCommandBufferRelease(command);

	wgpuSwapChainPresent(gpu->SwapChain);

	//
	// Cleanup
	//
	wgpuTextureViewRelease(nextTexture);
}

GraphicsBuffer_t Graphics::MakeVertexBuffer(GraphicsDevice_t* gpu, std::vector<Vertex_t> vertexData, WGPUVertexBufferLayout vertexBufferLayout)
{
	GraphicsBuffer_t vertexBuffer;

	WGPUBufferDescriptor vertexBufferDesc = {
		.nextInChain = nullptr,
		.label = "Vertex Data Buffer",
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
		.size = vertexData.size() * sizeof(Vertex_t),
		.mappedAtCreation = false
	};

	vertexBuffer.DataBuffer = wgpuDeviceCreateBuffer(gpu->Device, &vertexBufferDesc);

	wgpuQueueWriteBuffer(gpu->Queue, vertexBuffer.DataBuffer, 0, vertexData.data(), vertexBufferDesc.size);

	vertexBuffer.Count = vertexData.size();
	vertexBuffer.DataSize = vertexData.size();

	return vertexBuffer;
}

GraphicsBuffer_t Graphics::MakeIndexBuffer(GraphicsDevice_t* gpu, std::vector<unsigned int> indexData)
{
	GraphicsBuffer_t indexBuffer;

	WGPUBufferDescriptor indexBufferDesc = {
		.nextInChain = nullptr,
		.label = "Index Data Buffer",
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
		.size = indexData.size() * sizeof(unsigned int),
		.mappedAtCreation = false
	};

	indexBuffer.DataBuffer = wgpuDeviceCreateBuffer(gpu->Device, &indexBufferDesc);
	wgpuQueueWriteBuffer(gpu->Queue, indexBuffer.DataBuffer, 0, indexData.data(), indexBufferDesc.size);

	indexBuffer.Count = indexData.size();
	indexBuffer.DataSize = indexData.size();

	return indexBuffer;
}

GraphicsBuffer_t Graphics::MakeUniformBuffer(GraphicsDevice_t* gpu)
{
	GraphicsBuffer_t uniformBuffer;

	WGPUBufferDescriptor uniformBufferDesc = {
		.nextInChain = nullptr,
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
		.size = sizeof(UniformBuffer_t),
		.mappedAtCreation = false
	};

	uniformBuffer.DataBuffer = wgpuDeviceCreateBuffer(gpu->Device, &uniformBufferDesc);
	uniformBuffer.Count = 1;
	uniformBuffer.DataSize = sizeof(UniformBuffer_t);

	return uniformBuffer;
}

void Graphics::UpdateUniformBuffer(GraphicsDevice_t* gpu, GraphicsBuffer_t uniformBuffer, UniformBuffer_t uniformBufferData)
{
	wgpuQueueWriteBuffer(gpu->Queue, uniformBuffer.DataBuffer, 0, (void*)&uniformBufferData, sizeof(UniformBuffer_t));
}

void GraphicsBuffer_t::Destroy()
{
	wgpuBufferDestroy(DataBuffer);
	wgpuBufferRelease(DataBuffer);
}

void Mesh_t::Init(GraphicsDevice_t* gpu, std::vector<Vertex_t> vertices, std::vector<unsigned int> indices, Texture_t colorTexture)
{
	ColorTexture = colorTexture;
	UniformBuffer = Graphics::MakeUniformBuffer(gpu);

	assert(colorTexture.Texture != nullptr);

	// Vertices
	WGPUVertexAttribute vertexAttribute = {
		.format = WGPUVertexFormat_Float32x3,
		.offset = 0,
		.shaderLocation = 0,
	};

	WGPUVertexAttribute uvAttribute = {
		.format = WGPUVertexFormat_Float32x2,
		.offset = sizeof(glm::vec3),
		.shaderLocation = 1,
	};

	WGPUVertexAttribute normalAttribute = {
		.format = WGPUVertexFormat_Float32x3,
		.offset = sizeof(glm::vec3) + sizeof(glm::vec2),
		.shaderLocation = 2,
	};

	std::vector<WGPUVertexAttribute> vertexAttributes = { vertexAttribute, uvAttribute, normalAttribute };

	WGPUVertexBufferLayout vertexBufferLayout = {
		.arrayStride = sizeof(Vertex_t),
		.stepMode = WGPUVertexStepMode_Vertex,
		.attributeCount = vertexAttributes.size(),
		.attributes = vertexAttributes.data()
	};

	VertexBuffer = Graphics::MakeVertexBuffer(gpu, vertices, vertexBufferLayout);

	// Indices
	IndexBuffer = Graphics::MakeIndexBuffer(gpu, indices);

	// Shader
	WGPUShaderModule shaderModule = CreateShader(gpu->Device);

	// Pipeline
	WGPUBlendState blendState = {
		.color = {
			.operation = WGPUBlendOperation_Add,
			.srcFactor = WGPUBlendFactor_SrcAlpha,
			.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha
		}
	};

	WGPUColorTargetState colorTarget = {
		.format = WGPUTextureFormat_BGRA8Unorm,
		.blend = &blendState,
		.writeMask = WGPUColorWriteMask_All
	};

	WGPUFragmentState fragmentState = {
		.module = shaderModule,
		.entryPoint = "fs_main",
		.constantCount = 0,
		.constants = nullptr,
		.targetCount = 1,
		.targets = &colorTarget
	};

	WGPUVertexState vertexState = {
		.module = shaderModule,
		.entryPoint = "vs_main",
		.constantCount = 0,
		.constants = nullptr,
		.bufferCount = 1,
		.buffers = &vertexBufferLayout
	};

	std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);

	//
	// Uniform binding
	//
	WGPUBindGroupLayoutEntry& vertexBindingLayout = bindingLayoutEntries[0];
	SetDefaultBindGroupLayoutEntry(vertexBindingLayout);
	vertexBindingLayout.binding = 0;
	vertexBindingLayout.visibility = WGPUShaderStage_Vertex;
	vertexBindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
	vertexBindingLayout.buffer.minBindingSize = sizeof(UniformBuffer_t);

	WGPUBindGroupEntry uniformBinding = {
		.nextInChain = nullptr,
		.binding = 0,
		.buffer = UniformBuffer.DataBuffer,
		.offset = 0,
		.size = sizeof(UniformBuffer_t)
	};

	//
	// Color texture binding
	//
	WGPUBindGroupLayoutEntry& fragmentBindingLayout = bindingLayoutEntries[1];
	SetDefaultBindGroupLayoutEntry(fragmentBindingLayout);
	fragmentBindingLayout.binding = 1;
	fragmentBindingLayout.visibility = WGPUShaderStage_Fragment;
	fragmentBindingLayout.texture.sampleType = WGPUTextureSampleType_Float;
	fragmentBindingLayout.texture.viewDimension = WGPUTextureViewDimension_2D;

	WGPUBindGroupEntry textureBinding = {
		.nextInChain = nullptr,
		.binding = 1,
		.textureView = ColorTexture.TextureView
	};

	//
	// Sampler
	//
	WGPUBindGroupLayoutEntry& samplerBindingLayout = bindingLayoutEntries[2];
	samplerBindingLayout.binding = 2;
	samplerBindingLayout.visibility = WGPUShaderStage_Fragment;
	samplerBindingLayout.sampler.type = WGPUSamplerBindingType_Filtering;

	WGPUBindGroupEntry samplerBinding = {
		.nextInChain = nullptr,
		.binding = 2,
		.sampler = ColorTexture.Sampler
	};

	std::vector<WGPUBindGroupEntry> bindings = { uniformBinding, textureBinding, samplerBinding };

	WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {
		.nextInChain = nullptr,
		.entryCount = bindingLayoutEntries.size(),
		.entries = bindingLayoutEntries.data()
	};

	WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(gpu->Device, &bindGroupLayoutDesc);

	WGPUPipelineLayoutDescriptor layoutDesc = {
		.nextInChain = nullptr,
		.bindGroupLayoutCount = 1,
		.bindGroupLayouts = &bindGroupLayout
	};

	WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(gpu->Device, &layoutDesc);

	WGPUBindGroupDescriptor bindGroupDesc = {
		.nextInChain = nullptr,
		.layout = bindGroupLayout,
		.entryCount = (unsigned int)bindings.size(),
		.entries = bindings.data()
	};

	BindGroup = wgpuDeviceCreateBindGroup(gpu->Device, &bindGroupDesc);

	WGPUDepthStencilState depthStencilState = {};
	SetDefaultDepthStencilState(depthStencilState);
	depthStencilState.depthCompare = WGPUCompareFunction_Less;
	depthStencilState.depthWriteEnabled = true;
	depthStencilState.format = DepthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

	WGPURenderPipelineDescriptor pipelineDesc = {
		.nextInChain = nullptr,
		.layout = layout,
		.vertex = vertexState,

		.primitive = {
			.topology = WGPUPrimitiveTopology_TriangleList,
			.stripIndexFormat = WGPUIndexFormat_Undefined,
			.frontFace = WGPUFrontFace_CCW,
			.cullMode = WGPUCullMode_None
		},

		.depthStencil = &depthStencilState,
		.multisample = {
			.count = 1,
			.mask = ~0u,
			.alphaToCoverageEnabled = false
		},
		.fragment = &fragmentState
	};

	Pipeline = wgpuDeviceCreateRenderPipeline(gpu->Device, &pipelineDesc);
}

void Mesh_t::Draw(GraphicsDevice_t* gpu, WGPURenderPassEncoder renderPass)
{
	UniformBuffer_t uniformBufferData;
	uniformBufferData.ModelMatrix = GetModelMatrix();
	uniformBufferData.ViewProjMatrix = Camera->GetViewProjMatrix();
	Graphics::UpdateUniformBuffer(gpu, UniformBuffer, uniformBufferData);

	wgpuRenderPassEncoderSetPipeline(renderPass, Pipeline);
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, VertexBuffer.DataBuffer, 0, VertexBuffer.DataSize * sizeof(float));
	wgpuRenderPassEncoderSetIndexBuffer(renderPass, IndexBuffer.DataBuffer, WGPUIndexFormat_Uint32, 0, IndexBuffer.DataSize * sizeof(unsigned int));
	wgpuRenderPassEncoderSetBindGroup(renderPass, 0, BindGroup, 0, nullptr);

	wgpuRenderPassEncoderDrawIndexed(renderPass, IndexBuffer.Count, 1, 0, 0, 0);
}

void Model_t::Init(GraphicsDevice_t* gpu, const char* gltfPath)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	std::string err;
	std::string warn;

	// Check if filePath ends with .glb
	std::string filePathStr = gltfPath;
	std::string extension = filePathStr.substr(filePathStr.length() - 4, 4);

	bool ret;

	if (extension == ".glb")
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, gltfPath);
	else
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, gltfPath);

	if (!err.empty())
	{
		std::cout << "GLTF Error: " << err << std::endl;
		return;
	}

	if (!warn.empty())
	{
		std::cout << "GLTF Warning: " << warn << std::endl;
	}

	if (!ret)
	{
		std::cout << "GLTF Error: " << err << std::endl;
		return;
	}

	std::cout << "GLTF loaded: " << gltfPath << std::endl;

	// Load all meshes
	for (auto& mesh : model.meshes)
	{
		std::vector<Vertex_t> vertices = {};
		std::vector<unsigned int> indices = {};

		for (auto& primitive : mesh.primitives)
		{
			// Load indices
			{
				auto& accessor = model.accessors[primitive.indices];
				auto& bufferView = model.bufferViews[accessor.bufferView];
				auto& buffer = model.buffers[bufferView.buffer];

				auto& data = buffer.data[accessor.byteOffset + bufferView.byteOffset];

				for (int i = 0; i < accessor.count; ++i)
				{
					unsigned int index = 0;

					// GLTF stores indices as uint16 - this line is correct!
					memcpy(&index, &buffer.data[accessor.byteOffset + bufferView.byteOffset + i * sizeof(uint16_t)], sizeof(uint16_t));

					indices.emplace_back(index);
				}
			}

			// Load vertices
			{
				auto& posAccessor = model.accessors[primitive.attributes["POSITION"]];
				auto& posBufferView = model.bufferViews[posAccessor.bufferView];
				auto& posBuffer = model.buffers[posBufferView.buffer];

				auto& uvAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
				auto& uvBufferView = model.bufferViews[uvAccessor.bufferView];
				auto& uvBuffer = model.buffers[uvBufferView.buffer];

				auto& normAccessor = model.accessors[primitive.attributes["NORMAL"]];
				auto& normBufferView = model.bufferViews[normAccessor.bufferView];
				auto& normBuffer = model.buffers[normBufferView.buffer];

				for (int i = 0; i < posAccessor.count; ++i)
				{
					Vertex_t vertex = {};

					// Load position
					memcpy(&vertex.Position, &posBuffer.data[posAccessor.byteOffset + posBufferView.byteOffset + i * sizeof(glm::vec3)], sizeof(glm::vec3));

					// Load UVs
					if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
						memcpy(&vertex.TexCoords, &uvBuffer.data[uvAccessor.byteOffset + uvBufferView.byteOffset + i * sizeof(glm::vec2)], sizeof(glm::vec2));
					else
						vertex.TexCoords = glm::vec2(0.0f, 0.0f); // Default UVs if not present

					// Load normals
					if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
						memcpy(&vertex.Normal, &posBuffer.data[normAccessor.byteOffset + normBufferView.byteOffset + i * sizeof(glm::vec3)], sizeof(glm::vec3));
					else
						vertex.Normal = glm::vec3(0, 0, 0); // Default normals if not present

					vertices.emplace_back(vertex);
				}
			}

			Texture_t texture;

			if (primitive.material >= 0)
			{
				const tinygltf::Material& material = model.materials[primitive.material];

				if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
				{
					const tinygltf::Texture& gltfTexture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
					auto& image = model.images[gltfTexture.source];

					// Directly load texture from memory
					texture.LoadFromMemory(gpu, image.image.data(), image.width, image.height, image.component);
				}
			}

			Mesh_t newMesh;
			newMesh.Init(gpu, vertices, indices, texture);
			Meshes.push_back(newMesh);
		}
	}
}

void Model_t::Draw(GraphicsDevice_t* gpu, WGPURenderPassEncoder renderPass)
{
	for (auto& mesh : Meshes)
	{
		mesh.Draw(gpu, renderPass);
	}
}

void Model_t::Destroy()
{
	for (auto& mesh : Meshes)
	{
		mesh.Destroy();
	}
}

void Mesh_t::Destroy()
{
	VertexBuffer.Destroy();
	IndexBuffer.Destroy();
}

void Texture_t::LoadFromMemory(GraphicsDevice_t* gpu, const unsigned char* data, int width, int height, int channels)
{
	WGPUTextureDescriptor textureDesc = {
		.nextInChain = nullptr,
		.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
		.dimension = WGPUTextureDimension_2D,
		.size = { (unsigned int)width, (unsigned int)height, 1 },
		.format = WGPUTextureFormat_RGBA8Snorm,

		.mipLevelCount = 1,
		.sampleCount = 1,
		.viewFormatCount = 0,
		.viewFormats = nullptr
	};

	Texture = wgpuDeviceCreateTexture(gpu->Device, &textureDesc);

	WGPUImageCopyTexture destination = {
		.nextInChain = nullptr,
		.texture = Texture,
		.mipLevel = 0,
		.origin = { 0, 0, 0 },
		.aspect = WGPUTextureAspect_All,
	};

	WGPUTextureDataLayout source = {
		.offset = 0,
		.bytesPerRow = (unsigned int)(4 * width),
		.rowsPerImage = (unsigned int)height
	};

	wgpuQueueWriteTexture(gpu->Queue, &destination, data, static_cast<size_t>(width * height * channels), &source, &textureDesc.size);

	WGPUTextureViewDescriptor textureViewDesc = {
		.nextInChain = nullptr,
		.format = textureDesc.format,
		.dimension = WGPUTextureViewDimension_2D,
		.baseMipLevel = 0,
		.mipLevelCount = 1,
		.baseArrayLayer = 0,
		.arrayLayerCount = 1,
		.aspect = WGPUTextureAspect_All
	};

	TextureView = wgpuTextureCreateView(Texture, &textureViewDesc);

	WGPUSamplerDescriptor samplerDesc = {
		.addressModeU = WGPUAddressMode_ClampToEdge,
		.addressModeV = WGPUAddressMode_ClampToEdge,
		.addressModeW = WGPUAddressMode_ClampToEdge,
		.magFilter = WGPUFilterMode_Linear,
		.minFilter = WGPUFilterMode_Linear,
		.mipmapFilter = WGPUMipmapFilterMode_Linear,
		.lodMinClamp = 0.0f,
		.lodMaxClamp = 1.0f,
		.compare = WGPUCompareFunction_Undefined,
		.maxAnisotropy = 1
	};

	Sampler = wgpuDeviceCreateSampler(gpu->Device, &samplerDesc);
}