#pragma once

#include "webgpu/webgpu.h"

#include <vector>

class CWindow;
struct GraphicsDevice_t;

/*
 *
 */
struct GraphicsBuffer_t
{
	WGPUBuffer DataBuffer										= nullptr;
	size_t DataSize												= SIZE_MAX;
	int Count													= -1;

	void Destroy();
};

struct Triangle_t
{
private:
	WGPURenderPipeline Pipeline									= nullptr;
	GraphicsBuffer_t IndexBuffer = {};
	GraphicsBuffer_t VertexBuffer = {};

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

	GraphicsBuffer_t MakeVertexBuffer(GraphicsDevice_t* gpu, std::vector<float> vertexData, WGPUVertexBufferLayout** vertexBufferLayout);
	GraphicsBuffer_t MakeIndexBuffer(GraphicsDevice_t* gpu, std::vector<unsigned int> indexData);
}