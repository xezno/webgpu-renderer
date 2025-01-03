#pragma once

#include <glm/glm.hpp>

#include <webgpu/webgpu.h>
#include <string>

struct GraphicsDevice_t;
struct GLFWwindow;

/*
 * Creates and manages a window.
 */
class CWindow {
private:
	//
	// Window properties
	//
	int Width													= 1280;
	int Height													= 720;
	std::string Title											= "Hack week - WebGPU";

	//
	// Rendering
	//
	GraphicsDevice_t* GraphicsDevice							= nullptr;

	//
	// Events
	//
	void (*FrameFunc)(GraphicsDevice_t*)						= nullptr;

	//
	// GLFW
	//
	GLFWwindow* Window											= nullptr;
	WGPUSurface Surface											= nullptr;

private:
	// Should this window close?
	bool ShouldWindowClose();

public:
	// Retrieve the window size
	glm::ivec2 GetSize()										{ return glm::ivec2{ Width, Height }; }

	// Set the window size
	void SetSize(glm::ivec2 size)								{ Width = size.x; Height = size.y; }

	// Set the window size
	void SetSize(int width, int height)							{ Width = width; Height = height; }

	// Set the window title
	void SetTitle(std::string title)							{ Title = title; }

	// Retrieve the window title
	std::string GetTitle()										{ return Title; };

	// Set a function that runs every frame
	void SetFrameFunc(void (*func)(GraphicsDevice_t*))			{ FrameFunc = func; }

	// Run the application
	void Run();

	// Set the current graphics device
	void SetGraphicsDevice(GraphicsDevice_t* graphicsDevice)	{ GraphicsDevice = graphicsDevice; }

	// Get the WebGPU surface to render to
	WGPUSurface GetSurface();

	// Create a WebGPU surface to render to
	WGPUSurface CreateSurface(WGPUInstance instance);

	CWindow();
	~CWindow();
};