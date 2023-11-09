#pragma once

#include "webgpu/webgpu.h"

class CWindow;
struct GraphicsDevice_t;

struct Triangle_t
{
private:
	WGPURenderPipeline Pipeline									= nullptr;

	WGPUBuffer IndexDataBuffer									= nullptr;
	size_t IndexDataSize										= -1;
	int IndexCount												= -1;

	WGPUBuffer VertexDataBuffer									= nullptr;
	size_t VertexDataSize										= -1;
	int VertexCount												= -1;

public:
	void Init(GraphicsDevice_t* gpu);
	void Draw(WGPURenderPassEncoder renderPass);

	~Triangle_t();
};

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