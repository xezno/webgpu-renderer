#pragma once

#include "webgpu/webgpu.h"

class CWindow;

/*
 * Describes a rendering context/state
 */
struct GraphicsDevice_t
{
	WGPUInstance Instance										= nullptr;
	WGPUAdapter Adapter											= nullptr;
	WGPUSurface Surface											= nullptr;
	WGPUDevice Device											= nullptr;
	WGPUQueue Queue												= nullptr;
	WGPUSwapChain SwapChain										= nullptr;

	GraphicsDevice_t(CWindow* window);
	~GraphicsDevice_t();
};

/*
 * Rendering functions
 */
namespace Graphics
{
	void OnRender(GraphicsDevice_t* gpu);
}