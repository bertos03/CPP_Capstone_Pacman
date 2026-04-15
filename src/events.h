/*
 * PROJECT COMMENT (events.h)
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

#ifndef EVENTS_H
#define EVENTS_H

#include <SDL.h>
#include <iostream>

#include "globaltypes.h"

class Events {
public:
  Events();
  ~Events();
  void update();
  void Keyreset();
  void RequestQuit();
  bool is_quit() { return quit; }
  Directions get_next_move() { return current_direction; }
  bool ConsumePlaceDynamiteRequest();

protected:
private:
  SDL_Event *sdl_events;
  bool quit;
  Directions current_direction;
  bool place_dynamite_requested;
};

#endif
