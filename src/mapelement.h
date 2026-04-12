/*
 * PROJECT COMMENT (mapelement.h)
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

#ifndef MAPELEMENT_H
#define MAPELEMENT_H

#include "globaltypes.h"
#include <mutex>

/**
 * @brief Defines the base class for dynamic elements (pacman, monsters,
 * goodies) such as Pacman, the monsters and goodies.
 *
 */
class MapElement {
public:
  /* since all derived classes have own constructors/destructors, no definition
   * of those for base class */
protected:
  PixelCoord px_coord; // Coordinates in pixelmap
  MapCoord map_coord;  // coordinates in logical map
  PixelCoord px_delta; // percentage(!) of derivation (0-100) from position
                       // (100: 1 element-size)

  bool is_active = true;
  bool locked = false;
  char active_teleporter_digit = '\0';
  static std::mutex mtx;
  int id;

private:
  friend class Game;
};

#endif
