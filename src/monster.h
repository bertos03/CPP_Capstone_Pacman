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
class Pacman;

class Monster : public MapElement {
public:
  Monster(MapCoord, int, char, int);
  ~Monster();
  void simulate(Events *, Map *, Pacman *, const std::vector<Monster *> *);

protected:
private:
  std::vector<Directions> options;
  char monster_char;
  bool is_alive;
  Uint32 last_fireball_ticks;
  Uint32 next_gas_cloud_ticks;
  Uint32 death_started_ticks;
  MapCoord death_coord;
  Uint32 scheduled_dynamite_blast_ticks;
  MapCoord scheduled_dynamite_blast_coord;
  Directions facing_direction;
  int movement_step_delay_ms;
  bool is_electrified;
  Uint32 electrified_started_ticks;
  int electrified_visual_seed;
  int electrified_charge_target_id;
  Uint32 biohazard_paralyzed_until_ticks;
  Uint32 next_grazing_ticks;
  Uint32 grazing_until_ticks;
  bool goat_is_jumping;

  friend class Renderer;
  friend class Game;
};

#endif
