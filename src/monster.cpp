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

#include <algorithm>
#include <limits>

namespace {

bool HasLineOfSight(Map *map, MapCoord from, MapCoord to,
                    Directions &direction_out) {
  direction_out = Directions::None;
  if (map == nullptr) {
    return false;
  }

  if (from.u == to.u) {
    direction_out = (to.v > from.v) ? Directions::Right : Directions::Left;
    const int step = (to.v > from.v) ? 1 : -1;
    for (int col = from.v + step; col != to.v; col += step) {
      if (map->map_entry(static_cast<size_t>(from.u), static_cast<size_t>(col)) ==
          ElementType::TYPE_WALL) {
        return false;
      }
    }
    return true;
  }

  if (from.v == to.v) {
    direction_out = (to.u > from.u) ? Directions::Down : Directions::Up;
    const int step = (to.u > from.u) ? 1 : -1;
    for (int row = from.u + step; row != to.u; row += step) {
      if (map->map_entry(static_cast<size_t>(row), static_cast<size_t>(from.v)) ==
          ElementType::TYPE_WALL) {
        return false;
      }
    }
    return true;
  }

  return false;
}

} // namespace


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
  is_electrified = false;
  electrified_started_ticks = 0;
  electrified_visual_seed = 0;
  electrified_charge_target_id = -1;
}

/**
 * @brief Simulation loop for monster object
 * 
 * @param events for reading keyboard input (signal for ending thread)
 * @param map for calculating the possible directions per move
 */
void Monster::simulate(Events *events, Map *map,
                       const std::vector<Monster *> *all_monsters) {
  std::vector<Directions> options;
  Directions next_move;
  Directions last_move;
  std::unique_lock<std::mutex> lck(mtx);
  bool way_clear{false};
  auto try_find_charge_direction = [&]() {
    Directions direction_out = Directions::None;
    if (!is_electrified || map == nullptr || all_monsters == nullptr) {
      return direction_out;
    }

    int best_distance = std::numeric_limits<int>::max();
    for (Monster *other : *all_monsters) {
      if (other == nullptr || other == this || !other->is_alive) {
        continue;
      }

      Directions direction = Directions::None;
      if (!HasLineOfSight(map, map_coord, other->map_coord, direction)) {
        continue;
      }

      const int distance = std::abs(other->map_coord.u - map_coord.u) +
                           std::abs(other->map_coord.v - map_coord.v);
      if (distance >= best_distance) {
        continue;
      }

      best_distance = distance;
      direction_out = direction;
    }

    return direction_out;
  };
  // std::cout << "Simulating monster #" << id << "\n";
  lck.unlock();
  map->get_options(map_coord, options);
  if (options.empty()) {
    return;
  }
  next_move = try_find_charge_direction();
  if (next_move == Directions::None) {
    next_move = options[rand() % options.size()];
  }

  while (!events->is_quit() && is_alive) {
    if (events->IsGameplayFrozen()) {
      px_delta.x = 0;
      px_delta.y = 0;
      sleep(15);
      continue;
    }

    int step_delay_ms = movement_step_delay_ms;
    const Directions charge_direction = try_find_charge_direction();
    if (charge_direction != Directions::None) {
      next_move = charge_direction;
      step_delay_ms = std::max(1, BIOHAZARD_CHARGE_STEP_DELAY_MS);
    }

    // carry out next monster move ... do it with a little dirty hack to enable
    // smooth moving
    if (next_move != Directions::None) {
      facing_direction = next_move;
    }
    switch (next_move) {
    case Directions::Up:
      for (int i = 0;
           i > -100 && is_alive && !events->IsGameplayFrozen() &&
           !events->is_quit();
           i--) {
        px_delta.y = i;
        sleep(step_delay_ms);
      }
      if (!is_alive || events->IsGameplayFrozen() || events->is_quit()) {
        break;
      }
      px_delta.y = 0;
      map_coord.u--;
      break;
    case Directions::Down:
      for (int i = 0;
           i < 100 && is_alive && !events->IsGameplayFrozen() &&
           !events->is_quit();
           i++) {
        px_delta.y = i;
        sleep(step_delay_ms);
      }
      if (!is_alive || events->IsGameplayFrozen() || events->is_quit()) {
        break;
      }
      px_delta.y = 0;
      map_coord.u++;
      break;
    case Directions::Left:
      for (int i = 0;
           i > -100 && is_alive && !events->IsGameplayFrozen() &&
           !events->is_quit();
           i--) {
        px_delta.x = i;
        sleep(step_delay_ms);
      }
      if (!is_alive || events->IsGameplayFrozen() || events->is_quit()) {
        break;
      }
      px_delta.x = 0;
      map_coord.v--;
      break;
    case Directions::Right:
      for (int i = 0;
           i < 100 && is_alive && !events->IsGameplayFrozen() &&
           !events->is_quit();
           i++) {
        px_delta.x = i;
        sleep(step_delay_ms);
      }
      if (!is_alive || events->IsGameplayFrozen() || events->is_quit()) {
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

    const Directions next_charge_direction = try_find_charge_direction();
    if (next_charge_direction != Directions::None &&
        std::find(options.begin(), options.end(), next_charge_direction) !=
            options.end()) {
      next_move = next_charge_direction;
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
