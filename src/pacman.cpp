/*
 * PROJECT COMMENT (pacman.cpp)
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

#include "pacman.h"

/**
 * @brief Construct a new Pacman:: Pacman object
 * 
 * @param _coord map coordinates for placing pacman
 */
Pacman::Pacman(MapCoord _coord) {
  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Creating Pacman\n";
  lck.unlock();
  map_coord = _coord;
  px_delta.x = 0;
  px_delta.y = 0;
  paralyzed_until_ticks = 0;
  invulnerable_until_ticks = 0;
  slimed_until_ticks = 0;
  facing_direction = Directions::Down;
  teleport_animation_active = false;
  teleport_animation_started_ticks = 0;
  teleport_animation_from_coord = _coord;
  teleport_animation_to_coord = _coord;
  teleport_arrival_sound_played = false;
};

/**
 * @brief Destroy the Pacman:: Pacman object
 * 
 */
Pacman::~Pacman() {
  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Pacman is being destroyed \n";
  lck.unlock();
};

/**
 * @brief The simulation loop for pacman
 * 
 * @param events Events object to read keyboard inputs
 * @param map  Map object to check the possible options for pacman
 */
void Pacman::simulate(Events *events, Map *map) {
  std::vector<Directions> options;
  bool valid_choice = false;
  Directions next_move;

  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Simulating Pacman\n";
  lck.unlock();

  while (!events->is_quit()) {
    if (events->IsGameplayFrozen()) {
      px_delta.x = 0;
      px_delta.y = 0;
      sleep(15);
      continue;
    }

    if (SDL_GetTicks() < paralyzed_until_ticks) {
      px_delta.x = 0;
      px_delta.y = 0;
      events->Keyreset();
      sleep(15);
      continue;
    }

    next_move = events->get_next_move();
    map->get_options(map_coord, options);
    events->Keyreset();
    sleep(5);
	    for (auto i : options) {
	      if (i == next_move) {
        facing_direction = next_move;
	        switch (next_move) {
        case Directions::Up:
          for (int i = 0;
               i > -100 && SDL_GetTicks() >= paralyzed_until_ticks &&
               !events->IsGameplayFrozen() && !events->is_quit();
               i--) {
            px_delta.y = i;
            sleep(10 - SPEED_PACMAN);
          }
          if (events->IsGameplayFrozen() || events->is_quit() ||
              SDL_GetTicks() < paralyzed_until_ticks) {
            px_delta.y = 0;
            break;
          }
          px_delta.y = 0;
          map_coord.u--;
          break;
        case Directions::Down:
          for (int i = 0;
               i < 100 && SDL_GetTicks() >= paralyzed_until_ticks &&
               !events->IsGameplayFrozen() && !events->is_quit();
               i++) {
            px_delta.y = i;
            sleep(10 - SPEED_PACMAN);
          }
          if (events->IsGameplayFrozen() || events->is_quit() ||
              SDL_GetTicks() < paralyzed_until_ticks) {
            px_delta.y = 0;
            break;
          }
          px_delta.y = 0;
          map_coord.u++;
          break;
        case Directions::Left:
          for (int i = 0;
               i > -100 && SDL_GetTicks() >= paralyzed_until_ticks &&
               !events->IsGameplayFrozen() && !events->is_quit();
               i--) {
            px_delta.x = i;
            sleep(10 - SPEED_PACMAN);
          }
          if (events->IsGameplayFrozen() || events->is_quit() ||
              SDL_GetTicks() < paralyzed_until_ticks) {
            px_delta.x = 0;
            break;
          }
          px_delta.x = 0;
          map_coord.v--;
          break;
        case Directions::Right:
          for (int i = 0;
               i < 100 && SDL_GetTicks() >= paralyzed_until_ticks &&
               !events->IsGameplayFrozen() && !events->is_quit();
               i++) {
            px_delta.x = i;
            sleep(10 - SPEED_PACMAN);
          }
          if (events->IsGameplayFrozen() || events->is_quit() ||
              SDL_GetTicks() < paralyzed_until_ticks) {
            px_delta.x = 0;
            break;
          }
          px_delta.x = 0;
          map_coord.v++;
        default:
          break;
        };
      }
    }
  }
}
