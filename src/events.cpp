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

namespace {

ExtraSlot KeycodeToExtraSlot(SDL_Keycode keycode) {
  switch (keycode) {
  case SDLK_1:
  case SDLK_KP_1:
    return ExtraSlot::Dynamite;
  case SDLK_2:
  case SDLK_KP_2:
    return ExtraSlot::PlasticExplosive;
  case SDLK_3:
  case SDLK_KP_3:
    return ExtraSlot::Airstrike;
  case SDLK_4:
  case SDLK_KP_4:
    return ExtraSlot::Rocket;
  default:
    return ExtraSlot::None;
  }
}

} // namespace

/**
 * @brief Construct a new Events:: Events object
 * Events object takes care of all keyboard inputs which are required for the
 * game
 */
Events::Events() {
  sdl_events = new SDL_Event;
  quit = false;
  current_direction = Directions::None;
  requested_extra = ExtraSlot::None;
  gameplay_frozen.store(false, std::memory_order_relaxed);
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

void Events::SetGameplayFrozen(bool frozen) {
  gameplay_frozen.store(frozen, std::memory_order_relaxed);
  if (frozen) {
    current_direction = Directions::None;
    requested_extra = ExtraSlot::None;
  }
}

bool Events::IsGameplayFrozen() const {
  return gameplay_frozen.load(std::memory_order_relaxed);
}

bool Events::ConsumeExtraUseRequest(ExtraSlot slot) {
  if (requested_extra != slot) {
    return false;
  }

  requested_extra = ExtraSlot::None;
  return true;
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
    if (IsGameplayFrozen()) {
      if (sdl_events->key.keysym.sym == SDLK_ESCAPE) {
        quit = true;
      }
      current_direction = Directions::None;
      return;
    }

    const ExtraSlot requested_slot =
        KeycodeToExtraSlot(sdl_events->key.keysym.sym);
    if (requested_slot != ExtraSlot::None) {
      requested_extra = requested_slot;
      current_direction = Directions::None;
      return;
    }

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
    default:
      current_direction = Directions::None;
    }
  }
}
