#include "gpu.hpp"
#include "window.hpp"

#include <webgpu/webgpu.h>

#include <cassert>
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
		fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> VertexOutput
		{
			var out : VertexOutput;
			
			var p = vec2f(0.0, 0.0);
			if (in_vertex_index == 0u) {
				p = vec2f(-0.5, -0.5);
				out.color = vec4f(1.0, 0.0, 0.0, 1.0);
			} else if (in_vertex_index == 1u) {
				p = vec2f(0.5, -0.5);
				out.color = vec4f(0.0, 1.0, 0.0, 1.0);
			} else {
				p = vec2f(0.0, 0.5);
				out.color = vec4f(0.0, 0.0, 1.0, 1.0);
			}

			out.position = vec4f(p, 0.0, 1.0);
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
	WGPUShaderModule shaderModule = CreateShader(gpu->Device);

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
		.bufferCount = 0,
		.buffers = 0
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

void Triangle_t::Draw(WGPURenderPassEncoder renderPass)
{
	wgpuRenderPassEncoderSetPipeline(renderPass, Pipeline);
	wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);
}

GraphicsDevice_t::GraphicsDevice_t(CWindow* window)
{
	//
	// Instance
	//
	Instance = CreateInstance();
	std::cout << "Created instance: " << Instance << std::endl;

	//
	// Surface
	//
	Surface = window->CreateSurface(Instance);

	//
	// Adapter
	//
	WGPURequestAdapterOptions adapterOpts = {};
	Adapter = RequestAdapter(Instance, &adapterOpts);
	std::cout << "Got adapter: " << Adapter << std::endl;

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
	std::cout << "Got device: " << Device << std::endl;

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
	std::cout << "Next texture: " << nextTexture << std::endl;

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