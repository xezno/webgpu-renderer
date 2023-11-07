#include "gpu.hpp"
#include "window.hpp"

#include <webgpu/webgpu.h>

#include <cassert>
#include <iostream>

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
		.format = WGPUTextureFormat_BGRA8Unorm, // Dawn only supports this as swapchain - `wgpuSurfaceGetPreferredFormat` not implemented
		.width = static_cast<uint32_t>(window->GetSize().x),
		.height = static_cast<uint32_t>(window->GetSize().y),
		.presentMode = WGPUPresentMode_Fifo // First in, first out queue
	};

	// We can safely pass nullptr in here because the surface has already been 
	// created - we're just retrieving it (note: this is shit)
	WGPUSurface surface = window->GetSurface(nullptr);
	WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapchainDesc);

	return swapChain;
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
	Surface = window->GetSurface(Instance);

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

	//
	// Main command context
	//
	Queue = wgpuDeviceGetQueue(Device);

	WGPUCommandEncoderDescriptor encoderDesc = {
		.nextInChain = nullptr,
		.label = "Command encoder"
	};
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(Device, &encoderDesc);

	// Test debug marker
	wgpuCommandEncoderInsertDebugMarker(encoder, "Do a thing");

	WGPUCommandBufferDescriptor cmdBufferDescriptor = {
		.nextInChain = nullptr,
		.label = "Command buffer"
	};
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);

	std::cout << "Submitting command" << std::endl;
	wgpuQueueSubmit(Queue, 1, &command);

	wgpuCommandEncoderRelease(encoder);
	wgpuCommandBufferRelease(command);
	
	//
	// Swapchain
	//
	SwapChain = CreateSwapChain(Device, window);
}

GraphicsDevice_t::~GraphicsDevice_t() {
#define RELEASE(x) do { if(x) { wgpu##x##Release(x); x = nullptr; } } while(0)
	RELEASE(Instance);
	RELEASE(Adapter);
	RELEASE(Device);
	RELEASE(SwapChain);
#undef RELEASE
}

void Graphics::OnRender(GraphicsDevice_t* gpu)
{
	WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(gpu->SwapChain);
	std::cout << "Next texture: " << nextTexture << std::endl;

	wgpuSwapChainPresent(gpu->SwapChain);

	//
	// Cleanup
	//
	wgpuTextureViewRelease(nextTexture);
}