/*
 * PROJECT COMMENT (events.cpp)
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

#include "events.h"

/**
 * @brief Construct a new Events:: Events object
 * Events object takes care of all keyboard inputs which are required for the
 * game
 */
Events::Events() {
  sdl_events = new SDL_Event;
  quit = false;
  current_direction = Directions::None;
  place_dynamite_requested = false;
  place_plastic_explosive_requested = false;
  airstrike_requested = false;
  rocket_requested = false;
}

/**
 * @brief Destroy the Events:: Events object
 *
 */
Events::~Events() { delete sdl_events; }

/**
 * @brief Make sure there is only one pacman move per key press
 *
 */
void Events::Keyreset() { current_direction = Directions::None; }

void Events::RequestQuit() { quit = true; }

bool Events::ConsumePlaceDynamiteRequest() {
  const bool was_requested = place_dynamite_requested;
  place_dynamite_requested = false;
  return was_requested;
}

bool Events::ConsumePlacePlasticExplosiveRequest() {
  const bool was_requested = place_plastic_explosive_requested;
  place_plastic_explosive_requested = false;
  return was_requested;
}

bool Events::ConsumeAirstrikeRequest() {
  const bool was_requested = airstrike_requested;
  airstrike_requested = false;
  return was_requested;
}

bool Events::ConsumeRocketRequest() {
  const bool was_requested = rocket_requested;
  rocket_requested = false;
  return was_requested;
}

/**
 * @brief Updates the input buffer each cycle
 *
 */
void Events::update() {
  SDL_PollEvent(sdl_events);
  if (sdl_events->type == SDL_QUIT) {
    quit = true;
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
    case SDLK_ESCAPE: // Quit the program with escape key
      quit = true;
      break;
    case SDLK_b:
      place_dynamite_requested = true;
      current_direction = Directions::None;
      break;
    case SDLK_p:
      place_plastic_explosive_requested = true;
      current_direction = Directions::None;
      break;
    case SDLK_a:
      airstrike_requested = true;
      current_direction = Directions::None;
      break;
    case SDLK_r:
      rocket_requested = true;
      current_direction = Directions::None;
      break;
    default:
      current_direction = Directions::None;
    }
  }
}
