#include "CrossWindow/CrossWindow.h"
#include "webgpu/webgpu.h"

#include <iostream>

void xmain(int argc, const char **argv)
{
    // 🖼️ Create Window Description
    xwin::WindowDesc windowDesc;
    windowDesc.name = "Test";
    windowDesc.title = "wgpu";
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

    // 🏁 Engine loop
    bool isRunning = true;

    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance)
    {
        std::cout << "Failed to create instance" << std::endl;
        return;
    }

    std::cout << "Created instance: " << instance << std::endl;

    while (isRunning)
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
                isRunning = false;
            }

            eventQueue.pop();
        }
    }

    wgpuInstanceRelease(instance);
}