#pragma once

#include "webgpu/webgpu.h"

#include <vector>

class CWindow;
struct GraphicsDevice_t;
struct Vector3_t;

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

struct Mesh_t
{
private:
	friend struct Model_t;
	
	WGPURenderPipeline Pipeline									= nullptr;
	GraphicsBuffer_t IndexBuffer								= {};
	GraphicsBuffer_t VertexBuffer								= {};

	void Init(GraphicsDevice_t* gpu, std::vector<Vector3_t> vertices, std::vector<unsigned int> indices);
	void Draw(WGPURenderPassEncoder renderPass);

	void Destroy();
};

struct Model_t
{
private:
	std::vector<Mesh_t> Meshes									= {};

public:
	void Init(GraphicsDevice_t* gpu, const char* gltfPath);
	void Draw(WGPURenderPassEncoder renderPass);

	void Destroy();
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

	GraphicsBuffer_t MakeVertexBuffer(GraphicsDevice_t* gpu, std::vector<Vector3_t> vertexData, WGPUVertexBufferLayout** vertexBufferLayout);
	GraphicsBuffer_t MakeIndexBuffer(GraphicsDevice_t* gpu, std::vector<unsigned int> indexData);
}