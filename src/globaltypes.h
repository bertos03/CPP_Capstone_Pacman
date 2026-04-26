/*
 * PROJECT COMMENT (globaltypes.h)
 * ---------------------------------------------------------------------------
 * This file is part of "BobMan", an SDL2 game inspired by Pac-Man.
 * The code in this unit has a clear responsibility so newcomers can quickly
 * understand the architecture: data model (map, elements), runtime logic
 * (game, events), rendering (renderer), and optional audio output.
 *
 * Important notes for newcomers:
 * - Header files declare classes, methods, and data types.
 * - CPP files contain the concrete implementation of the logic.
 * - Multiple threads move game entities in parallel, so shared data is read
 *   and written in a controlled way.
 * - Macros in definitions.h control resource paths, colors, and features.
 */

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
enum class Difficulty { Training, Easy, Medium, Hard };
enum class ExtraSlot {
  None,
  Dynamite,
  PlasticExplosive,
  Airstrike,
  Rocket,
  Biohazard
};


// struct DirOptions{
//   bool up = false;
//   bool down = false;
//   bool left = false;
//   bool right = false;
// };



#endif
