/*
 * PROJECT COMMENT (goodie.h)
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

#ifndef GOODIE_H
#define GOODIE_H

#include "mapelement.h"
#include "map.h"
#include "globaltypes.h"

struct MapCoord;

/**
 * @brief The class for goodies
 *
 */
class Goodie : public MapElement {
public:
  Goodie(MapCoord, int);
  ~Goodie();
  void Deactivate();

protected:
private:
friend class Renderer;
friend class Game;
};

#endif
