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
		.width = static_cast<uint32_t>(window->GetSize().x),
		.height = static_cast<uint32_t>(window->GetSize().y),
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
			modelMatrix : mat4x4f,
			viewProjMatrix : mat4x4f
		};

		@group(0) @binding(0) var<uniform> uConstants : UniformBuffer;

		struct VertexOutput {
			@builtin(position) position: vec4f,
			@location(0) color: vec4f
		};
		
		@vertex
		fn vs_main(@location(0) position: vec3f) -> VertexOutput
		{
			var out : VertexOutput;
			
			out.color = vec4f(position.x, position.y, 0.5, 1.0);
			out.position = uConstants.viewProjMatrix * uConstants.modelMatrix * vec4f(position, 1.0);
			
			return out;
		}

		@fragment
		fn fs_main(in: VertexOutput) -> @location(0) vec4f
		{
			return in.color;
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
	// Model
	//
	Model = new Model_t();
	Model->Init(this, "content/models/box.gltf");

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
}

void Graphics::OnRender(GraphicsDevice_t* gpu)
{
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

	WGPURenderPassDescriptor renderPassDesc = {
		.nextInChain = nullptr,
		.colorAttachmentCount = 1,
		.colorAttachments = &renderPassColorAttachment,
		.depthStencilAttachment = nullptr,
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

GraphicsBuffer_t Graphics::MakeVertexBuffer(GraphicsDevice_t* gpu, std::vector<glm::vec3> vertexData, WGPUVertexBufferLayout** outVertexBufferLayout)
{
	GraphicsBuffer_t vertexBuffer;

	WGPUBufferDescriptor vertexBufferDesc = {
		.nextInChain = nullptr,
		.label = "Vertex Data Buffer",
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
		.size = vertexData.size() * sizeof(glm::vec3),
		.mappedAtCreation = false
	};

	vertexBuffer.DataBuffer = wgpuDeviceCreateBuffer(gpu->Device, &vertexBufferDesc);

	WGPUVertexAttribute* vertexAttribute = new WGPUVertexAttribute{
		.format = WGPUVertexFormat_Float32x3,
		.offset = 0,
		.shaderLocation = 0,
	};

	WGPUVertexBufferLayout* vertexBufferLayout = new WGPUVertexBufferLayout{
		.arrayStride = 3 * sizeof(float),
		.stepMode = WGPUVertexStepMode_Vertex,
		.attributeCount = 1,
		.attributes = vertexAttribute
	};

	wgpuQueueWriteBuffer(gpu->Queue, vertexBuffer.DataBuffer, 0, vertexData.data(), vertexBufferDesc.size);

	vertexBuffer.Count = vertexData.size();
	vertexBuffer.DataSize = vertexData.size();

	*outVertexBufferLayout = vertexBufferLayout;

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

void Mesh_t::Init(GraphicsDevice_t* gpu, std::vector<glm::vec3> vertices, std::vector<unsigned int> indices)
{
	UniformBuffer = Graphics::MakeUniformBuffer(gpu);

	// Vertices
	WGPUVertexBufferLayout* vertexBufferLayout;
	VertexBuffer = Graphics::MakeVertexBuffer(gpu, vertices, &vertexBufferLayout);

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
		.buffers = vertexBufferLayout
	};

	WGPUBindGroupLayoutEntry bindingLayout = {};
	SetDefaultBindGroupLayoutEntry(bindingLayout);
	bindingLayout.binding = 0;
	bindingLayout.visibility = WGPUShaderStage_Vertex;
	bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(UniformBuffer_t);

	WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {
		.nextInChain = nullptr,
		.entryCount = 1,
		.entries = &bindingLayout
	};

	WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(gpu->Device, &bindGroupLayoutDesc);

	WGPUPipelineLayoutDescriptor layoutDesc = {
		.nextInChain = nullptr,
		.bindGroupLayoutCount = 1,
		.bindGroupLayouts = &bindGroupLayout
	};

	WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(gpu->Device, &layoutDesc);

	WGPUBindGroupEntry binding = {
		.nextInChain = nullptr,
		.binding = 0,
		.buffer = UniformBuffer.DataBuffer,
		.offset = 0,
		.size = sizeof(UniformBuffer_t)
	};

	WGPUBindGroupDescriptor bindGroupDesc = {
		.nextInChain = nullptr,
		.layout = bindGroupLayout,
		.entryCount = bindGroupLayoutDesc.entryCount,
		.entries = &binding
	};

	BindGroup = wgpuDeviceCreateBindGroup(gpu->Device, &bindGroupDesc);

	WGPURenderPipelineDescriptor pipelineDesc = {
		.nextInChain = nullptr,
		.vertex = vertexState,

		.primitive = {
			.topology = WGPUPrimitiveTopology_TriangleList,
			.stripIndexFormat = WGPUIndexFormat_Undefined,
			.frontFace = WGPUFrontFace_CCW,
			.cullMode = WGPUCullMode_None
		},

		.depthStencil = nullptr,
		.multisample = {
			.count = 1,
			.mask = ~0u,
			.alphaToCoverageEnabled = false
		},
		.fragment = &fragmentState,
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
		std::vector<glm::vec3> vertices = {};
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
				auto& accessor = model.accessors[primitive.attributes["POSITION"]];
				auto& bufferView = model.bufferViews[accessor.bufferView];
				auto& buffer = model.buffers[bufferView.buffer];

				auto& data = buffer.data[accessor.byteOffset + bufferView.byteOffset];

				for (int i = 0; i < accessor.count; ++i)
				{
					glm::vec3 vert = {};

					memcpy(&vert, &buffer.data[accessor.byteOffset + bufferView.byteOffset + i * sizeof(glm::vec3)], sizeof(glm::vec3));

					vertices.emplace_back(vert);
				}
			}

			Mesh_t newMesh;
			newMesh.Init(gpu, vertices, indices);
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