#pragma once

#include "math/point2.hpp"

#include <string>

class CWindow {
private:
	//
	// Window properties
	//
	int Width				= 1600;
	int Height				= 900;
	std::string Title		= "My Window";

	//
	// Events
	//
	void (*FrameFunc)(void)	= nullptr;

public:
	Point2_t GetSize() { return Point2_t{ Width, Height }; }
	void SetSize(Point2_t size) { Width = size.x; Height = size.y; }

	void SetTitle(std::string title) { Title = title; }
	std::string GetTitle() { return Title; };

	void SetFrameFunc(void (*func)(void)) { FrameFunc = func; }
	void Run();

	CWindow();
	~CWindow();
};