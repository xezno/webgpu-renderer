#include "gpu.hpp"
#include "window.hpp"

#include <webgpu/webgpu.h>

#include <cassert>
#include <vector>
#include <iostream>

static Triangle_t* Triangle;

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
		struct VertexOutput {
			@builtin(position) position: vec4f,
			@location(0) color: vec4f
		};
		
		@vertex
		fn vs_main(@location(0) position: vec3f) -> VertexOutput
		{
			var out : VertexOutput;
			
			out.color = vec4f(position.x, position.y, 0.5, 1.0);
			out.position = vec4f(position, 1.0);
			
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

void Triangle_t::Init(GraphicsDevice_t* gpu)
{
	//
	// Vertex data
	//
	std::vector<float> vertices = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};

	WGPUBufferDescriptor vertexBufferDesc = {
		.nextInChain = nullptr,
		.label = "Triangle Vertex Data Buffer",
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
		.size = vertices.size() * sizeof(float),
		.mappedAtCreation = false
	};

	VertexDataBuffer = wgpuDeviceCreateBuffer(gpu->Device, &vertexBufferDesc);

	WGPUVertexAttribute vertexAttribute = {
		.format = WGPUVertexFormat_Float32x3,
		.offset = 0,
		.shaderLocation = 0,
	};

	WGPUVertexBufferLayout vertexBufferLayout = {
		.arrayStride = 3 * sizeof(float),
		.stepMode = WGPUVertexStepMode_Vertex,
		.attributeCount = 1,
		.attributes = &vertexAttribute
	};

	wgpuQueueWriteBuffer(gpu->Queue, VertexDataBuffer, 0, vertices.data(), vertexBufferDesc.size);

	VertexCount = 3;
	VertexDataSize = vertices.size();

	//
	// Shader
	//
	WGPUShaderModule shaderModule = CreateShader(gpu->Device);

	//
	// Pipeline
	//
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

Triangle_t::~Triangle_t()
{
	wgpuBufferDestroy(VertexDataBuffer);
	wgpuBufferRelease(VertexDataBuffer);
}

void Triangle_t::Draw(WGPURenderPassEncoder renderPass)
{
	wgpuRenderPassEncoderSetPipeline(renderPass, Pipeline);
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, VertexDataBuffer, 0, VertexDataSize * sizeof(float));
	wgpuRenderPassEncoderDraw(renderPass, VertexCount, 1, 0, 0);
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
	// Triangle
	//
	Triangle = new Triangle_t();
	Triangle->Init(this);
}

GraphicsDevice_t::~GraphicsDevice_t()
{
	delete Triangle;

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

	Triangle->Draw(renderPass);

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