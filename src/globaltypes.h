#ifndef GLOBALTYPES_H
#define GLOBALTYPES_H

#include <thread>

extern void sleep(int);

struct PixelCoord {
  int x;
  int y;
};

struct MapCoord {
  int u;
  int v;
};

enum class Directions { None, Up, Down, Left, Right };


// struct DirOptions{
//   bool up = false;
//   bool down = false;
//   bool left = false;
//   bool right = false;
// };



#endif