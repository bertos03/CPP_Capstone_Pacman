#include "events.h"

Events::Events() {
  sdl_events = new SDL_Event;
  quit = false;
}

Events::~Events() {
  delete sdl_events;
}

void Events::Keyreset(){
  current_direction = Directions::None;
}

void Events::update() {
  SDL_PollEvent(sdl_events);
  if (sdl_events->type == SDL_QUIT) {
    quit = true;
  }
  
  if (sdl_events->type == SDL_MOUSEBUTTONDOWN) {
    //quit = true;
  }
  if (sdl_events->type == SDL_KEYDOWN) {
    switch (sdl_events->key.keysym.sym) {
    case SDLK_UP:
      current_direction = Directions::Up;
      break;
    case SDLK_DOWN:
      current_direction = Directions::Down;
      break;
    case SDLK_LEFT:
      current_direction = Directions::Left;
      break;
    case SDLK_RIGHT:
      current_direction = Directions::Right;
      break;
    default:
      current_direction = Directions::None;
    }
  }
}