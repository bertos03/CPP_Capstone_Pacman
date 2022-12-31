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
  bool is_quit() { return quit; }
  Directions get_next_move(){return current_direction;}

protected:
private:
  SDL_Event *sdl_events;
  bool quit;
  Directions current_direction;
};

#endif