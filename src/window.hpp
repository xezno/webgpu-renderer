#pragma once

#include "math/point2.hpp"

#include <string>

struct GraphicsDevice_t;
struct WGPUInstance;
struct WGPUSurface;

class CWindow {
private:
	//
	// Window properties
	//
	int Width								= 1600;
	int Height								= 900;
	std::string Title						= "My Window";

	//
	// Rendering
	//
	GraphicsDevice_t* GraphicsDevice		= nullptr;

	//
	// Events
	//
	void (*FrameFunc)(GraphicsDevice_t*)	= nullptr;

	//
	// CrossWindow
	//
	xwin::Window Window;
	xwin::EventQueue EventQueue;

public:
	Point2_t GetSize() { return Point2_t{ Width, Height }; }
	void SetSize(Point2_t size) { Width = size.x; Height = size.y; }

	void SetTitle(std::string title) { Title = title; }
	std::string GetTitle() { return Title; };

	void SetFrameFunc(void (*func)(GraphicsDevice_t*)) { FrameFunc = func; }
	void Run();

	void SetGraphicsDevice(GraphicsDevice_t* graphicsDevice) { GraphicsDevice = graphicsDevice; }

	WGPUSurface* GetSurface(WGPUInstance* instance);

	CWindow();
	~CWindow();
};