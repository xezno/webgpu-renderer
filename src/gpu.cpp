#include "gpu.hpp"

#include "webgpu/webgpu.h"

#include <cassert>
#include <iostream>

WGPUAdapter RequestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
	struct UserData {
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};

	UserData userData;
	auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* pUserData) {
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		if (status == WGPURequestAdapterStatus_Success) {
			userData.adapter = adapter;
		}
		else {
			std::cout << "Could not get WebGPU adapter: " << message << std::endl;
		}
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

GraphicsDevice_t::GraphicsDevice_t()
{
	WGPUInstance instance = CreateInstance();

	std::cout << "Created instance: " << instance << std::endl;

	WGPURequestAdapterOptions adapterOpts = {};
	WGPUAdapter adapter = RequestAdapter(instance, &adapterOpts);

	std::cout << "Got adapter: " << adapter << std::endl;
}

void Graphics::OnRender(GraphicsDevice_t* gpu)
{
	std::cout << "Graphics::OnRender()" << std::endl;
}