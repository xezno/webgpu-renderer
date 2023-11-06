#include "CrossWindow/CrossWindow.h"
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

	xwin::Window window;
	xwin::EventQueue eventQueue;

	if (!window.create(windowDesc, eventQueue))
		return;

	bool shouldRun = true;
	while (shouldRun)
	{
		eventQueue.update();
		while (!eventQueue.empty())
		{
			const xwin::Event& event = eventQueue.front();

			if (event.type == xwin::EventType::MouseInput)
			{
				const xwin::MouseInputData mouse = event.data.mouseInput;
			}
			if (event.type == xwin::EventType::Close)
			{
				window.close();
				shouldRun = false;
			}

			eventQueue.pop();
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

CWindow::~CWindow()
{
}