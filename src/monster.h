/*
 * PROJECT COMMENT (monster.h)
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

#ifndef MONSTER_H
#define MONSTER_H

#include <SDL.h>

#include <mutex>
#include <thread>
#include <iostream>
#include <vector>

#include "mapelement.h"
#include "events.h"
#include "map.h"
#include "globaltypes.h"
#include "renderer.h"

struct MapCoord;
class Map;

class Monster : public MapElement {
public:
  Monster(MapCoord, int, char);
  ~Monster();
  void simulate(Events *, Map *);

protected:
private:
std::vector<Directions> options;
char monster_char;
bool is_alive;
Uint32 last_fireball_ticks;
Uint32 next_gas_cloud_ticks;
Uint32 death_started_ticks;
MapCoord death_coord;

friend class Renderer;
friend class Game;
};

#endif
