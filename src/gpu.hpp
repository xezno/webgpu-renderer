#pragma once

#include "webgpu/webgpu.h"

struct GraphicsDevice_t
{
public:
	WGPUInstance Instance;
	WGPUAdapter Adapter;

	GraphicsDevice_t();
	~GraphicsDevice_t();
};

namespace Graphics
{
	void OnRender(GraphicsDevice_t* gpu);
}