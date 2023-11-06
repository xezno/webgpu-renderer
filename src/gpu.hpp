#pragma once

#include "webgpu/webgpu.h"

struct GraphicsDevice_t
{
public:
	WGPUInstance Instance;
	GraphicsDevice_t();
};

namespace Graphics
{
	void OnRender(GraphicsDevice_t* gpu);
}	