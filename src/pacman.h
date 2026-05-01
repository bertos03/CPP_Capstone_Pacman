/*
 * PROJECT COMMENT (pacman.h)
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

#ifndef PACMAN_H
#define PACMAN_H

#include <SDL.h>

#include <iostream>
#include <mutex>
#include <vector>

#include "mapelement.h"
#include "events.h"
#include "map.h"
#include "renderer.h"
#include "globaltypes.h"


class Map;
class Renderer;
class Events;

/**
 * @brief The class for Pacman
 *
 */

class Pacman : public MapElement {
public:
  Pacman(MapCoord);
  ~Pacman();
  void simulate( Events *,Map *);
  MapCoord getMapCoord() const;

protected:
private:
  bool is_locked;
  Map *map;
  Uint32 paralyzed_until_ticks;
  Uint32 invulnerable_until_ticks;
  Uint32 slimed_until_ticks;
  Directions facing_direction;
  bool teleport_animation_active;
  Uint32 teleport_animation_started_ticks;
  MapCoord teleport_animation_from_coord;
  MapCoord teleport_animation_to_coord;
  bool teleport_arrival_sound_played;
  friend class Renderer;
  friend class Game;
};

#endif
