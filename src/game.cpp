/*
 * PROJECT COMMENT (game.cpp)
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

#include "game.h"

#include <algorithm>
#include <random>

namespace {

constexpr Uint32 kFireballCooldownMs = 10000;
constexpr Uint32 kMonsterExplosionDurationMs = 520;
constexpr Uint32 kWallImpactDurationMs = 180;

std::mt19937 &RandomGenerator() {
  static std::mt19937 generator(std::random_device{}());
  return generator;
}

Uint32 RandomInterval(Uint32 minimum, Uint32 maximum) {
  std::uniform_int_distribution<Uint32> distribution(minimum, maximum);
  return distribution(RandomGenerator());
}

MapCoord StepCoord(MapCoord coord, Directions direction) {
  switch (direction) {
  case Directions::Up:
    coord.u--;
    break;
  case Directions::Down:
    coord.u++;
    break;
  case Directions::Left:
    coord.v--;
    break;
  case Directions::Right:
    coord.v++;
    break;
  default:
    break;
  }
  return coord;
}

bool SameCoord(MapCoord left, MapCoord right) {
  return left.u == right.u && left.v == right.v;
}

bool HasLineOfSight(Map *map, MapCoord from, MapCoord to,
                    Directions &direction_out) {
  direction_out = Directions::None;
  if (from.u == to.u) {
    direction_out = (to.v > from.v) ? Directions::Right : Directions::Left;
    const int step = (to.v > from.v) ? 1 : -1;
    for (int col = from.v + step; col != to.v; col += step) {
      if (map->map_entry(from.u, col) == ElementType::TYPE_WALL) {
        return false;
      }
    }
    return true;
  }

  if (from.v == to.v) {
    direction_out = (to.u > from.u) ? Directions::Down : Directions::Up;
    const int step = (to.u > from.u) ? 1 : -1;
    for (int row = from.u + step; row != to.u; row += step) {
      if (map->map_entry(row, from.v) == ElementType::TYPE_WALL) {
        return false;
      }
    }
    return true;
  }

  return false;
}

} // namespace

/**
 * @brief Construct a new Game:: Game object
 * 
 * @param _map: Pointer to Map object
 * @param _events: Pointer to Events object
 * @param _audio: Pointer to Audio object
 */
Game::Game(Map *_map, Events *_events, Audio *_audio)
    : map(_map), events(_events), audio(_audio) {
  win = false;
  dead = false;
  score = 0;
  simulation_started = false;
  death_started_ticks = 0;
  last_update_ticks = SDL_GetTicks();
  // create Pacman object
  pacman = new Pacman(map->get_coord_pacman());
  death_coord = pacman->map_coord;

  // create Monster objects
  for (int i = 0; i < map->get_number_monsters(); i++) {
    Monster *monster =
        new Monster(map->get_coord_monster(i), i, map->get_char_monster(i));
    if (monster->monster_char == MONSTER_MEDIUM) {
      monster->next_gas_cloud_ticks =
          last_update_ticks +
          RandomInterval(MONSTER_GAS_MIN_INTERVAL_MS,
                         MONSTER_GAS_MAX_INTERVAL_MS);
    }
    monsters.emplace_back(monster);
  }

  // create Goodie objects
  for (int i = 0; i < map->get_number_goodies(); i++) {
    goodies.emplace_back(new Goodie(map->get_coord_goodie(i), i));
  }
};

void Game::StartSimulation() {
  if (simulation_started) {
    return;
  }

  events->Keyreset();

  for (size_t i = 0; i < monsters.size(); i++) {
    monster_threads.emplace_back(&Monster::simulate, monsters[i], events, map);
  }

  pacman_thread = std::thread(&Pacman::simulate, pacman, events, map);
  simulation_started = true;
}

/**
 * @brief Game logic - is called during each cyclee
 * 
 */
void Game::Update() {
  const Uint32 now = SDL_GetTicks();
  CleanupEffects(now);
  if (pacman->teleport_animation_active &&
      !pacman->teleport_arrival_sound_played &&
      now - pacman->teleport_animation_started_ticks >=
          TELEPORT_ANIMATION_PHASE_MS) {
#ifdef AUDIO
    audio->PlayTeleporterArc();
#endif
    pacman->teleport_arrival_sound_played = true;
  }

  if (pacman->teleport_animation_active &&
      now - pacman->teleport_animation_started_ticks >=
          TELEPORT_ANIMATION_PHASE_MS * 2) {
    pacman->teleport_animation_active = false;
  }
  if (dead) {
    return;
  }

  ApplyTeleporters();

  // Check for collision with Goodies ... game is won if all goodies are
  // collected
  win = true;
  for (auto i : goodies) {
    if (i->map_coord.u == pacman->map_coord.u &&
        i->map_coord.v == pacman->map_coord.v && i->is_active) {
      score += SCORE_PER_GOODIE;
#ifdef AUDIO
      audio->PlayCoin();
#endif
      i->Deactivate();
    }
    win = (i->is_active) ? false : win;
  }
  if (win) {
#ifdef AUDIO
    audio->PlayWin();
#endif
  }

  TryShootFireballs(now);
  TrySpawnGasClouds(now);
  UpdateFireballs(now);
  UpdateGasClouds(now);
  if (dead) {
    return;
  }

  // Check for collision with Monsters ... game is lost if collision occurs
  for (auto i : monsters) {
    if (!i->is_alive) {
      continue;
    }
    if (pacman->map_coord.u == i->map_coord.u &&
        pacman->map_coord.v == i->map_coord.v) {
      TriggerLoss(pacman->map_coord, now);
    }
  }
}

/**
 * @brief Destroy the Game:: Game object
 * 
 */
Game::~Game() {
  // std::cout << "Waiting for threads to finish ... \n";
  for (size_t i = 0; i < monster_threads.size(); i++) {
    if (monster_threads[i].joinable()) {
      monster_threads[i].join();
    }
  }
  if (pacman_thread.joinable()) {
    pacman_thread.join();
  }

  // std::cout << "Threads finished.\n";

  for (auto p : monsters) {
    delete p;
  }
  for (auto p : goodies) {
    delete p;
  }
  delete pacman;
};

bool Game::is_lost() { return dead; }
bool Game::is_won() { return win; }

void Game::TriggerLoss(MapCoord coord, Uint32 now) {
  if (dead) {
    return;
  }

  death_coord = coord;
  death_started_ticks = now;
#ifdef AUDIO
  audio->PlayGameOver();
#endif
  dead = true;
}

void Game::ApplyTeleporters() {
  TryTeleportElement(pacman);
}

void Game::TryTeleportElement(MapElement *element) {
  if (element == nullptr) {
    return;
  }

  MapCoord destination;
  char teleporter_digit = '\0';
  if (!map->TryGetTeleporterDestination(element->map_coord, destination,
                                        teleporter_digit)) {
    element->active_teleporter_digit = '\0';
    return;
  }

  if (element->active_teleporter_digit == teleporter_digit) {
    return;
  }

  const MapCoord source_coord = element->map_coord;
  element->map_coord = destination;
  element->px_delta.x = 0;
  element->px_delta.y = 0;
  element->active_teleporter_digit = teleporter_digit;

  if (element == pacman) {
    pacman->teleport_animation_active = true;
    pacman->teleport_animation_started_ticks = SDL_GetTicks();
    pacman->teleport_animation_from_coord = source_coord;
    pacman->teleport_animation_to_coord = destination;
    pacman->teleport_arrival_sound_played = false;
    pacman->paralyzed_until_ticks =
        std::max(pacman->paralyzed_until_ticks,
                 pacman->teleport_animation_started_ticks +
                     TELEPORT_ANIMATION_PHASE_MS * 2);
#ifdef AUDIO
    audio->PlayTeleporterZap();
#endif
  }
}

void Game::TryShootFireballs(Uint32 now) {
  for (auto monster : monsters) {
    if (!monster->is_alive || monster->monster_char != MONSTER_MANY) {
      continue;
    }

    if (monster->last_fireball_ticks != 0 &&
        now - monster->last_fireball_ticks < kFireballCooldownMs) {
      continue;
    }

    Directions direction = Directions::None;
    if (!HasLineOfSight(map, monster->map_coord, pacman->map_coord, direction)) {
      continue;
    }

    fireballs.push_back({monster->map_coord, direction, now, monster->id});
    monster->last_fireball_ticks = now;
#ifdef AUDIO
    audio->PlayMonsterShot();
#endif
  }
}

void Game::TrySpawnGasClouds(Uint32 now) {
  for (auto monster : monsters) {
    if (!monster->is_alive || monster->monster_char != MONSTER_MEDIUM) {
      continue;
    }

    if (monster->next_gas_cloud_ticks == 0 ||
        now < monster->next_gas_cloud_ticks) {
      continue;
    }

    gas_clouds.push_back(
        {monster->map_coord, now, now + GAS_CLOUD_ACTIVE_MS,
         static_cast<int>(monster->id * 43 + now % 997), false, false});
    monster->next_gas_cloud_ticks =
        now + RandomInterval(MONSTER_GAS_MIN_INTERVAL_MS,
                             MONSTER_GAS_MAX_INTERVAL_MS);
#ifdef AUDIO
    audio->PlayMonsterFart();
#endif
  }
}

void Game::UpdateFireballs(Uint32 now) {
  for (size_t index = 0; index < fireballs.size();) {
    Fireball &fireball = fireballs[index];
    bool remove_fireball = false;

    while (!remove_fireball &&
           now - fireball.segment_started_ticks >= FIREBALL_STEP_DURATION_MS) {
      MapCoord next_coord = StepCoord(fireball.current_coord, fireball.direction);

      if (map->map_entry(next_coord.u, next_coord.v) == ElementType::TYPE_WALL) {
        effects.push_back({next_coord, now, EffectType::WallImpact});
#ifdef AUDIO
        audio->PlayFireballWallHit();
#endif
        remove_fireball = true;
        break;
      }

      fireball.current_coord = next_coord;
      fireball.segment_started_ticks += FIREBALL_STEP_DURATION_MS;

      if (SameCoord(pacman->map_coord, fireball.current_coord)) {
        TriggerLoss(fireball.current_coord, now);
        remove_fireball = true;
        break;
      }

      for (auto monster : monsters) {
        if (!monster->is_alive || monster->id == fireball.owner_id) {
          continue;
        }

        if (SameCoord(monster->map_coord, fireball.current_coord)) {
          EliminateMonster(monster, now);
          remove_fireball = true;
          break;
        }
      }
    }

    if (remove_fireball) {
      fireballs.erase(fireballs.begin() + static_cast<long>(index));
    } else {
      index++;
    }
  }
}

void Game::UpdateGasClouds(Uint32 now) {
  for (size_t index = 0; index < gas_clouds.size();) {
    GasCloud &cloud = gas_clouds[index];

    if (!cloud.is_fading && now >= cloud.fade_started_ticks) {
      cloud.is_fading = true;
    }

    if (!cloud.is_fading && SameCoord(pacman->map_coord, cloud.coord)) {
      cloud.is_fading = true;
      cloud.fade_started_ticks = now;
      cloud.triggered_by_pacman = true;
      pacman->paralyzed_until_ticks =
          std::max(pacman->paralyzed_until_ticks, now + PACMAN_PARALYSIS_MS);
      pacman->px_delta.x = 0;
      pacman->px_delta.y = 0;
#ifdef AUDIO
      audio->PlayPacmanGag();
#endif
    }

    if (cloud.is_fading &&
        now - cloud.fade_started_ticks >= GAS_CLOUD_FADE_MS) {
      gas_clouds.erase(gas_clouds.begin() + static_cast<long>(index));
    } else {
      index++;
    }
  }
}

void Game::CleanupEffects(Uint32 now) {
  effects.erase(
      std::remove_if(effects.begin(), effects.end(),
                     [now](const GameEffect &effect) {
                       const Uint32 max_age =
                           (effect.type == EffectType::MonsterExplosion)
                               ? kMonsterExplosionDurationMs
                               : kWallImpactDurationMs;
                       return now - effect.started_ticks > max_age;
                     }),
      effects.end());
}

void Game::EliminateMonster(Monster *monster, Uint32 now) {
  if (monster == nullptr || !monster->is_alive) {
    return;
  }

  monster->is_alive = false;
  monster->px_delta.x = 0;
  monster->px_delta.y = 0;
  monster->death_coord = monster->map_coord;
  monster->death_started_ticks = now;
  effects.push_back({monster->death_coord, now, EffectType::MonsterExplosion});
#ifdef AUDIO
  audio->PlayMonsterExplosion();
#endif
}
