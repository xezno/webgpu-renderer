#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <webgpu/webgpu.h>

#include <vector>

class CWindow;
struct GraphicsDevice_t;
struct Vector3_t;

/*
 *
 */
struct Vertex_t
{
	glm::vec3 Position											= {};
	glm::vec2 TexCoords											= {};
	glm::vec3 Normal											= {};
	glm::vec3 Tangent											= {};
};

/*
 *
 */
struct Transform_t
{
private:
	glm::vec4 PositionAndScale									= {0,0,0,1};
	glm::quat Rotation											= {0,0,0,1};

public:
	float GetScale()											{ return PositionAndScale.w; }
	void SetScale(float scale)									{ PositionAndScale.w = scale; }

	glm::vec3 GetPosition()										{ return glm::vec3(PositionAndScale); }
	void SetPosition(glm::vec3 newPos)							{ float scale = GetScale(); PositionAndScale = glm::vec4(newPos, scale); }

	glm::quat GetRotation()										{ return Rotation; }
	void SetRotation(glm::quat newRot)							{ Rotation = newRot; }

	static Transform_t* MakeDefault()
	{
		Transform_t* tx = new Transform_t();
		tx->PositionAndScale = { 0,0,0,1 };
		tx->Rotation = { 0,0,0,1 };
		return tx;
	}
};

/*
 *
 */
struct Camera_t
{
	Transform_t Transform										= {};

	float FieldOfView											= 40.0f;
	float ZNear													= 0.1f;
	float ZFar													= 100.0f;
	float Aspect												= 16.0f / 9.0f;

	inline glm::mat4 GetViewProjMatrix()
	{
		glm::mat4 view = glm::lookAt(Transform.GetPosition(), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
		glm::mat4 projection = glm::perspective(glm::radians(FieldOfView), Aspect, ZNear, ZFar);

		return projection * view;
	}
};

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

/*
 *
 */
struct Texture_t
{
	WGPUTexture Texture											= nullptr;
	WGPUTextureView TextureView									= nullptr;

	void LoadFromMemory(GraphicsDevice_t* gpu, const unsigned char* data, int width, int height, int channels);
};

struct Material_t
{
	Texture_t ColorTexture										= {};
	Texture_t AoTexture											= {};
	Texture_t EmissiveTexture									= {};
	Texture_t MetalRoughnessTexture								= {};
	Texture_t NormalTexture										= {};

	WGPUSampler Sampler											= nullptr;
};

/*
 *
 */
struct Mesh_t
{
private:
	friend struct Model_t;
	
	WGPURenderPipeline Pipeline									= nullptr;
	WGPUBindGroup BindGroup										= nullptr;
	GraphicsBuffer_t IndexBuffer								= {};
	GraphicsBuffer_t VertexBuffer								= {};
	Transform_t Transform										= {};
	GraphicsBuffer_t UniformBuffer								= {};

	Material_t Material											= {};

	void Init(GraphicsDevice_t* gpu, std::vector<Vertex_t> vertices, std::vector<unsigned int> indices, Material_t material);
	void Draw(GraphicsDevice_t* gpu, WGPURenderPassEncoder renderPass);

	inline glm::mat4 GetModelMatrix()
	{
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(Transform.GetScale()));
		glm::mat4 rotationMatrix = glm::mat4_cast(Transform.GetRotation());
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), Transform.GetPosition());

		return translationMatrix * rotationMatrix * scaleMatrix;
	}

	void Destroy();
};

/*
 *
 */
struct Model_t
{
private:
	std::vector<Mesh_t> Meshes = {};

public:
	void Init(GraphicsDevice_t* gpu, const char* gltfPath);
	void Draw(GraphicsDevice_t* gpu, WGPURenderPassEncoder renderPass);

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

	WGPUTextureView DepthTextureView							= nullptr;
	WGPUTexture DepthTexture									= nullptr;

	GraphicsDevice_t(CWindow* window);
	~GraphicsDevice_t();
};

/*
 *
 */
struct UniformBuffer_t
{
	glm::mat4 ModelMatrix										= {};
	glm::mat4 ViewProjMatrix									= {};
	glm::vec3 CameraPosition									= {};
	float unused												= -1.0f;
};

/*
 * Rendering functions
 */
namespace Graphics
{
	void OnRender(GraphicsDevice_t* gpu);

	GraphicsBuffer_t MakeVertexBuffer(GraphicsDevice_t* gpu, std::vector<Vertex_t> vertexData, WGPUVertexBufferLayout vertexBufferLayout);
	GraphicsBuffer_t MakeIndexBuffer(GraphicsDevice_t* gpu, std::vector<unsigned int> indexData);

	GraphicsBuffer_t MakeUniformBuffer(GraphicsDevice_t* gpu);
	void UpdateUniformBuffer(GraphicsDevice_t* gpu, GraphicsBuffer_t uniformBuffer, UniformBuffer_t uniformBufferData);
}