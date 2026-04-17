/*
 * PROJECT COMMENT (monster.cpp)
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

#include "monster.h"


/**
 * @brief Construct a new Monster:: Monster object
 * 
 * @param _coord : the map coordinates for placing the monster
 * @param _id: A unique "monster ID" (currently not used)
 * @param _monster_char original monster type marker from the map
 * @param _movement_step_delay_ms delay per animation step in milliseconds
 */
Monster::Monster(MapCoord _coord, int _id, char _monster_char,
                 int _movement_step_delay_ms) {
  id = _id;
  // std::cout << "Creating monster #" << id << "\n";
  map_coord = _coord;
  death_coord = _coord;
  monster_char = _monster_char;
  is_alive = true;
  last_fireball_ticks = 0;
  next_gas_cloud_ticks = 0;
  death_started_ticks = 0;
  scheduled_dynamite_blast_ticks = 0;
  scheduled_dynamite_blast_coord = _coord;
  px_delta.x = 0;
  px_delta.y = 0;
  facing_direction = Directions::Down;
  movement_step_delay_ms =
      (_movement_step_delay_ms > 0) ? _movement_step_delay_ms : 1;
}

/**
 * @brief Simulation loop for monster object
 * 
 * @param events for reading keyboard input (signal for ending thread)
 * @param map for calculating the possible directions per move
 */
void Monster::simulate(Events *events, Map *map) {
  std::vector<Directions> options;
  Directions next_move;
  Directions last_move;
  std::unique_lock<std::mutex> lck(mtx);
  bool way_clear{false};
  // std::cout << "Simulating monster #" << id << "\n";
  lck.unlock();
  map->get_options(map_coord, options);
  if (options.empty()) {
    return;
  }
  next_move = options[rand() % options.size()];

  while (!events->is_quit() && is_alive) {
    // carry out next monster move ... do it with a little dirty hack to enable
    // smooth moving
    if (next_move != Directions::None) {
      facing_direction = next_move;
    }
    switch (next_move) {
    case Directions::Up:
      for (int i = 0; i > -100 && is_alive; i--) {
        px_delta.y = i;
        sleep(movement_step_delay_ms);
      }
      if (!is_alive) {
        break;
      }
      px_delta.y = 0;
      map_coord.u--;
      break;
    case Directions::Down:
      for (int i = 0; i < 100 && is_alive; i++) {
        px_delta.y = i;
        sleep(movement_step_delay_ms);
      }
      if (!is_alive) {
        break;
      }
      px_delta.y = 0;
      map_coord.u++;
      break;
    case Directions::Left:
      for (int i = 0; i > -100 && is_alive; i--) {
        px_delta.x = i;
        sleep(movement_step_delay_ms);
      }
      if (!is_alive) {
        break;
      }
      px_delta.x = 0;
      map_coord.v--;
      break;
    case Directions::Right:
      for (int i = 0; i < 100 && is_alive; i++) {
        px_delta.x = i;
        sleep(movement_step_delay_ms);
      }
      if (!is_alive) {
        break;
      }
      px_delta.x = 0;
      map_coord.v++;
    default:
      break;
    };

    if (!is_alive) {
      px_delta.x = 0;
      px_delta.y = 0;
      break;
    }

    
    map->get_options(map_coord, options);
    if (options.empty()) {
      px_delta.x = 0;
      px_delta.y = 0;
      sleep(1);
      continue;
    }
    way_clear = false;
    for (auto i : options) {
      if (i == next_move) {
        way_clear = true;
      }
    }
    last_move = next_move;
    next_move = options[rand() % options.size()];

    // Control switch to avoid monsters changing directions (e.g.
    // up->down->up->down...)
    switch (last_move) {
    case Directions::Up:
      if (next_move == Directions::Down && way_clear) {
        next_move = Directions::Up;
      }
      break;
    case Directions::Down:
      if (next_move == Directions::Up && way_clear) {
        next_move = Directions::Down;
      }
      break;
    case Directions::Left:
      if (next_move == Directions::Right && way_clear) {
        next_move = Directions::Left;
      }
      break;
    case Directions::Right:
      if (next_move == Directions::Left && way_clear) {
        next_move = Directions::Right;
      }
      break;
    default:
      break;
    }
    sleep(1);
  }
  lck.lock();
  // std::cout << "Got kill signal for monster #" << id << "\n";
  lck.unlock();
}

/**
 * @brief Destroy the Monster:: Monster object
 * 
 */
Monster::~Monster() {
  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Monster #" << id << " is being destroyed \n";
  lck.unlock();
}
