#include "window.hpp"

#include <iostream>

void xmain(int argc, const char **argv)
{
    CWindow window;
    window.SetTitle("Hackweek WebGPU test");
    window.Run();
}