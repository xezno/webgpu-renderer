#include "CrossWindow/CrossWindow.h"
#include "window.hpp"

#include <iostream>

CWindow::CWindow() {}

void CWindow::Run()
{
  // 🖼️ Create Window Description
  xwin::WindowDesc windowDesc;
  windowDesc.name = "Test";
  windowDesc.title = "CWindow Test";
  windowDesc.visible = true;
  windowDesc.width = 1280;
  windowDesc.height = 720;

  bool closed = false;

  // 🌟 Initialize
  xwin::Window window;
  xwin::EventQueue eventQueue;

  if (!window.create(windowDesc, eventQueue))
  {
    return;
  }

  bool shouldRun = true;
  while (shouldRun)
  {
    // ♻️ Update the event queue
    eventQueue.update();

    // 🎈 Iterate through that queue:
    while (!eventQueue.empty())
    {
      const xwin::Event &event = eventQueue.front();

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

    // Render
    if (FrameFunc)
    {
      FrameFunc();
    }
    else
    {
      std::cout << "FrameFunc is null!" << std::endl;
    }
  }
}

CWindow::~CWindow()
{
}