#include "window.hpp"

#include <GLFW/glfw3.h>
#include <iostream>
#include <glfw3webgpu.h>

CWindow::CWindow()
{
	glfwInit();
}

void CWindow::Run()
{
	GLFWwindow* window = glfwCreateWindow(Width, Height, Title.c_str(), nullptr, nullptr);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		//
		// Render
		//
		if (FrameFunc)
			FrameFunc(GraphicsDevice);
		else
			std::cout << "FrameFunc is null!" << std::endl;
	}

	glfwDestroyWindow(window);
}

WGPUSurface CWindow::GetSurface(WGPUInstance instance)
{
	if (Surface == nullptr)
	{
		Surface = glfwGetWGPUSurface(instance, Window);
	}
	return Surface;
}

CWindow::~CWindow()
{
	glfwTerminate();
}