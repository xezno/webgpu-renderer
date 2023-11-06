#pragma once

#include "webgpu/webgpu.h"

class CWindow;

struct GraphicsDevice_t
{
private:
	WGPUInstance Instance				= nullptr;
	WGPUAdapter Adapter					= nullptr;
	WGPUSurface Surface					= nullptr;

public:
	GraphicsDevice_t(CWindow* window);
	~GraphicsDevice_t();
};

namespace Graphics
{
	void OnRender(GraphicsDevice_t* gpu);
}