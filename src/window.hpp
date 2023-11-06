#pragma once

#include "math/point2.hpp"

class CWindow {
private:
  int Width;
  int Height;

  void (*FrameFunc)(void);

public:
  Point2_t GetSize() { return Point2_t{Width, Height}; }
  void SetSize(Point2_t size) { Width = size.x; Height = size.y; }

  void SetFrameFunc(void (*func)(void)) { FrameFunc = func; }
  void Run();

  CWindow();
  ~CWindow();
};