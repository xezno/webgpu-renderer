#include "CrossWindow/CrossWindow.h"
#include "window.hpp"

#include <iostream>

CWindow::CWindow() {}

void CWindow::Run()
{
	xwin::WindowDesc windowDesc = {
		.width = 1280,
		.height = 720,
		.visible = true,
		.title = "CWindow Test",
		.name = "Test"
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
			FrameFunc();
		else
			std::cout << "FrameFunc is null!" << std::endl;
	}
}

CWindow::~CWindow()
{
}