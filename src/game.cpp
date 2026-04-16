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
#include <cmath>
#include <random>

namespace {

constexpr Uint32 kFireballCooldownMs = 10000;
constexpr Uint32 kMonsterExplosionDurationMs = 900;
constexpr Uint32 kWallImpactDurationMs = 180;
constexpr Uint32 kDynamiteChainDelayMs = 140;

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

bool IsInsideMapBounds(Map *map, MapCoord coord) {
  if (map == nullptr || coord.u < 0 || coord.v < 0) {
    return false;
  }

  return coord.u < static_cast<int>(map->get_map_rows()) &&
         coord.v < static_cast<int>(map->get_map_cols());
}

bool IsOuterWallCoord(Map *map, MapCoord coord) {
  if (!IsInsideMapBounds(map, coord)) {
    return false;
  }

  const int last_row = static_cast<int>(map->get_map_rows()) - 1;
  const int last_col = static_cast<int>(map->get_map_cols()) - 1;
  return coord.u == 0 || coord.v == 0 || coord.u == last_row ||
         coord.v == last_col;
}

int DistanceSquared(MapCoord left, MapCoord right) {
  const int delta_u = left.u - right.u;
  const int delta_v = left.v - right.v;
  return delta_u * delta_u + delta_v * delta_v;
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
  dynamite_inventory = 0;
  plastic_explosive_inventory = 0;
  plastic_explosive_is_armed = false;
  simulation_started = false;
  death_started_ticks = 0;
  last_update_ticks = SDL_GetTicks();
  // create Pacman object
  pacman = new Pacman(map->get_coord_pacman());
  death_coord = pacman->map_coord;
  ScheduleNextInvulnerabilityPotionSpawn(last_update_ticks);
  ScheduleNextDynamiteSpawn(last_update_ticks);
  ScheduleNextPlasticExplosiveSpawn(last_update_ticks);

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
  UpdateScheduledMonsterBlasts(now);
  UpdateInvulnerability(now);
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
    UpdatePlacedDynamites(now);
    return;
  }

  ApplyTeleporters();
  UpdateInvulnerabilityPotion(now);
  UpdateDynamitePickup(now);
  UpdatePlasticExplosivePickup(now);
  TryPlaceDynamite(now);
  TryUsePlasticExplosive(now);
  UpdatePlacedDynamites(now);
  if (dead) {
    return;
  }

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
    audio->StopInvulnerabilityLoop();
    audio->PlayWin();
#endif
  }

  TryShootFireballs(now);
  TrySpawnGasClouds(now);
  UpdateFireballs(now);
  UpdateGasClouds(now);
  UpdateScheduledMonsterBlasts(now);
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
      if (!IsPacmanInvulnerable(now)) {
        TriggerLoss(pacman->map_coord, now);
      }
    }
  }
}

/**
 * @brief Destroy the Game:: Game object
 * 
 */
Game::~Game() {
#ifdef AUDIO
  audio->StopInvulnerabilityLoop();
#endif
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
  audio->StopInvulnerabilityLoop();
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

void Game::ScheduleNextInvulnerabilityPotionSpawn(Uint32 now) {
  invulnerability_potion.next_spawn_ticks =
      now + RandomInterval(BLUE_POTION_SPAWN_MIN_INTERVAL_MS,
                           BLUE_POTION_SPAWN_MAX_INTERVAL_MS);
}

void Game::ScheduleNextDynamiteSpawn(Uint32 now) {
  dynamite_pickup.next_spawn_ticks =
      now + RandomInterval(DYNAMITE_SPAWN_MIN_INTERVAL_MS,
                           DYNAMITE_SPAWN_MAX_INTERVAL_MS);
}

void Game::ScheduleNextPlasticExplosiveSpawn(Uint32 now) {
  plastic_explosive_pickup.next_spawn_ticks =
      now + RandomInterval(PLASTIC_EXPLOSIVE_SPAWN_MIN_INTERVAL_MS,
                           PLASTIC_EXPLOSIVE_SPAWN_MAX_INTERVAL_MS);
}

bool Game::IsWithinDynamiteRadius(MapCoord center, MapCoord target) const {
  return DistanceSquared(center, target) <=
         DYNAMITE_EXPLOSION_RADIUS_CELLS * DYNAMITE_EXPLOSION_RADIUS_CELLS;
}

bool Game::IsCellFreeForDynamitePickup(MapCoord coord) const {
  if (map == nullptr ||
      map->map_entry(static_cast<size_t>(coord.u), static_cast<size_t>(coord.v)) !=
          ElementType::TYPE_PATH) {
    return false;
  }

  if (SameCoord(coord, pacman->map_coord)) {
    return false;
  }

  if (invulnerability_potion.is_visible &&
      SameCoord(coord, invulnerability_potion.coord)) {
    return false;
  }

  if (plastic_explosive_pickup.is_visible &&
      SameCoord(coord, plastic_explosive_pickup.coord)) {
    return false;
  }

  for (const auto *monster : monsters) {
    if ((monster->is_alive ||
         monster->scheduled_dynamite_blast_ticks != 0) &&
        SameCoord(coord, monster->map_coord)) {
      return false;
    }
  }

  for (const auto &fireball : fireballs) {
    if (SameCoord(coord, fireball.current_coord)) {
      return false;
    }
  }

  for (const auto &cloud : gas_clouds) {
    if (SameCoord(coord, cloud.coord)) {
      return false;
    }
  }

  for (const auto &dynamite : placed_dynamites) {
    if (SameCoord(coord, dynamite.coord)) {
      return false;
    }
  }

  if (plastic_explosive_is_armed &&
      SameCoord(coord, placed_plastic_explosive.coord)) {
    return false;
  }

  return true;
}

bool Game::IsCellFreeForInvulnerabilityPotion(MapCoord coord) const {
  if (map == nullptr ||
      map->map_entry(static_cast<size_t>(coord.u), static_cast<size_t>(coord.v)) !=
          ElementType::TYPE_PATH) {
    return false;
  }

  if (SameCoord(coord, pacman->map_coord)) {
    return false;
  }

  for (const auto *monster : monsters) {
    if (monster->is_alive && SameCoord(coord, monster->map_coord)) {
      return false;
    }
  }

  for (const auto &fireball : fireballs) {
    if (SameCoord(coord, fireball.current_coord)) {
      return false;
    }
  }

  for (const auto &cloud : gas_clouds) {
    if (SameCoord(coord, cloud.coord)) {
      return false;
    }
  }

  if (dynamite_pickup.is_visible && SameCoord(coord, dynamite_pickup.coord)) {
    return false;
  }

  if (plastic_explosive_pickup.is_visible &&
      SameCoord(coord, plastic_explosive_pickup.coord)) {
    return false;
  }

  for (const auto &dynamite : placed_dynamites) {
    if (SameCoord(coord, dynamite.coord)) {
      return false;
    }
  }

  if (plastic_explosive_is_armed &&
      SameCoord(coord, placed_plastic_explosive.coord)) {
    return false;
  }

  return true;
}

bool Game::IsCellFreeForPlasticExplosivePickup(MapCoord coord) const {
  if (map == nullptr ||
      map->map_entry(static_cast<size_t>(coord.u), static_cast<size_t>(coord.v)) !=
          ElementType::TYPE_PATH) {
    return false;
  }

  if (SameCoord(coord, pacman->map_coord)) {
    return false;
  }

  if (invulnerability_potion.is_visible &&
      SameCoord(coord, invulnerability_potion.coord)) {
    return false;
  }

  if (dynamite_pickup.is_visible && SameCoord(coord, dynamite_pickup.coord)) {
    return false;
  }

  for (const auto *monster : monsters) {
    if ((monster->is_alive ||
         monster->scheduled_dynamite_blast_ticks != 0) &&
        SameCoord(coord, monster->map_coord)) {
      return false;
    }
  }

  for (const auto &fireball : fireballs) {
    if (SameCoord(coord, fireball.current_coord)) {
      return false;
    }
  }

  for (const auto &cloud : gas_clouds) {
    if (SameCoord(coord, cloud.coord)) {
      return false;
    }
  }

  for (const auto &dynamite : placed_dynamites) {
    if (SameCoord(coord, dynamite.coord)) {
      return false;
    }
  }

  if (plastic_explosive_is_armed &&
      SameCoord(coord, placed_plastic_explosive.coord)) {
    return false;
  }

  return true;
}

bool Game::CanPlacePlasticExplosiveAt(MapCoord coord) const {
  if (!IsInsideMapBounds(map, coord)) {
    return false;
  }

  const ElementType entry =
      map->map_entry(static_cast<size_t>(coord.u), static_cast<size_t>(coord.v));
  if (entry == ElementType::TYPE_TELEPORTER) {
    return false;
  }

  if (dynamite_pickup.is_visible && SameCoord(coord, dynamite_pickup.coord)) {
    return false;
  }
  if (invulnerability_potion.is_visible &&
      SameCoord(coord, invulnerability_potion.coord)) {
    return false;
  }
  if (plastic_explosive_pickup.is_visible &&
      SameCoord(coord, plastic_explosive_pickup.coord)) {
    return false;
  }
  for (const auto &dynamite : placed_dynamites) {
    if (SameCoord(coord, dynamite.coord)) {
      return false;
    }
  }

  return true;
}

void Game::TrySpawnInvulnerabilityPotion(Uint32 now) {
  if (invulnerability_potion.is_visible ||
      now < invulnerability_potion.next_spawn_ticks) {
    return;
  }

  std::vector<MapCoord> candidates;
  candidates.reserve(map->get_map_rows() * map->get_map_cols());
  for (size_t row = 0; row < map->get_map_rows(); row++) {
    for (size_t col = 0; col < map->get_map_cols(); col++) {
      MapCoord coord{static_cast<int>(row), static_cast<int>(col)};
      if (IsCellFreeForInvulnerabilityPotion(coord)) {
        candidates.push_back(coord);
      }
    }
  }

  if (candidates.empty()) {
    invulnerability_potion.next_spawn_ticks = now + 1000;
    return;
  }

  std::uniform_int_distribution<size_t> distribution(0, candidates.size() - 1);
  invulnerability_potion.coord = candidates[distribution(RandomGenerator())];
  invulnerability_potion.appeared_ticks = now;
  invulnerability_potion.fade_started_ticks = 0;
  invulnerability_potion.animation_seed =
      static_cast<int>((now % 997) + invulnerability_potion.coord.u * 31 +
                       invulnerability_potion.coord.v * 17);
  invulnerability_potion.is_visible = true;
  invulnerability_potion.is_fading = false;
  ScheduleNextInvulnerabilityPotionSpawn(now);
#ifdef AUDIO
  audio->PlayPotionSpawn();
#endif
}

void Game::ActivateInvulnerability(Uint32 now) {
  pacman->invulnerable_until_ticks = now + PACMAN_INVULNERABLE_MS;
#ifdef AUDIO
  audio->StartInvulnerabilityLoop();
#endif
}

void Game::TrySpawnDynamite(Uint32 now) {
  if (dynamite_pickup.is_visible || now < dynamite_pickup.next_spawn_ticks) {
    return;
  }

  std::vector<MapCoord> candidates;
  candidates.reserve(map->get_map_rows() * map->get_map_cols());
  for (size_t row = 0; row < map->get_map_rows(); row++) {
    for (size_t col = 0; col < map->get_map_cols(); col++) {
      MapCoord coord{static_cast<int>(row), static_cast<int>(col)};
      if (IsCellFreeForDynamitePickup(coord)) {
        candidates.push_back(coord);
      }
    }
  }

  if (candidates.empty()) {
    dynamite_pickup.next_spawn_ticks = now + 1000;
    return;
  }

  std::uniform_int_distribution<size_t> distribution(0, candidates.size() - 1);
  dynamite_pickup.coord = candidates[distribution(RandomGenerator())];
  dynamite_pickup.appeared_ticks = now;
  dynamite_pickup.fade_started_ticks = 0;
  dynamite_pickup.animation_seed = static_cast<int>(
      (now % 997) + dynamite_pickup.coord.u * 19 + dynamite_pickup.coord.v * 37);
  dynamite_pickup.is_visible = true;
  dynamite_pickup.is_fading = false;
  ScheduleNextDynamiteSpawn(now);
#ifdef AUDIO
  audio->PlayDynamiteSpawn();
#endif
}

void Game::UpdateDynamitePickup(Uint32 now) {
  TrySpawnDynamite(now);
  if (!dynamite_pickup.is_visible) {
    return;
  }

  if (SameCoord(pacman->map_coord, dynamite_pickup.coord)) {
    dynamite_inventory++;
    dynamite_pickup.is_visible = false;
    dynamite_pickup.is_fading = false;
    dynamite_pickup.fade_started_ticks = 0;
    return;
  }

  if (!dynamite_pickup.is_fading &&
      now - dynamite_pickup.appeared_ticks >= DYNAMITE_VISIBLE_MS) {
    dynamite_pickup.is_fading = true;
    dynamite_pickup.fade_started_ticks = now;
  }

  if (dynamite_pickup.is_fading &&
      now - dynamite_pickup.fade_started_ticks >= DYNAMITE_FADE_MS) {
    dynamite_pickup.is_visible = false;
    dynamite_pickup.is_fading = false;
    dynamite_pickup.fade_started_ticks = 0;
  }
}

void Game::TrySpawnPlasticExplosive(Uint32 now) {
  if (plastic_explosive_pickup.is_visible ||
      now < plastic_explosive_pickup.next_spawn_ticks) {
    return;
  }

  std::vector<MapCoord> candidates;
  candidates.reserve(map->get_map_rows() * map->get_map_cols());
  for (size_t row = 0; row < map->get_map_rows(); row++) {
    for (size_t col = 0; col < map->get_map_cols(); col++) {
      MapCoord coord{static_cast<int>(row), static_cast<int>(col)};
      if (IsCellFreeForPlasticExplosivePickup(coord)) {
        candidates.push_back(coord);
      }
    }
  }

  if (candidates.empty()) {
    plastic_explosive_pickup.next_spawn_ticks = now + 1000;
    return;
  }

  std::uniform_int_distribution<size_t> distribution(0, candidates.size() - 1);
  plastic_explosive_pickup.coord = candidates[distribution(RandomGenerator())];
  plastic_explosive_pickup.appeared_ticks = now;
  plastic_explosive_pickup.fade_started_ticks = 0;
  plastic_explosive_pickup.animation_seed = static_cast<int>(
      (now % 997) + plastic_explosive_pickup.coord.u * 41 +
      plastic_explosive_pickup.coord.v * 23);
  plastic_explosive_pickup.is_visible = true;
  plastic_explosive_pickup.is_fading = false;
  ScheduleNextPlasticExplosiveSpawn(now);
#ifdef AUDIO
  audio->PlayPlasticExplosiveSpawn();
#endif
}

void Game::UpdatePlasticExplosivePickup(Uint32 now) {
  TrySpawnPlasticExplosive(now);
  if (!plastic_explosive_pickup.is_visible) {
    return;
  }

  if (SameCoord(pacman->map_coord, plastic_explosive_pickup.coord)) {
    plastic_explosive_inventory++;
    plastic_explosive_pickup.is_visible = false;
    plastic_explosive_pickup.is_fading = false;
    plastic_explosive_pickup.fade_started_ticks = 0;
    return;
  }

  if (!plastic_explosive_pickup.is_fading &&
      now - plastic_explosive_pickup.appeared_ticks >=
          PLASTIC_EXPLOSIVE_VISIBLE_MS) {
    plastic_explosive_pickup.is_fading = true;
    plastic_explosive_pickup.fade_started_ticks = now;
  }

  if (plastic_explosive_pickup.is_fading &&
      now - plastic_explosive_pickup.fade_started_ticks >=
          PLASTIC_EXPLOSIVE_FADE_MS) {
    plastic_explosive_pickup.is_visible = false;
    plastic_explosive_pickup.is_fading = false;
    plastic_explosive_pickup.fade_started_ticks = 0;
  }
}

void Game::TryPlaceDynamite(Uint32 now) {
  if (events == nullptr || pacman == nullptr) {
    return;
  }

  const bool place_requested = events->ConsumePlaceDynamiteRequest();
  if (!place_requested || dynamite_inventory <= 0) {
    return;
  }

  for (const auto &placed_dynamite : placed_dynamites) {
    if (SameCoord(placed_dynamite.coord, pacman->map_coord)) {
      return;
    }
  }

  placed_dynamites.push_back(
      {pacman->map_coord, now, now + DYNAMITE_COUNTDOWN_MS,
       static_cast<int>((now % 997) + pacman->map_coord.u * 23 +
                        pacman->map_coord.v * 29 + dynamite_inventory * 17)});
  dynamite_inventory--;
#ifdef AUDIO
  audio->PlayDynamiteIgnite();
#endif
}

void Game::TryUsePlasticExplosive(Uint32 now) {
  if (events == nullptr || pacman == nullptr) {
    return;
  }

  const bool use_requested = events->ConsumePlacePlasticExplosiveRequest();
  if (!use_requested) {
    return;
  }

  if (plastic_explosive_is_armed) {
    const PlacedPlasticExplosive detonated_charge = placed_plastic_explosive;
    plastic_explosive_is_armed = false;
    DetonatePlasticExplosive(detonated_charge, now);
    return;
  }

  if (plastic_explosive_inventory <= 0) {
    return;
  }

  Directions facing_direction = pacman->facing_direction;
  if (facing_direction == Directions::None) {
    facing_direction = Directions::Down;
  }

  const MapCoord target_coord = StepCoord(pacman->map_coord, facing_direction);
  if (!CanPlacePlasticExplosiveAt(target_coord)) {
    return;
  }

  const ElementType target_entry =
      map->map_entry(static_cast<size_t>(target_coord.u),
                     static_cast<size_t>(target_coord.v));
  placed_plastic_explosive = {
      target_coord,
      now,
      static_cast<int>((now % 997) + target_coord.u * 47 + target_coord.v * 31 +
                       plastic_explosive_inventory * 19),
      target_entry == ElementType::TYPE_WALL};
  plastic_explosive_is_armed = true;
  plastic_explosive_inventory--;
#ifdef AUDIO
  audio->PlayPlasticExplosivePlace();
#endif
}

void Game::ScheduleMonsterBlast(Monster *monster, Uint32 trigger_ticks) {
  if (monster == nullptr || !monster->is_alive) {
    return;
  }

  monster->is_alive = false;
  monster->px_delta.x = 0;
  monster->px_delta.y = 0;
  monster->scheduled_dynamite_blast_ticks = trigger_ticks;
  monster->scheduled_dynamite_blast_coord = monster->map_coord;
  monster->death_coord = monster->map_coord;
  scheduled_monster_blasts.push_back(
      {monster, monster->map_coord, trigger_ticks});
}

void Game::DetonateDynamite(const PlacedDynamite &dynamite, Uint32 now) {
  effects.push_back(
      {dynamite.coord, now, EffectType::DynamiteExplosion,
       DYNAMITE_EXPLOSION_RADIUS_CELLS});
#ifdef AUDIO
  audio->PlayDynamiteExplosion();
#endif

  for (auto *monster : monsters) {
    if (!monster->is_alive ||
        !IsWithinDynamiteRadius(dynamite.coord, monster->map_coord)) {
      continue;
    }

    const int distance_steps =
        std::max(std::abs(monster->map_coord.u - dynamite.coord.u),
                 std::abs(monster->map_coord.v - dynamite.coord.v));
    const Uint32 trigger_ticks =
        now + kDynamiteChainDelayMs +
        static_cast<Uint32>(std::max(0, distance_steps) * 80);
    ScheduleMonsterBlast(monster, trigger_ticks);
  }

  if (!IsPacmanInvulnerable(now) &&
      IsWithinDynamiteRadius(dynamite.coord, pacman->map_coord)) {
    TriggerLoss(pacman->map_coord, now);
  }
}

void Game::DetonatePlasticExplosive(const PlacedPlasticExplosive &explosive,
                                    Uint32 now) {
  effects.push_back({explosive.coord, now, EffectType::DynamiteExplosion, 1});

  const bool targets_breakable_wall =
      IsInsideMapBounds(map, explosive.coord) &&
      map->map_entry(static_cast<size_t>(explosive.coord.u),
                     static_cast<size_t>(explosive.coord.v)) ==
          ElementType::TYPE_WALL &&
      !IsOuterWallCoord(map, explosive.coord);
  if (targets_breakable_wall) {
    map->SetEntry(explosive.coord, ElementType::TYPE_PATH);
#ifdef AUDIO
    audio->PlayPlasticExplosiveWallBreak();
#endif
  }

  bool eliminated_monster = false;
  for (auto *monster : monsters) {
    if (!monster->is_alive || !SameCoord(monster->map_coord, explosive.coord)) {
      continue;
    }

    EliminateMonster(monster, now);
    eliminated_monster = true;
  }

  if (!targets_breakable_wall && !eliminated_monster) {
#ifdef AUDIO
    audio->PlayMonsterExplosion();
#endif
  }

  if (!IsPacmanInvulnerable(now) &&
      SameCoord(pacman->map_coord, explosive.coord)) {
    TriggerLoss(pacman->map_coord, now);
  }
}

void Game::UpdatePlacedDynamites(Uint32 now) {
  for (size_t index = 0; index < placed_dynamites.size();) {
    if (now < placed_dynamites[index].explode_at_ticks) {
      index++;
      continue;
    }

    const PlacedDynamite detonated_dynamite = placed_dynamites[index];
    placed_dynamites.erase(placed_dynamites.begin() + static_cast<long>(index));
    DetonateDynamite(detonated_dynamite, now);
  }
}

void Game::UpdateScheduledMonsterBlasts(Uint32 now) {
  for (size_t index = 0; index < scheduled_monster_blasts.size();) {
    ScheduledMonsterBlast &blast = scheduled_monster_blasts[index];
    if (now < blast.trigger_ticks) {
      index++;
      continue;
    }

    if (blast.monster != nullptr) {
      blast.monster->scheduled_dynamite_blast_ticks = 0;
      blast.monster->death_coord = blast.coord;
      blast.monster->death_started_ticks = now;
      effects.push_back({blast.coord, now, EffectType::MonsterExplosion, 1});
#ifdef AUDIO
      audio->PlayDynamiteExplosion();
#endif
    }

    scheduled_monster_blasts.erase(scheduled_monster_blasts.begin() +
                                   static_cast<long>(index));
  }
}

void Game::UpdateInvulnerabilityPotion(Uint32 now) {
  TrySpawnInvulnerabilityPotion(now);
  if (!invulnerability_potion.is_visible) {
    return;
  }

  if (SameCoord(pacman->map_coord, invulnerability_potion.coord)) {
    ActivateInvulnerability(now);
    invulnerability_potion.is_visible = false;
    invulnerability_potion.is_fading = false;
    invulnerability_potion.fade_started_ticks = 0;
    return;
  }

  if (!invulnerability_potion.is_fading &&
      now - invulnerability_potion.appeared_ticks >= BLUE_POTION_VISIBLE_MS) {
    invulnerability_potion.is_fading = true;
    invulnerability_potion.fade_started_ticks = now;
  }

  if (invulnerability_potion.is_fading &&
      now - invulnerability_potion.fade_started_ticks >= BLUE_POTION_FADE_MS) {
    invulnerability_potion.is_visible = false;
    invulnerability_potion.is_fading = false;
    invulnerability_potion.fade_started_ticks = 0;
  }
}

void Game::UpdateInvulnerability(Uint32 now) {
  if (pacman->invulnerable_until_ticks != 0 &&
      now >= pacman->invulnerable_until_ticks) {
    pacman->invulnerable_until_ticks = 0;
#ifdef AUDIO
    audio->StopInvulnerabilityLoop();
#endif
  }
}

bool Game::IsPacmanInvulnerable(Uint32 now) const {
  return pacman != nullptr && pacman->invulnerable_until_ticks > now;
}

void Game::UpdateFireballs(Uint32 now) {
  for (size_t index = 0; index < fireballs.size();) {
    Fireball &fireball = fireballs[index];
    bool remove_fireball = false;

    while (!remove_fireball &&
           now - fireball.segment_started_ticks >= FIREBALL_STEP_DURATION_MS) {
      MapCoord next_coord = StepCoord(fireball.current_coord, fireball.direction);

      if (map->map_entry(next_coord.u, next_coord.v) == ElementType::TYPE_WALL) {
        effects.push_back({next_coord, now, EffectType::WallImpact, 0});
#ifdef AUDIO
        audio->PlayFireballWallHit();
#endif
        remove_fireball = true;
        break;
      }

      fireball.current_coord = next_coord;
      fireball.segment_started_ticks += FIREBALL_STEP_DURATION_MS;

      if (SameCoord(pacman->map_coord, fireball.current_coord)) {
#ifdef AUDIO
        audio->PlayFireballWallHit();
#endif
        if (!IsPacmanInvulnerable(now)) {
          TriggerLoss(fireball.current_coord, now);
        }
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
      if (!IsPacmanInvulnerable(now)) {
        pacman->paralyzed_until_ticks =
            std::max(pacman->paralyzed_until_ticks, now + PACMAN_PARALYSIS_MS);
        pacman->px_delta.x = 0;
        pacman->px_delta.y = 0;
#ifdef AUDIO
        audio->PlayPacmanGag();
#endif
      }
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
                       if (now < effect.started_ticks) {
                         return false;
                       }

                       Uint32 max_age = kWallImpactDurationMs;
                       if (effect.type == EffectType::MonsterExplosion) {
                         max_age = kMonsterExplosionDurationMs;
                       } else if (effect.type ==
                                  EffectType::DynamiteExplosion) {
                         max_age = DYNAMITE_EXPLOSION_DURATION_MS;
                       }
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
  monster->scheduled_dynamite_blast_ticks = 0;
  monster->death_coord = monster->map_coord;
  monster->death_started_ticks = now;
  effects.push_back(
      {monster->death_coord, now, EffectType::MonsterExplosion, 1});
#ifdef AUDIO
  audio->PlayMonsterExplosion();
#endif
}
