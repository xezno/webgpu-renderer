#include "window.hpp"

#include <GLFW/glfw3.h>
#include <iostream>
#include <glfw3webgpu.h>
#include <cassert>

CWindow::CWindow()
{
	glfwInit();

	// Non-resizable window
	glfwWindowHint(GLFW_RESIZABLE, 0);
	Window = glfwCreateWindow(Width, Height, Title.c_str(), nullptr, nullptr);
}

bool CWindow::ShouldWindowClose()
{
	return glfwWindowShouldClose(Window);
}

void CWindow::Run()
{
	while (!ShouldWindowClose())
	{
		glfwPollEvents();

		//
		// Render
		//
		assert(FrameFunc != nullptr);

		FrameFunc(GraphicsDevice);
	}
}

WGPUSurface CWindow::CreateSurface(WGPUInstance instance)
{
	assert(Surface == nullptr);

	Surface = glfwGetWGPUSurface(instance, Window);
	return Surface;
}

WGPUSurface CWindow::GetSurface()
{
	return Surface;
}

CWindow::~CWindow()
{
	glfwDestroyWindow(Window);
	glfwTerminate();
}