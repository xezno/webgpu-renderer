#include "CrossWindow/CrossWindow.h"
#include "CrossWindow/Graphics.h"

#include "window.hpp"

#include <iostream>

CWindow::CWindow() {}

void CWindow::Run()
{
	xwin::WindowDesc windowDesc = {
		.width = (unsigned int)Width,
		.height = (unsigned int)Height,
		.visible = true,
		.title = Title,
		.name = "Main Window"
	};

	if (!Window.create(windowDesc, EventQueue))
		return;

	bool shouldRun = true;
	while (shouldRun)
	{
		EventQueue.update();
		while (!EventQueue.empty())
		{
			const xwin::Event& event = EventQueue.front();

			if (event.type == xwin::EventType::MouseInput)
			{
				const xwin::MouseInputData mouse = event.data.mouseInput;
			}
			if (event.type == xwin::EventType::Close)
			{
				Window.close();
				shouldRun = false;
			}

			EventQueue.pop();
		}

		//
		// Render
		//
		if (FrameFunc)
			FrameFunc(GraphicsDevice);
		else
			std::cout << "FrameFunc is null!" << std::endl;
	}
}

WGPUSurface* CWindow::GetSurface(WGPUInstance* instance)
{
	xgfx::getSurface()
	
}

CWindow::~CWindow()
{
}