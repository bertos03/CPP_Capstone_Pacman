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
#include "pacman.h"

#include <algorithm>
#include <limits>

namespace {

bool HasLineOfSight(Map *map, MapCoord from, MapCoord to,
                    Directions &direction_out) {
  direction_out = Directions::None;
  if (map == nullptr) {
    return false;
  }
  if (from.u == to.u && from.v == to.v) {
    return false;
  }

  if (from.u == to.u) {
    direction_out = (to.v > from.v) ? Directions::Right : Directions::Left;
    const int step = (to.v > from.v) ? 1 : -1;
    for (int col = from.v + step; col != to.v; col += step) {
      if (IsBlockingMapElement(map->map_entry(static_cast<size_t>(from.u),
                                              static_cast<size_t>(col)))) {
        return false;
      }
    }
    return true;
  }

  if (from.v == to.v) {
    direction_out = (to.u > from.u) ? Directions::Down : Directions::Up;
    const int step = (to.u > from.u) ? 1 : -1;
    for (int row = from.u + step; row != to.u; row += step) {
      if (IsBlockingMapElement(map->map_entry(static_cast<size_t>(row),
                                              static_cast<size_t>(from.v)))) {
        return false;
      }
    }
    return true;
  }

  return false;
}

Uint32 RandomTicksBetween(Uint32 minimum, Uint32 maximum) {
  if (maximum <= minimum) {
    return minimum;
  }
  return minimum + static_cast<Uint32>(rand()) % (maximum - minimum + 1);
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
  biohazard_paralyzed_until_ticks = 0;
  next_grazing_ticks = 0;
  grazing_until_ticks = 0;
  goat_is_jumping = false;
  goat_state = GoatState::Grazing;
  goat_slide_direction = Directions::None;
  goat_slide_remaining_steps = 0;
  goat_stun_until_ticks = 0;
  goat_recovery_until_ticks = 0;
  goat_ignore_player_until_ticks = 0;
  goat_love_started_ticks = 0;
  goat_love_attack_at_ticks = 0;
  goat_love_target_id = -1;
  goat_love_sequence_active = false;
  goat_pacified = false;
  goat_request_punch_sound = false;
  goat_request_bleat_sound = false;
  goat_request_crash_sound = false;
  goat_request_wall_break = false;
  goat_wall_break_coord = _coord;
}

/**
 * @brief Simulation loop for monster object
 * 
 * @param events for reading keyboard input (signal for ending thread)
 * @param map for calculating the possible directions per move
 */
void Monster::simulate(Events *events, Map *map, Pacman *pacman,
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
  auto try_find_goat_jump_direction = [&]() {
    Directions direction_out = Directions::None;
    if (monster_char != GOAT || map == nullptr || pacman == nullptr) {
      return direction_out;
    }

    const MapCoord pacman_coord = pacman->getMapCoord();
    if (pacman_coord.u == map_coord.u && pacman_coord.v == map_coord.v) {
      return direction_out;
    }
    if (!HasLineOfSight(map, map_coord, pacman_coord, direction_out)) {
      return Directions::None;
    }
    return direction_out;
  };
  auto try_find_goat_love_direction = [&]() {
    Directions direction_out = Directions::None;
    if (monster_char != GOAT || map == nullptr || all_monsters == nullptr ||
        goat_love_target_id == -1) {
      return direction_out;
    }

    Monster *target = nullptr;
    for (Monster *other : *all_monsters) {
      if (other != nullptr && other->id == goat_love_target_id &&
          other->is_alive) {
        target = other;
        break;
      }
    }
    if (target == nullptr) {
      goat_love_target_id = -1;
      return direction_out;
    }
    if (!HasLineOfSight(map, map_coord, target->map_coord, direction_out)) {
      return Directions::None;
    }
    return direction_out;
  };
  // std::cout << "Simulating monster #" << id << "\n";
  lck.unlock();
  map->get_options(map_coord, options);
  if (options.empty()) {
    return;
  }
  if (monster_char == GOAT) {
    next_grazing_ticks =
        SDL_GetTicks() + RandomTicksBetween(GOAT_GRAZING_MIN_INTERVAL_MS,
                                            GOAT_GRAZING_MAX_INTERVAL_MS);
  }
  next_move = try_find_charge_direction();
  if (next_move == Directions::None) {
    next_move = options[rand() % options.size()];
  }

  while (!events->is_quit() && is_alive) {
    const Uint32 now = SDL_GetTicks();
    if (events->IsGameplayFrozen() || now < biohazard_paralyzed_until_ticks) {
      px_delta.x = 0;
      px_delta.y = 0;
      goat_is_jumping = false;
      sleep(15);
      continue;
    }

    Directions goat_jump_direction = Directions::None;
    if (monster_char == GOAT) {
      if (goat_pacified ||
          (goat_love_sequence_active && goat_love_target_id == -1)) {
        px_delta.x = 0;
        px_delta.y = 0;
        goat_is_jumping = false;
        grazing_until_ticks = std::max(grazing_until_ticks, now + 250);
        sleep(15);
        continue;
      }

      // Stunned goat: stand still until stun expires.
      if (goat_state == GoatState::Stunned) {
        if (now >= goat_stun_until_ticks) {
          goat_state = GoatState::Grazing;
          goat_is_jumping = false;
          grazing_until_ticks = 0;
          next_grazing_ticks =
              now + RandomTicksBetween(GOAT_GRAZING_MIN_INTERVAL_MS,
                                       GOAT_GRAZING_MAX_INTERVAL_MS);
        } else {
          px_delta.x = 0;
          px_delta.y = 0;
          goat_is_jumping = false;
          sleep(15);
          continue;
        }
      }
      // Recovering: brief pause after a successful slide-stop, then bleat
      // and resume grazing.
      if (goat_state == GoatState::Recovering) {
        if (now >= goat_recovery_until_ticks) {
          goat_request_bleat_sound = true;
          goat_state = GoatState::Grazing;
          goat_is_jumping = false;
          grazing_until_ticks = 0;
          next_grazing_ticks =
              now + RandomTicksBetween(GOAT_GRAZING_MIN_INTERVAL_MS,
                                       GOAT_GRAZING_MAX_INTERVAL_MS);
        } else {
          px_delta.x = 0;
          px_delta.y = 0;
          goat_is_jumping = false;
          sleep(15);
          continue;
        }
      }

      const bool ignoring_player =
          (goat_state == GoatState::PostHitGrace) ||
          (now < goat_ignore_player_until_ticks) ||
          goat_love_target_id != -1;
      if (goat_state == GoatState::PostHitGrace &&
          now >= goat_ignore_player_until_ticks) {
        goat_state = GoatState::Grazing;
      }

      const Directions love_direction = try_find_goat_love_direction();
      const Directions sighted_direction =
          (love_direction != Directions::None)
              ? love_direction
              : (ignoring_player ? Directions::None
                                 : try_find_goat_jump_direction());

      if (goat_state == GoatState::Sliding) {
        // Continue sliding in stored direction; LOS is irrelevant now.
        goat_jump_direction = goat_slide_direction;
        goat_is_jumping = true;
      } else if (goat_state == GoatState::Charging) {
        // Already charging: do NOT track player around corners. Only stay
        // charging while the player is still on the same axis we committed
        // to. Otherwise transition to Sliding (player turned a corner OR
        // dropped out of sight); slide budget is reset so the goat slides
        // a fixed distance from this point.
        if (sighted_direction == goat_slide_direction) {
          goat_jump_direction = goat_slide_direction;
          goat_is_jumping = true;
        } else {
          goat_state = GoatState::Sliding;
          goat_slide_remaining_steps = GOAT_SLIDE_TILES;
          goat_jump_direction = goat_slide_direction;
          goat_is_jumping = true;
        }
      } else if (sighted_direction != Directions::None) {
        // Grazing/Recovering -> Charging: commit to the direction in which
        // we first spotted the player. Bleat on transition.
        goat_request_bleat_sound = true;
        goat_state = GoatState::Charging;
        goat_slide_direction = sighted_direction;
        goat_slide_remaining_steps = GOAT_SLIDE_TILES;
        grazing_until_ticks = 0;
        goat_is_jumping = true;
        goat_jump_direction = sighted_direction;
      } else {
        // Grazing.
        goat_is_jumping = false;
        if (next_grazing_ticks != 0 && now >= next_grazing_ticks) {
          const Uint32 grazing_duration =
              RandomTicksBetween(GOAT_GRAZING_MIN_DURATION_MS,
                                 GOAT_GRAZING_MAX_DURATION_MS);
          grazing_until_ticks = now + grazing_duration;
          next_grazing_ticks =
              grazing_until_ticks +
              RandomTicksBetween(GOAT_GRAZING_MIN_INTERVAL_MS,
                                 GOAT_GRAZING_MAX_INTERVAL_MS);
        }
        if (grazing_until_ticks != 0 && now < grazing_until_ticks) {
          px_delta.x = 0;
          px_delta.y = 0;
          sleep(15);
          continue;
        }
        if (grazing_until_ticks != 0 && now >= grazing_until_ticks) {
          grazing_until_ticks = 0;
        }
      }
    }

    int step_delay_ms = movement_step_delay_ms;
    if (goat_jump_direction != Directions::None) {
      // For sliding, check if next tile is a wall: if breakable -> request
      // wall break + stun and stop; if outer (unbreakable) -> stop into
      // recovery.
      if (monster_char == GOAT && goat_state == GoatState::Sliding) {
        MapCoord next_coord = map_coord;
        switch (goat_jump_direction) {
        case Directions::Up:    next_coord.u--; break;
        case Directions::Down:  next_coord.u++; break;
        case Directions::Left:  next_coord.v--; break;
        case Directions::Right: next_coord.v++; break;
        default: break;
        }
        const bool in_bounds =
            map != nullptr && next_coord.u >= 0 && next_coord.v >= 0 &&
            next_coord.u < static_cast<int>(map->get_map_rows()) &&
            next_coord.v < static_cast<int>(map->get_map_cols());
        const ElementType next_entry =
            in_bounds ? map->map_entry(static_cast<size_t>(next_coord.u),
                                       static_cast<size_t>(next_coord.v))
                      : ElementType::TYPE_WALL;
        if (next_entry == ElementType::TYPE_WALL) {
          // Crash. Game::Update will perform the actual wall break; we
          // request it here and immediately stun.
          goat_wall_break_coord = next_coord;
          goat_request_wall_break = true;
          goat_request_punch_sound = true;
          goat_request_crash_sound = true;
          goat_state = GoatState::Stunned;
          goat_stun_until_ticks = SDL_GetTicks() + GOAT_STUN_DURATION_MS;
          goat_is_jumping = false;
          px_delta.x = 0;
          px_delta.y = 0;
          sleep(15);
          continue;
        }
        // Path ahead: take a slide step and decrement counter.
        if (goat_slide_remaining_steps > 0) {
          goat_slide_remaining_steps--;
        }
        if (goat_slide_remaining_steps <= 0) {
          // We've slid the full distance and stopped on a free tile.
          goat_state = GoatState::Recovering;
          goat_recovery_until_ticks =
              SDL_GetTicks() + GOAT_RECOVERY_PAUSE_MS;
          goat_is_jumping = false;
          px_delta.x = 0;
          px_delta.y = 0;
          sleep(15);
          continue;
        }
      }
      next_move = goat_jump_direction;
      step_delay_ms = std::max(1, GOAT_JUMP_STEP_DELAY_MS);
    } else if (const Directions charge_direction = try_find_charge_direction();
               charge_direction != Directions::None) {
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
           !events->is_quit() && SDL_GetTicks() >= biohazard_paralyzed_until_ticks;
           i--) {
        px_delta.y = i;
        sleep(step_delay_ms);
      }
      if (!is_alive || events->IsGameplayFrozen() || events->is_quit() ||
          SDL_GetTicks() < biohazard_paralyzed_until_ticks) {
        break;
      }
      px_delta.y = 0;
      map_coord.u--;
      break;
    case Directions::Down:
      for (int i = 0;
           i < 100 && is_alive && !events->IsGameplayFrozen() &&
           !events->is_quit() && SDL_GetTicks() >= biohazard_paralyzed_until_ticks;
           i++) {
        px_delta.y = i;
        sleep(step_delay_ms);
      }
      if (!is_alive || events->IsGameplayFrozen() || events->is_quit() ||
          SDL_GetTicks() < biohazard_paralyzed_until_ticks) {
        break;
      }
      px_delta.y = 0;
      map_coord.u++;
      break;
    case Directions::Left:
      for (int i = 0;
           i > -100 && is_alive && !events->IsGameplayFrozen() &&
           !events->is_quit() && SDL_GetTicks() >= biohazard_paralyzed_until_ticks;
           i--) {
        px_delta.x = i;
        sleep(step_delay_ms);
      }
      if (!is_alive || events->IsGameplayFrozen() || events->is_quit() ||
          SDL_GetTicks() < biohazard_paralyzed_until_ticks) {
        break;
      }
      px_delta.x = 0;
      map_coord.v--;
      break;
    case Directions::Right:
      for (int i = 0;
           i < 100 && is_alive && !events->IsGameplayFrozen() &&
           !events->is_quit() && SDL_GetTicks() >= biohazard_paralyzed_until_ticks;
           i++) {
        px_delta.x = i;
        sleep(step_delay_ms);
      }
      if (!is_alive || events->IsGameplayFrozen() || events->is_quit() ||
          SDL_GetTicks() < biohazard_paralyzed_until_ticks) {
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

    if (SDL_GetTicks() < biohazard_paralyzed_until_ticks) {
      px_delta.x = 0;
      px_delta.y = 0;
      sleep(1);
      continue;
    }

    
    map->get_options(map_coord, options);
    if (options.empty()) {
      px_delta.x = 0;
      px_delta.y = 0;
      sleep(1);
      continue;
    }

    const Directions next_charge_direction = try_find_charge_direction();
    // Goats commit to their charge direction; do NOT let a fresh post-step
    // line-of-sight snap them around a corner. Direction changes for goats
    // happen exclusively at the top-of-loop state machine (after sliding to
    // a halt or being stunned).
    if (monster_char != GOAT) {
      const Directions next_goat_jump_direction =
          try_find_goat_jump_direction();
      if (next_goat_jump_direction != Directions::None &&
          std::find(options.begin(), options.end(), next_goat_jump_direction) !=
              options.end()) {
        next_move = next_goat_jump_direction;
        goat_is_jumping = true;
        sleep(1);
        continue;
      }
    }
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
