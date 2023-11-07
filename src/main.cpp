#include "window.hpp"
#include "gpu.hpp"

#include <iostream>

int main(int argc, const char **argv)
{
    //
    // Set up window
    //
    CWindow window;
    window.SetTitle("Hackweek WebGPU test");
    window.SetFrameFunc(Graphics::OnRender);

    //
    // Set up gpu
    //
    GraphicsDevice_t gpu(&window);
    window.SetGraphicsDevice(&gpu);

    window.Run();


    return 0;
}