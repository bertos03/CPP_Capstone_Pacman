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
#include <array>
#include <cmath>
#include <limits>
#include <random>

namespace {

constexpr Uint32 kMonsterExplosionDurationMs = MONSTER_EXPLOSION_DURATION_MS;
constexpr Uint32 kWallImpactDurationMs = 180;
constexpr Uint32 kBiohazardHitSequenceDurationMs = BIOHAZARD_HIT_SEQUENCE_MS;
constexpr Uint32 kBiohazardImpactFlashDurationMs =
    BIOHAZARD_IMPACT_FLASH_DURATION_MS;
constexpr Uint32 kSlimeSplashDurationMs =
    SLIME_SPLASH_FRAME_MS * SLIME_SPLASH_FRAME_COUNT + SLIME_SPLASH_FADE_MS;
constexpr Uint32 kPlasmaShockwaveDurationMs = PLASMA_SHOCKWAVE_DURATION_MS;
constexpr Uint32 kDynamiteChainDelayMs = 140;
constexpr double kPlasticExplosiveMonsterHitRadiusCells = 0.72;
constexpr float kAirstrikePlaneMarginCells = 2.0f;
constexpr float kAirstrikePathSamplesPerCell = 14.0f;

struct AirstrikePathCandidate {
  MapCoord coord{0, 0};
  SDL_FPoint point{0.0f, 0.0f};
  double progress = 0.0;
};

struct DifficultyTuning {
  int monster_speed_rating;
  double monster_step_delay_scale;
  Uint32 fireball_cooldown_ms;
  Uint32 fireball_step_duration_ms;
  Uint32 pickup_visible_ms;
  double extra_spawn_interval_scale;
};

std::mt19937 &RandomGenerator() {
  static std::mt19937 generator(std::random_device{}());
  return generator;
}

Uint32 RandomInterval(Uint32 minimum, Uint32 maximum) {
  std::uniform_int_distribution<Uint32> distribution(minimum, maximum);
  return distribution(RandomGenerator());
}

void ShiftActiveTicks(Uint32 &ticks, Uint32 delta_ms) {
  if (ticks != 0) {
    ticks += delta_ms;
  }
}

DifficultyTuning GetDifficultyTuning(Difficulty difficulty) {
  switch (difficulty) {
  case Difficulty::Training:
    return {std::max(1, SPEED_MONSTER - 4), 1.5, 15000, 330, 60000, 0.75};
  case Difficulty::Easy:
    return {std::max(1, SPEED_MONSTER - 3), 1.0, 15000, 220, 60000, 0.75};
  case Difficulty::Hard:
    return {std::max(1, SPEED_MONSTER), 1.0, 6500, 120, 20000, 1.4};
  case Difficulty::Medium:
  default:
    return {std::max(1, SPEED_MONSTER - 2), 1.0, 10000, 160, 40000, 1.0};
  }
}

Uint32 ScaleInterval(Uint32 base_value, double scale) {
  return std::max<Uint32>(
      1000, static_cast<Uint32>(std::lround(base_value * scale)));
}

Uint32 RandomScaledInterval(Uint32 minimum, Uint32 maximum, double scale) {
  const Uint32 scaled_minimum = ScaleInterval(minimum, scale);
  const Uint32 scaled_maximum =
      std::max(scaled_minimum, ScaleInterval(maximum, scale));
  return RandomInterval(scaled_minimum, scaled_maximum);
}

int GetMonsterMovementStepDelayMs(Difficulty difficulty) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  const double base_delay_ms = std::max(1, 10 - tuning.monster_speed_rating);
  return std::max(
      1, static_cast<int>(std::ceil(base_delay_ms * tuning.monster_step_delay_scale)));
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

SDL_FPoint MakeCellCenter(MapCoord coord) {
  return SDL_FPoint{static_cast<float>(coord.v) + 0.5f,
                    static_cast<float>(coord.u) + 0.5f};
}

SDL_FPoint AddPoint(SDL_FPoint left, SDL_FPoint right) {
  return SDL_FPoint{left.x + right.x, left.y + right.y};
}

SDL_FPoint SubtractPoint(SDL_FPoint left, SDL_FPoint right) {
  return SDL_FPoint{left.x - right.x, left.y - right.y};
}

SDL_FPoint ScalePoint(SDL_FPoint point, float factor) {
  return SDL_FPoint{point.x * factor, point.y * factor};
}

double PointDistance(SDL_FPoint left, SDL_FPoint right) {
  return std::hypot(static_cast<double>(left.x - right.x),
                    static_cast<double>(left.y - right.y));
}

SDL_FPoint LerpPoint(SDL_FPoint start, SDL_FPoint end, double progress) {
  const float clamped_progress =
      static_cast<float>(std::clamp(progress, 0.0, 1.0));
  return SDL_FPoint{
      start.x + (end.x - start.x) * clamped_progress,
      start.y + (end.y - start.y) * clamped_progress};
}

Directions DominantDirection(SDL_FPoint vector) {
  if (std::fabs(vector.x) >= std::fabs(vector.y)) {
    return (vector.x >= 0.0f) ? Directions::Right : Directions::Left;
  }

  return (vector.y >= 0.0f) ? Directions::Down : Directions::Up;
}

SDL_FPoint RandomAirstrikeStart(Map *map) {
  if (map == nullptr) {
    return SDL_FPoint{0.0f, 0.0f};
  }

  const float rows = static_cast<float>(map->get_map_rows());
  const float cols = static_cast<float>(map->get_map_cols());
  const float min_x = 0.5f;
  const float max_x = std::max(min_x, cols - 0.5f);
  const float min_y = 0.5f;
  const float max_y = std::max(min_y, rows - 0.5f);
  std::uniform_real_distribution<float> x_distribution(min_x, max_x);
  std::uniform_real_distribution<float> y_distribution(min_y, max_y);
  std::uniform_int_distribution<int> distribution(0, 3);

  switch (distribution(RandomGenerator())) {
  case 0:
    return SDL_FPoint{-kAirstrikePlaneMarginCells, y_distribution(RandomGenerator())};
  case 1:
    return SDL_FPoint{cols + kAirstrikePlaneMarginCells,
                      y_distribution(RandomGenerator())};
  case 2:
    return SDL_FPoint{x_distribution(RandomGenerator()), -kAirstrikePlaneMarginCells};
  case 3:
  default:
    return SDL_FPoint{x_distribution(RandomGenerator()),
                      rows + kAirstrikePlaneMarginCells};
  }
}

std::vector<AirstrikePathCandidate>
CollectAirstrikePathCandidates(Map *map, SDL_FPoint start, SDL_FPoint end,
                               MapCoord excluded_coord) {
  std::vector<AirstrikePathCandidate> candidates;
  if (map == nullptr) {
    return candidates;
  }

  const int rows = static_cast<int>(map->get_map_rows());
  const int cols = static_cast<int>(map->get_map_cols());
  if (rows <= 0 || cols <= 0) {
    return candidates;
  }

  std::vector<bool> visited(static_cast<size_t>(rows * cols), false);
  const double path_length = PointDistance(start, end);
  const int sample_count = std::max(
      1, static_cast<int>(std::ceil(path_length * kAirstrikePathSamplesPerCell)));
  for (int sample = 0; sample <= sample_count; ++sample) {
    const double progress =
        static_cast<double>(sample) / static_cast<double>(sample_count);
    const SDL_FPoint point = LerpPoint(start, end, progress);
    const MapCoord coord{static_cast<int>(std::floor(point.y)),
                         static_cast<int>(std::floor(point.x))};
    if (!IsInsideMapBounds(map, coord) || SameCoord(coord, excluded_coord)) {
      continue;
    }

    const size_t visited_index =
        static_cast<size_t>(coord.u * cols + coord.v);
    if (visited[visited_index]) {
      continue;
    }

    visited[visited_index] = true;
    candidates.push_back({coord, point, progress});
  }

  return candidates;
}

} // namespace

/**
 * @brief Construct a new Game:: Game object
 * 
 * @param _map: Pointer to Map object
 * @param _events: Pointer to Events object
 * @param _audio: Pointer to Audio object
 * @param _starting_lives Configured number of lives for a new run
 */
Game::Game(Map *_map, Events *_events, Audio *_audio, Difficulty _difficulty,
           int _starting_lives)
    : map(_map), events(_events), audio(_audio), difficulty(_difficulty) {
  win = false;
  dead = false;
  starting_lives = std::clamp(_starting_lives, PLAYER_LIVES_MIN, PLAYER_LIVES_MAX);
  remaining_lives = starting_lives;
  score = 0;
  dynamite_inventory = 0;
  plastic_explosive_inventory = 0;
  airstrike_inventory = 0;
  rocket_inventory = 0;
  biohazard_inventory = 0;
  plastic_explosive_is_armed = false;
  active_airstrike = {};
  active_biohazard_beam = {};
  simulation_started = false;
  death_started_ticks = 0;
  life_recovery_until_ticks = 0;
  final_loss_commits_at_ticks = 0;
  last_update_ticks = SDL_GetTicks();
  // create Pacman object
  pacman = new Pacman(map->get_coord_pacman());
  death_coord = pacman->map_coord;
  ScheduleNextInvulnerabilityPotionSpawn(last_update_ticks);
  ScheduleNextDynamiteSpawn(last_update_ticks);
  ScheduleNextPlasticExplosiveSpawn(last_update_ticks);
  ScheduleNextWalkieTalkieSpawn(last_update_ticks);
  ScheduleNextRocketSpawn(last_update_ticks);
  ScheduleNextBiohazardSpawn(last_update_ticks);

  // create Monster objects
  for (int i = 0; i < map->get_number_monsters(); i++) {
    Monster *monster = new Monster(map->get_coord_monster(i), i,
                                   map->get_char_monster(i),
                                   GetMonsterMovementStepDelayMs(difficulty));
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

  events->SetGameplayFrozen(false);
  events->Keyreset();

  for (size_t i = 0; i < monsters.size(); i++) {
    monster_threads.emplace_back(&Monster::simulate, monsters[i], events, map,
                                 &monsters);
  }

  pacman_thread = std::thread(&Pacman::simulate, pacman, events, map);
  simulation_started = true;
}

void Game::PauseSimulation() {
  if (!simulation_started || events == nullptr) {
    return;
  }

  events->SetGameplayFrozen(true);
  events->Keyreset();
}

void Game::ResumeSimulation(Uint32 paused_duration_ms) {
  if (!simulation_started || events == nullptr) {
    return;
  }

  ShiftPausedTimers(paused_duration_ms);
  events->SetGameplayFrozen(false);
  events->Keyreset();
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
  UpdateFinalLossSequence(now);
  if (IsFinalLossSequenceActive(now)) {
    return;
  }
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
    UpdateAirstrike(now);
    UpdatePlacedDynamites(now);
    UpdateRockets(now);
    return;
  }

  ApplyTeleporters();
  UpdateInvulnerabilityPotion(now);
  UpdateDynamitePickup(now);
  UpdatePlasticExplosivePickup(now);
  UpdateWalkieTalkiePickup(now);
  UpdateRocketPickup(now);
  UpdateBiohazardPickup(now);
  TryUseBiohazardBeam(now);
  UpdateBiohazardBeam(now);
  TryUseAirstrike(now);
  TryPlaceDynamite(now);
  TryUsePlasticExplosive(now);
  TryFireRocket(now);
  UpdateAirstrike(now);
  UpdatePlacedDynamites(now);
  UpdateRockets(now);
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
    StopBiohazardBeam();
#ifdef AUDIO
    audio->StopInvulnerabilityLoop();
    audio->PlayWin();
#endif
  }

  TryShootFireballs(now);
  TryShootSlimeballs(now);
  TrySpawnGasClouds(now);
  UpdateFireballs(now);
  UpdateSlimeballs(now);
  UpdateGasClouds(now);
  UpdateElectrifiedMonsters(now);
  UpdateScheduledMonsterBlasts(now);
  if (dead) {
    return;
  }

  // Check for collision with Monsters ... game is lost if collision occurs
  for (auto i : monsters) {
    if (!i->is_alive || i->is_electrified) {
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

void Game::ShiftPausedTimers(Uint32 paused_duration_ms) {
  if (paused_duration_ms == 0) {
    return;
  }

  ShiftActiveTicks(last_update_ticks, paused_duration_ms);
  ShiftActiveTicks(death_started_ticks, paused_duration_ms);
  ShiftActiveTicks(life_recovery_until_ticks, paused_duration_ms);
  ShiftActiveTicks(final_loss_commits_at_ticks, paused_duration_ms);

  if (pacman != nullptr) {
    ShiftActiveTicks(pacman->paralyzed_until_ticks, paused_duration_ms);
    ShiftActiveTicks(pacman->invulnerable_until_ticks, paused_duration_ms);
    ShiftActiveTicks(pacman->slimed_until_ticks, paused_duration_ms);
    ShiftActiveTicks(pacman->teleport_animation_started_ticks,
                     paused_duration_ms);
  }

  for (Monster *monster : monsters) {
    if (monster == nullptr) {
      continue;
    }
    ShiftActiveTicks(monster->last_fireball_ticks, paused_duration_ms);
    ShiftActiveTicks(monster->next_gas_cloud_ticks, paused_duration_ms);
    ShiftActiveTicks(monster->death_started_ticks, paused_duration_ms);
    ShiftActiveTicks(monster->scheduled_dynamite_blast_ticks,
                     paused_duration_ms);
    ShiftActiveTicks(monster->electrified_started_ticks, paused_duration_ms);
    ShiftActiveTicks(monster->biohazard_paralyzed_until_ticks,
                     paused_duration_ms);
  }

  for (Fireball &fireball : fireballs) {
    ShiftActiveTicks(fireball.segment_started_ticks, paused_duration_ms);
  }
  for (Slimeball &slimeball : slimeballs) {
    ShiftActiveTicks(slimeball.segment_started_ticks, paused_duration_ms);
  }
  for (GasCloud &cloud : gas_clouds) {
    ShiftActiveTicks(cloud.started_ticks, paused_duration_ms);
    ShiftActiveTicks(cloud.fade_started_ticks, paused_duration_ms);
  }

  ShiftActiveTicks(invulnerability_potion.appeared_ticks, paused_duration_ms);
  ShiftActiveTicks(invulnerability_potion.fade_started_ticks, paused_duration_ms);
  ShiftActiveTicks(invulnerability_potion.next_spawn_ticks, paused_duration_ms);

  ShiftActiveTicks(dynamite_pickup.appeared_ticks, paused_duration_ms);
  ShiftActiveTicks(dynamite_pickup.fade_started_ticks, paused_duration_ms);
  ShiftActiveTicks(dynamite_pickup.next_spawn_ticks, paused_duration_ms);

  ShiftActiveTicks(plastic_explosive_pickup.appeared_ticks, paused_duration_ms);
  ShiftActiveTicks(plastic_explosive_pickup.fade_started_ticks,
                   paused_duration_ms);
  ShiftActiveTicks(plastic_explosive_pickup.next_spawn_ticks, paused_duration_ms);

  ShiftActiveTicks(walkie_talkie_pickup.appeared_ticks, paused_duration_ms);
  ShiftActiveTicks(walkie_talkie_pickup.fade_started_ticks, paused_duration_ms);
  ShiftActiveTicks(walkie_talkie_pickup.next_spawn_ticks, paused_duration_ms);

  ShiftActiveTicks(rocket_pickup.appeared_ticks, paused_duration_ms);
  ShiftActiveTicks(rocket_pickup.fade_started_ticks, paused_duration_ms);
  ShiftActiveTicks(rocket_pickup.next_spawn_ticks, paused_duration_ms);

  ShiftActiveTicks(biohazard_pickup.appeared_ticks, paused_duration_ms);
  ShiftActiveTicks(biohazard_pickup.fade_started_ticks, paused_duration_ms);
  ShiftActiveTicks(biohazard_pickup.next_spawn_ticks, paused_duration_ms);

  for (PlacedDynamite &placed_dynamite : placed_dynamites) {
    ShiftActiveTicks(placed_dynamite.placed_ticks, paused_duration_ms);
    ShiftActiveTicks(placed_dynamite.explode_at_ticks, paused_duration_ms);
  }

  ShiftActiveTicks(placed_plastic_explosive.placed_ticks, paused_duration_ms);

  ShiftActiveTicks(active_airstrike.started_ticks, paused_duration_ms);
  ShiftActiveTicks(active_airstrike.plane_launch_ticks, paused_duration_ms);
  ShiftActiveTicks(active_airstrike.overflight_ticks, paused_duration_ms);
  ShiftActiveTicks(active_airstrike.plane_finished_ticks, paused_duration_ms);
  ShiftActiveTicks(active_airstrike.finished_ticks, paused_duration_ms);
  for (AirstrikeBomb &bomb : active_airstrike.bombs) {
    ShiftActiveTicks(bomb.release_ticks, paused_duration_ms);
    ShiftActiveTicks(bomb.explode_ticks, paused_duration_ms);
  }

  for (RocketProjectile &rocket : active_rockets) {
    ShiftActiveTicks(rocket.segment_started_ticks, paused_duration_ms);
  }

  ShiftActiveTicks(active_biohazard_beam.started_ticks, paused_duration_ms);
  ShiftActiveTicks(active_biohazard_beam.minimum_visible_until_ticks,
                   paused_duration_ms);
  ShiftActiveTicks(active_biohazard_beam.hit_sequence_started_ticks,
                   paused_duration_ms);
  ShiftActiveTicks(active_biohazard_beam.locked_until_ticks,
                   paused_duration_ms);

  for (ScheduledMonsterBlast &blast : scheduled_monster_blasts) {
    ShiftActiveTicks(blast.trigger_ticks, paused_duration_ms);
  }

  for (GameEffect &effect : effects) {
    ShiftActiveTicks(effect.started_ticks, paused_duration_ms);
  }
}

/**
 * @brief Destroy the Game:: Game object
 * 
 */
Game::~Game() {
#ifdef AUDIO
  audio->StopBiohazardBeam();
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

bool Game::IsFinalLossSequenceActive(Uint32 now) const {
  return !dead && final_loss_commits_at_ticks != 0 &&
         now < final_loss_commits_at_ticks;
}

Uint32 Game::GetDeathStartedTicks() const { return death_started_ticks; }

void Game::TriggerLoss(MapCoord coord, Uint32 now) {
  if (dead || IsFinalLossSequenceActive(now)) {
    return;
  }

  if (remaining_lives > 1) {
    ActivateLifeRecovery(now);
#ifdef AUDIO
    audio->PlayLifeLost();
#endif
    return;
  }

  death_coord = coord;
  death_started_ticks = now;
  final_loss_commits_at_ticks = now + FINAL_LOSS_SOUND_LEAD_IN_MS;
  pacman->px_delta.x = 0;
  pacman->px_delta.y = 0;
  pacman->teleport_animation_active = false;
  pacman->teleport_animation_started_ticks = 0;
  pacman->teleport_animation_from_coord = pacman->map_coord;
  pacman->teleport_animation_to_coord = pacman->map_coord;
  pacman->teleport_arrival_sound_played = false;
  if (events != nullptr) {
    events->SetGameplayFrozen(true);
    events->Keyreset();
  }
  StopBiohazardBeam();
#ifdef AUDIO
  audio->StopInvulnerabilityLoop();
  audio->PlayGameOver();
#endif
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
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  for (auto monster : monsters) {
    if (!monster->is_alive || monster->is_electrified ||
        monster->biohazard_paralyzed_until_ticks > now ||
        monster->monster_char != MONSTER_MANY) {
      continue;
    }

    if (monster->last_fireball_ticks != 0 &&
        now - monster->last_fireball_ticks < tuning.fireball_cooldown_ms) {
      continue;
    }

    Directions direction = Directions::None;
    if (!HasLineOfSight(map, monster->map_coord, pacman->map_coord, direction)) {
      continue;
    }

    fireballs.push_back(
        {monster->map_coord, direction, now, monster->id,
         tuning.fireball_step_duration_ms});
    monster->last_fireball_ticks = now;
#ifdef AUDIO
    audio->PlayMonsterShot();
#endif
  }
}

void Game::TryShootSlimeballs(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  for (auto monster : monsters) {
    if (!monster->is_alive || monster->is_electrified ||
        monster->biohazard_paralyzed_until_ticks > now ||
        monster->monster_char != MONSTER_EXTRA) {
      continue;
    }

    if (monster->last_fireball_ticks != 0 &&
        now - monster->last_fireball_ticks < tuning.fireball_cooldown_ms) {
      continue;
    }

    Directions direction = Directions::None;
    if (!HasLineOfSight(map, monster->map_coord, pacman->map_coord, direction)) {
      continue;
    }

    slimeballs.push_back(
        {monster->map_coord, direction, now, monster->id,
         tuning.fireball_step_duration_ms});
    monster->last_fireball_ticks = now;
#ifdef AUDIO
    audio->PlaySlimeShot();
#endif
  }
}

void Game::TrySpawnGasClouds(Uint32 now) {
  for (auto monster : monsters) {
    if (!monster->is_alive || monster->is_electrified ||
        monster->biohazard_paralyzed_until_ticks > now ||
        monster->monster_char != MONSTER_MEDIUM) {
      continue;
    }

    if (monster->next_gas_cloud_ticks == 0 ||
        now < monster->next_gas_cloud_ticks) {
      continue;
    }

    gas_clouds.push_back(
        {monster->map_coord, now, now + GAS_CLOUD_ACTIVE_MS,
         static_cast<int>(monster->id * 43 + now % 997), monster->id, false,
         false});
    monster->next_gas_cloud_ticks =
        now + RandomInterval(MONSTER_GAS_MIN_INTERVAL_MS,
                             MONSTER_GAS_MAX_INTERVAL_MS);
#ifdef AUDIO
    audio->PlayMonsterFart();
#endif
  }
}

void Game::ScheduleNextInvulnerabilityPotionSpawn(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  invulnerability_potion.next_spawn_ticks =
      now + RandomScaledInterval(BLUE_POTION_SPAWN_MIN_INTERVAL_MS,
                                 BLUE_POTION_SPAWN_MAX_INTERVAL_MS,
                                 tuning.extra_spawn_interval_scale);
}

void Game::ScheduleNextDynamiteSpawn(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  dynamite_pickup.next_spawn_ticks =
      now + RandomScaledInterval(DYNAMITE_SPAWN_MIN_INTERVAL_MS,
                                 DYNAMITE_SPAWN_MAX_INTERVAL_MS,
                                 tuning.extra_spawn_interval_scale);
}

void Game::ScheduleNextPlasticExplosiveSpawn(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  plastic_explosive_pickup.next_spawn_ticks =
      now + RandomScaledInterval(PLASTIC_EXPLOSIVE_SPAWN_MIN_INTERVAL_MS,
                                 PLASTIC_EXPLOSIVE_SPAWN_MAX_INTERVAL_MS,
                                 tuning.extra_spawn_interval_scale);
}

void Game::ScheduleNextWalkieTalkieSpawn(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  walkie_talkie_pickup.next_spawn_ticks =
      now + RandomScaledInterval(WALKIE_TALKIE_SPAWN_MIN_INTERVAL_MS,
                                 WALKIE_TALKIE_SPAWN_MAX_INTERVAL_MS,
                                 tuning.extra_spawn_interval_scale);
}

void Game::ScheduleNextRocketSpawn(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  rocket_pickup.next_spawn_ticks =
      now + RandomScaledInterval(ROCKET_SPAWN_MIN_INTERVAL_MS,
                                 ROCKET_SPAWN_MAX_INTERVAL_MS,
                                 tuning.extra_spawn_interval_scale);
}

void Game::ScheduleNextBiohazardSpawn(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  biohazard_pickup.next_spawn_ticks =
      now + RandomScaledInterval(BIOHAZARD_SPAWN_MIN_INTERVAL_MS,
                                 BIOHAZARD_SPAWN_MAX_INTERVAL_MS,
                                 tuning.extra_spawn_interval_scale);
}

bool Game::IsWithinRadius(MapCoord center, MapCoord target,
                          int radius_cells) const {
  const int clamped_radius = std::max(0, radius_cells);
  return DistanceSquared(center, target) <= clamped_radius * clamped_radius;
}

bool Game::IsWithinDynamiteRadius(MapCoord center, MapCoord target) const {
  return IsWithinRadius(center, target, DYNAMITE_EXPLOSION_RADIUS_CELLS);
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

  if (walkie_talkie_pickup.is_visible &&
      SameCoord(coord, walkie_talkie_pickup.coord)) {
    return false;
  }

  if (rocket_pickup.is_visible && SameCoord(coord, rocket_pickup.coord)) {
    return false;
  }

  if (biohazard_pickup.is_visible && SameCoord(coord, biohazard_pickup.coord)) {
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

  for (const auto &slimeball : slimeballs) {
    if (SameCoord(coord, slimeball.current_coord)) {
      return false;
    }
  }

  for (const auto &rocket : active_rockets) {
    if (SameCoord(coord, rocket.current_coord)) {
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

  for (const auto &slimeball : slimeballs) {
    if (SameCoord(coord, slimeball.current_coord)) {
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

  if (walkie_talkie_pickup.is_visible &&
      SameCoord(coord, walkie_talkie_pickup.coord)) {
    return false;
  }

  if (rocket_pickup.is_visible && SameCoord(coord, rocket_pickup.coord)) {
    return false;
  }

  if (biohazard_pickup.is_visible && SameCoord(coord, biohazard_pickup.coord)) {
    return false;
  }

  for (const auto &dynamite : placed_dynamites) {
    if (SameCoord(coord, dynamite.coord)) {
      return false;
    }
  }

  for (const auto &rocket : active_rockets) {
    if (SameCoord(coord, rocket.current_coord)) {
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

  if (walkie_talkie_pickup.is_visible &&
      SameCoord(coord, walkie_talkie_pickup.coord)) {
    return false;
  }

  if (rocket_pickup.is_visible && SameCoord(coord, rocket_pickup.coord)) {
    return false;
  }

  if (biohazard_pickup.is_visible && SameCoord(coord, biohazard_pickup.coord)) {
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

  for (const auto &slimeball : slimeballs) {
    if (SameCoord(coord, slimeball.current_coord)) {
      return false;
    }
  }

  for (const auto &rocket : active_rockets) {
    if (SameCoord(coord, rocket.current_coord)) {
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

bool Game::IsCellFreeForWalkieTalkiePickup(MapCoord coord) const {
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

  if (plastic_explosive_pickup.is_visible &&
      SameCoord(coord, plastic_explosive_pickup.coord)) {
    return false;
  }

  if (rocket_pickup.is_visible && SameCoord(coord, rocket_pickup.coord)) {
    return false;
  }

  if (biohazard_pickup.is_visible && SameCoord(coord, biohazard_pickup.coord)) {
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

  for (const auto &slimeball : slimeballs) {
    if (SameCoord(coord, slimeball.current_coord)) {
      return false;
    }
  }

  for (const auto &rocket : active_rockets) {
    if (SameCoord(coord, rocket.current_coord)) {
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

bool Game::IsCellFreeForRocketPickup(MapCoord coord) const {
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

  if (plastic_explosive_pickup.is_visible &&
      SameCoord(coord, plastic_explosive_pickup.coord)) {
    return false;
  }

  if (walkie_talkie_pickup.is_visible &&
      SameCoord(coord, walkie_talkie_pickup.coord)) {
    return false;
  }

  if (biohazard_pickup.is_visible && SameCoord(coord, biohazard_pickup.coord)) {
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

  for (const auto &slimeball : slimeballs) {
    if (SameCoord(coord, slimeball.current_coord)) {
      return false;
    }
  }

  for (const auto &rocket : active_rockets) {
    if (SameCoord(coord, rocket.current_coord)) {
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

bool Game::IsCellFreeForBiohazardPickup(MapCoord coord) const {
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

  if (plastic_explosive_pickup.is_visible &&
      SameCoord(coord, plastic_explosive_pickup.coord)) {
    return false;
  }

  if (walkie_talkie_pickup.is_visible &&
      SameCoord(coord, walkie_talkie_pickup.coord)) {
    return false;
  }

  if (rocket_pickup.is_visible && SameCoord(coord, rocket_pickup.coord)) {
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

  for (const auto &slimeball : slimeballs) {
    if (SameCoord(coord, slimeball.current_coord)) {
      return false;
    }
  }

  for (const auto &rocket : active_rockets) {
    if (SameCoord(coord, rocket.current_coord)) {
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
  if (walkie_talkie_pickup.is_visible &&
      SameCoord(coord, walkie_talkie_pickup.coord)) {
    return false;
  }
  if (rocket_pickup.is_visible && SameCoord(coord, rocket_pickup.coord)) {
    return false;
  }
  if (biohazard_pickup.is_visible && SameCoord(coord, biohazard_pickup.coord)) {
    return false;
  }
  for (const auto &dynamite : placed_dynamites) {
    if (SameCoord(coord, dynamite.coord)) {
      return false;
    }
  }
  for (const auto &rocket : active_rockets) {
    if (SameCoord(coord, rocket.current_coord)) {
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

void Game::ActivateLifeRecovery(Uint32 now) {
  if (pacman == nullptr) {
    return;
  }

  remaining_lives = std::max(PLAYER_LIVES_MIN, remaining_lives - 1);
  life_recovery_until_ticks =
      std::max(life_recovery_until_ticks,
               now + PACMAN_EXTRA_LIFE_INVULNERABLE_MS);

  pacman->px_delta.x = 0;
  pacman->px_delta.y = 0;
  pacman->paralyzed_until_ticks = 0;
  pacman->slimed_until_ticks = 0;
  pacman->teleport_animation_active = false;
  pacman->teleport_animation_started_ticks = 0;
  pacman->teleport_animation_from_coord = pacman->map_coord;
  pacman->teleport_animation_to_coord = pacman->map_coord;
  pacman->teleport_arrival_sound_played = false;
  if (events != nullptr) {
    events->Keyreset();
  }
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
#ifdef AUDIO
  audio->PlayDynamiteSpawn();
#endif
}

void Game::UpdateDynamitePickup(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  TrySpawnDynamite(now);
  if (!dynamite_pickup.is_visible) {
    return;
  }

  if (SameCoord(pacman->map_coord, dynamite_pickup.coord)) {
    dynamite_inventory++;
#ifdef AUDIO
    audio->PlayCoin();
#endif
    dynamite_pickup.is_visible = false;
    dynamite_pickup.is_fading = false;
    dynamite_pickup.fade_started_ticks = 0;
    ScheduleNextDynamiteSpawn(now);
    return;
  }

  if (!dynamite_pickup.is_fading &&
      now - dynamite_pickup.appeared_ticks >= tuning.pickup_visible_ms) {
    dynamite_pickup.is_fading = true;
    dynamite_pickup.fade_started_ticks = now;
  }

  if (dynamite_pickup.is_fading &&
      now - dynamite_pickup.fade_started_ticks >= DYNAMITE_FADE_MS) {
    dynamite_pickup.is_visible = false;
    dynamite_pickup.is_fading = false;
    dynamite_pickup.fade_started_ticks = 0;
    ScheduleNextDynamiteSpawn(now);
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
#ifdef AUDIO
  audio->PlayPlasticExplosiveSpawn();
#endif
}

void Game::UpdatePlasticExplosivePickup(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  TrySpawnPlasticExplosive(now);
  if (!plastic_explosive_pickup.is_visible) {
    return;
  }

  if (SameCoord(pacman->map_coord, plastic_explosive_pickup.coord)) {
    plastic_explosive_inventory++;
#ifdef AUDIO
    audio->PlayCoin();
#endif
    plastic_explosive_pickup.is_visible = false;
    plastic_explosive_pickup.is_fading = false;
    plastic_explosive_pickup.fade_started_ticks = 0;
    ScheduleNextPlasticExplosiveSpawn(now);
    return;
  }

  if (!plastic_explosive_pickup.is_fading &&
      now - plastic_explosive_pickup.appeared_ticks >=
          tuning.pickup_visible_ms) {
    plastic_explosive_pickup.is_fading = true;
    plastic_explosive_pickup.fade_started_ticks = now;
  }

  if (plastic_explosive_pickup.is_fading &&
      now - plastic_explosive_pickup.fade_started_ticks >=
          PLASTIC_EXPLOSIVE_FADE_MS) {
    plastic_explosive_pickup.is_visible = false;
    plastic_explosive_pickup.is_fading = false;
    plastic_explosive_pickup.fade_started_ticks = 0;
    ScheduleNextPlasticExplosiveSpawn(now);
  }
}

void Game::TrySpawnWalkieTalkie(Uint32 now) {
  if (walkie_talkie_pickup.is_visible ||
      now < walkie_talkie_pickup.next_spawn_ticks) {
    return;
  }

  std::vector<MapCoord> candidates;
  candidates.reserve(map->get_map_rows() * map->get_map_cols());
  for (size_t row = 0; row < map->get_map_rows(); row++) {
    for (size_t col = 0; col < map->get_map_cols(); col++) {
      MapCoord coord{static_cast<int>(row), static_cast<int>(col)};
      if (IsCellFreeForWalkieTalkiePickup(coord)) {
        candidates.push_back(coord);
      }
    }
  }

  if (candidates.empty()) {
    walkie_talkie_pickup.next_spawn_ticks = now + 1000;
    return;
  }

  std::uniform_int_distribution<size_t> distribution(0, candidates.size() - 1);
  walkie_talkie_pickup.coord = candidates[distribution(RandomGenerator())];
  walkie_talkie_pickup.appeared_ticks = now;
  walkie_talkie_pickup.fade_started_ticks = 0;
  walkie_talkie_pickup.animation_seed = static_cast<int>(
      (now % 997) + walkie_talkie_pickup.coord.u * 29 +
      walkie_talkie_pickup.coord.v * 43);
  walkie_talkie_pickup.is_visible = true;
  walkie_talkie_pickup.is_fading = false;
#ifdef AUDIO
  audio->PlayDynamiteSpawn();
#endif
}

void Game::UpdateWalkieTalkiePickup(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  TrySpawnWalkieTalkie(now);
  if (!walkie_talkie_pickup.is_visible) {
    return;
  }

  if (SameCoord(pacman->map_coord, walkie_talkie_pickup.coord)) {
    airstrike_inventory++;
#ifdef AUDIO
    audio->PlayCoin();
#endif
    walkie_talkie_pickup.is_visible = false;
    walkie_talkie_pickup.is_fading = false;
    walkie_talkie_pickup.fade_started_ticks = 0;
    ScheduleNextWalkieTalkieSpawn(now);
    return;
  }

  if (!walkie_talkie_pickup.is_fading &&
      now - walkie_talkie_pickup.appeared_ticks >= tuning.pickup_visible_ms) {
    walkie_talkie_pickup.is_fading = true;
    walkie_talkie_pickup.fade_started_ticks = now;
  }

  if (walkie_talkie_pickup.is_fading &&
      now - walkie_talkie_pickup.fade_started_ticks >=
          WALKIE_TALKIE_FADE_MS) {
    walkie_talkie_pickup.is_visible = false;
    walkie_talkie_pickup.is_fading = false;
    walkie_talkie_pickup.fade_started_ticks = 0;
    ScheduleNextWalkieTalkieSpawn(now);
  }
}

void Game::TrySpawnRocket(Uint32 now) {
  if (rocket_pickup.is_visible || now < rocket_pickup.next_spawn_ticks) {
    return;
  }

  std::vector<MapCoord> candidates;
  candidates.reserve(map->get_map_rows() * map->get_map_cols());
  for (size_t row = 0; row < map->get_map_rows(); row++) {
    for (size_t col = 0; col < map->get_map_cols(); col++) {
      MapCoord coord{static_cast<int>(row), static_cast<int>(col)};
      if (IsCellFreeForRocketPickup(coord)) {
        candidates.push_back(coord);
      }
    }
  }

  if (candidates.empty()) {
    rocket_pickup.next_spawn_ticks = now + 1000;
    return;
  }

  std::uniform_int_distribution<size_t> distribution(0, candidates.size() - 1);
  rocket_pickup.coord = candidates[distribution(RandomGenerator())];
  rocket_pickup.appeared_ticks = now;
  rocket_pickup.fade_started_ticks = 0;
  rocket_pickup.animation_seed = static_cast<int>(
      (now % 997) + rocket_pickup.coord.u * 59 + rocket_pickup.coord.v * 67);
  rocket_pickup.is_visible = true;
  rocket_pickup.is_fading = false;
#ifdef AUDIO
  audio->PlayDynamiteSpawn();
#endif
}

void Game::UpdateRocketPickup(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  TrySpawnRocket(now);
  if (!rocket_pickup.is_visible) {
    return;
  }

  if (SameCoord(pacman->map_coord, rocket_pickup.coord)) {
    rocket_inventory++;
#ifdef AUDIO
    audio->PlayCoin();
#endif
    rocket_pickup.is_visible = false;
    rocket_pickup.is_fading = false;
    rocket_pickup.fade_started_ticks = 0;
    ScheduleNextRocketSpawn(now);
    return;
  }

  if (!rocket_pickup.is_fading &&
      now - rocket_pickup.appeared_ticks >= tuning.pickup_visible_ms) {
    rocket_pickup.is_fading = true;
    rocket_pickup.fade_started_ticks = now;
  }

  if (rocket_pickup.is_fading &&
      now - rocket_pickup.fade_started_ticks >= ROCKET_FADE_MS) {
    rocket_pickup.is_visible = false;
    rocket_pickup.is_fading = false;
    rocket_pickup.fade_started_ticks = 0;
    ScheduleNextRocketSpawn(now);
  }
}

void Game::TrySpawnBiohazard(Uint32 now) {
  if (biohazard_pickup.is_visible || now < biohazard_pickup.next_spawn_ticks) {
    return;
  }

  std::vector<MapCoord> candidates;
  candidates.reserve(map->get_map_rows() * map->get_map_cols());
  for (size_t row = 0; row < map->get_map_rows(); row++) {
    for (size_t col = 0; col < map->get_map_cols(); col++) {
      MapCoord coord{static_cast<int>(row), static_cast<int>(col)};
      if (IsCellFreeForBiohazardPickup(coord)) {
        candidates.push_back(coord);
      }
    }
  }

  if (candidates.empty()) {
    biohazard_pickup.next_spawn_ticks = now + 1000;
    return;
  }

  std::uniform_int_distribution<size_t> distribution(0, candidates.size() - 1);
  biohazard_pickup.coord = candidates[distribution(RandomGenerator())];
  biohazard_pickup.appeared_ticks = now;
  biohazard_pickup.fade_started_ticks = 0;
  biohazard_pickup.animation_seed =
      static_cast<int>((now % 997) + biohazard_pickup.coord.u * 83 +
                       biohazard_pickup.coord.v * 47);
  biohazard_pickup.is_visible = true;
  biohazard_pickup.is_fading = false;
#ifdef AUDIO
  audio->PlayDynamiteSpawn();
#endif
}

void Game::UpdateBiohazardPickup(Uint32 now) {
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  TrySpawnBiohazard(now);
  if (!biohazard_pickup.is_visible) {
    return;
  }

  if (SameCoord(pacman->map_coord, biohazard_pickup.coord)) {
    biohazard_inventory++;
#ifdef AUDIO
    audio->PlayCoin();
#endif
    biohazard_pickup.is_visible = false;
    biohazard_pickup.is_fading = false;
    biohazard_pickup.fade_started_ticks = 0;
    ScheduleNextBiohazardSpawn(now);
    return;
  }

  if (!biohazard_pickup.is_fading &&
      now - biohazard_pickup.appeared_ticks >= tuning.pickup_visible_ms) {
    biohazard_pickup.is_fading = true;
    biohazard_pickup.fade_started_ticks = now;
  }

  if (biohazard_pickup.is_fading &&
      now - biohazard_pickup.fade_started_ticks >= BIOHAZARD_FADE_MS) {
    biohazard_pickup.is_visible = false;
    biohazard_pickup.is_fading = false;
    biohazard_pickup.fade_started_ticks = 0;
    ScheduleNextBiohazardSpawn(now);
  }
}

void Game::TryUseBiohazardBeam(Uint32 now) {
  if (events == nullptr || pacman == nullptr) {
    return;
  }

  const bool use_requested =
      events->ConsumeExtraUseRequest(ExtraSlot::Biohazard);
  if (!use_requested || biohazard_inventory <= 0 ||
      active_biohazard_beam.is_active) {
    return;
  }

  Directions facing_direction = events->get_next_move();
  if (facing_direction != Directions::None) {
    pacman->facing_direction = facing_direction;
  } else {
    facing_direction = pacman->facing_direction;
  }
  if (facing_direction == Directions::None) {
    facing_direction = Directions::Down;
  }

  active_biohazard_beam = {};
  active_biohazard_beam.is_active = true;
  active_biohazard_beam.direction = facing_direction;
  active_biohazard_beam.started_ticks = now;
  active_biohazard_beam.minimum_visible_until_ticks =
      now + BIOHAZARD_BEAM_MIN_VISIBLE_MS;
  active_biohazard_beam.animation_seed =
      static_cast<int>((now % 997) + pacman->map_coord.u * 89 +
                       pacman->map_coord.v * 53 + biohazard_inventory * 29);
  biohazard_inventory--;
#ifdef AUDIO
  audio->StartBiohazardBeam();
#endif
}

void Game::StopBiohazardBeam() {
  active_biohazard_beam = {};
#ifdef AUDIO
  if (audio != nullptr) {
    audio->StopBiohazardBeam();
  }
#endif
}

void Game::UpdateBiohazardBeam(Uint32 now) {
  if (!active_biohazard_beam.is_active || pacman == nullptr) {
    return;
  }

  if (active_biohazard_beam.is_locked_after_hit) {
    if (now >= active_biohazard_beam.locked_until_ticks) {
      StopBiohazardBeam();
    }
    return;
  }

  if (now - active_biohazard_beam.started_ticks >= BIOHAZARD_BEAM_ACTIVE_MS) {
    StopBiohazardBeam();
    return;
  }

  Monster *closest_target = nullptr;
  int closest_distance = std::numeric_limits<int>::max();
  for (Monster *monster : monsters) {
    if (monster == nullptr || !monster->is_alive || monster->is_electrified ||
        !IsMonsterHitByBiohazardBeam(monster)) {
      continue;
    }

    const int distance = std::abs(monster->map_coord.u - pacman->map_coord.u) +
                         std::abs(monster->map_coord.v - pacman->map_coord.v);
    if (distance >= closest_distance) {
      continue;
    }

    closest_distance = distance;
    closest_target = monster;
  }

  if (closest_target != nullptr) {
    SDL_FPoint impact_point = MakeCellCenter(closest_target->map_coord);
    impact_point.x += static_cast<float>(closest_target->px_delta.x) / 100.0f;
    impact_point.y += static_cast<float>(closest_target->px_delta.y) / 100.0f;
    const float impact_offset = 0.24f;
    switch (active_biohazard_beam.direction) {
    case Directions::Up:
      impact_point.y += impact_offset;
      break;
    case Directions::Down:
      impact_point.y -= impact_offset;
      break;
    case Directions::Left:
      impact_point.x += impact_offset;
      break;
    case Directions::Right:
      impact_point.x -= impact_offset;
      break;
    case Directions::None:
    default:
      break;
    }

    ElectrifyMonster(closest_target, now);
    closest_target->biohazard_paralyzed_until_ticks = std::max(
        closest_target->biohazard_paralyzed_until_ticks,
        now + kBiohazardHitSequenceDurationMs);
    closest_target->px_delta.x = 0;
    closest_target->px_delta.y = 0;
    GameEffect impact_flash{closest_target->map_coord, now,
                            EffectType::BiohazardImpactFlash, 1};
    impact_flash.has_precise_world_center = true;
    impact_flash.precise_world_center = impact_point;
    effects.push_back(impact_flash);

    active_biohazard_beam.hit_sequence_started_ticks = now;
    active_biohazard_beam.locked_until_ticks =
        now + kBiohazardHitSequenceDurationMs;
    active_biohazard_beam.is_locked_after_hit = true;
    active_biohazard_beam.locked_origin_coord = pacman->map_coord;
    active_biohazard_beam.locked_origin_delta = pacman->px_delta;
    active_biohazard_beam.locked_end_world_center = impact_point;
  }
}

void Game::TryPlaceDynamite(Uint32 now) {
  if (events == nullptr || pacman == nullptr) {
    return;
  }

  const bool place_requested =
      events->ConsumeExtraUseRequest(ExtraSlot::Dynamite);
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

  const bool use_requested =
      events->ConsumeExtraUseRequest(ExtraSlot::PlasticExplosive);
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

void Game::TryFireRocket(Uint32 now) {
  if (events == nullptr || pacman == nullptr) {
    return;
  }

  const bool fire_requested =
      events->ConsumeExtraUseRequest(ExtraSlot::Rocket);
  if (!fire_requested || rocket_inventory <= 0 || !active_rockets.empty()) {
    return;
  }

  Directions facing_direction = pacman->facing_direction;
  if (facing_direction == Directions::None) {
    facing_direction = Directions::Down;
  }

  active_rockets.push_back(
      {pacman->map_coord,
       facing_direction,
       now,
       ROCKET_STEP_DURATION_MS,
       static_cast<int>((now % 997) + pacman->map_coord.u * 79 +
                        pacman->map_coord.v * 61 + rocket_inventory * 23)});
  rocket_inventory--;
#ifdef AUDIO
  audio->PlayRocketLaunch();
#endif
}

void Game::TryUseAirstrike(Uint32 now) {
  if (events == nullptr || pacman == nullptr) {
    return;
  }

  const bool use_requested =
      events->ConsumeExtraUseRequest(ExtraSlot::Airstrike);
  if (!use_requested || airstrike_inventory <= 0 || active_airstrike.is_active) {
    return;
  }

  active_airstrike = {};
  active_airstrike.is_active = true;
  active_airstrike.target_coord = pacman->map_coord;
  active_airstrike.started_ticks = now;
  active_airstrike.animation_seed =
      static_cast<int>((now % 997) + pacman->map_coord.u * 53 +
                       pacman->map_coord.v * 71 + airstrike_inventory * 17);
  active_airstrike.flight_start = RandomAirstrikeStart(map);

  const SDL_FPoint target_point = MakeCellCenter(active_airstrike.target_coord);
  const SDL_FPoint approach_vector =
      SubtractPoint(target_point, active_airstrike.flight_start);
  const double approach_length = PointDistance(active_airstrike.flight_start,
                                               target_point);
  if (approach_length <= 0.001) {
    active_airstrike = {};
    return;
  }

  const SDL_FPoint direction = ScalePoint(
      approach_vector, static_cast<float>(1.0 / approach_length));
  const double max_span =
      std::hypot(static_cast<double>(map->get_map_rows()),
                 static_cast<double>(map->get_map_cols())) +
      kAirstrikePlaneMarginCells * 3.0;
  active_airstrike.flight_end =
      AddPoint(target_point, ScalePoint(direction, static_cast<float>(max_span)));
  active_airstrike.flight_direction =
      DominantDirection(SubtractPoint(active_airstrike.flight_end,
                                      active_airstrike.flight_start));

  const double total_path_length =
      PointDistance(active_airstrike.flight_start, active_airstrike.flight_end);
  if (total_path_length <= 0.001) {
    active_airstrike = {};
    return;
  }

  active_airstrike.target_progress =
      std::clamp(approach_length / total_path_length, 0.0, 1.0);

#ifdef AUDIO
  const Uint32 radio_delay_ms =
      (audio != nullptr) ? audio->GetAirstrikeRadioDurationMs()
                         : AIRSTRIKE_RADIO_DELAY_MS;
#else
  const Uint32 radio_delay_ms = AIRSTRIKE_RADIO_DELAY_MS;
#endif
  const Uint32 plane_flight_ms = std::max<Uint32>(
      AIRSTRIKE_PLANE_MIN_FLIGHT_MS,
      static_cast<Uint32>(
          std::lround(total_path_length * AIRSTRIKE_PLANE_CELL_TRAVEL_MS)));
  active_airstrike.plane_launch_ticks = now + radio_delay_ms;
  active_airstrike.overflight_ticks =
      active_airstrike.plane_launch_ticks +
      static_cast<Uint32>(std::lround(active_airstrike.target_progress *
                                      static_cast<double>(plane_flight_ms)));
  active_airstrike.plane_finished_ticks =
      active_airstrike.plane_launch_ticks + plane_flight_ms;

  std::vector<AirstrikePathCandidate> bomb_candidates =
      CollectAirstrikePathCandidates(map, active_airstrike.flight_start,
                                     active_airstrike.flight_end,
                                     active_airstrike.target_coord);
  if (bomb_candidates.size() > AIRSTRIKE_BOMB_COUNT) {
    std::shuffle(bomb_candidates.begin(), bomb_candidates.end(),
                 RandomGenerator());
    bomb_candidates.resize(AIRSTRIKE_BOMB_COUNT);
  }
  std::sort(bomb_candidates.begin(), bomb_candidates.end(),
            [](const AirstrikePathCandidate &left,
               const AirstrikePathCandidate &right) {
              return left.progress < right.progress;
            });
  active_airstrike.bombs.reserve(bomb_candidates.size());

  for (size_t index = 0; index < bomb_candidates.size(); ++index) {
    const auto &candidate = bomb_candidates[index];
    const Uint32 release_ticks =
        active_airstrike.plane_launch_ticks +
        static_cast<Uint32>(std::lround(candidate.progress *
                                        static_cast<double>(plane_flight_ms)));
    active_airstrike.bombs.push_back(
        {candidate.coord,
         candidate.point,
         candidate.progress,
         release_ticks,
         release_ticks + AIRSTRIKE_BOMB_FALL_MS,
         false,
         static_cast<int>(active_airstrike.animation_seed + index * 31)});
  }

  if (active_airstrike.bombs.empty()) {
    active_airstrike = {};
    return;
  }

  active_airstrike.finished_ticks = std::max(
      active_airstrike.plane_finished_ticks,
      active_airstrike.bombs.back().explode_ticks +
          static_cast<Uint32>(AIRSTRIKE_EXPLOSION_DURATION_MS));

  airstrike_inventory--;
#ifdef AUDIO
  audio->PlayAirstrikeRadio();
#endif
}

void Game::UpdateAirstrike(Uint32 now) {
  if (!active_airstrike.is_active) {
    return;
  }

  for (auto &bomb : active_airstrike.bombs) {
    if (bomb.exploded || now < bomb.explode_ticks) {
      continue;
    }

    bomb.exploded = true;
    DetonateAirstrikeBomb(bomb, now);
  }

  const bool all_bombs_resolved =
      std::all_of(active_airstrike.bombs.begin(), active_airstrike.bombs.end(),
                  [](const AirstrikeBomb &bomb) { return bomb.exploded; });
  if (all_bombs_resolved && now >= active_airstrike.finished_ticks) {
    active_airstrike = {};
  }
}

void Game::UpdateRockets(Uint32 now) {
  for (size_t index = 0; index < active_rockets.size();) {
    RocketProjectile &rocket = active_rockets[index];
    const Uint32 step_duration_ms = std::max<Uint32>(1, rocket.step_duration_ms);
    bool detonate_rocket = false;
    bool hit_wall = false;
    MapCoord impact_coord = rocket.current_coord;
    Monster *hit_monster = nullptr;

    while (!detonate_rocket &&
           now - rocket.segment_started_ticks >= step_duration_ms) {
      const MapCoord next_coord = StepCoord(rocket.current_coord, rocket.direction);
      if (map->map_entry(static_cast<size_t>(next_coord.u),
                         static_cast<size_t>(next_coord.v)) ==
          ElementType::TYPE_WALL) {
        impact_coord = rocket.current_coord;
        hit_wall = true;
        detonate_rocket = true;
        break;
      }

      rocket.current_coord = next_coord;
      rocket.segment_started_ticks += step_duration_ms;

      for (auto *monster : monsters) {
        if (!monster->is_alive) {
          continue;
        }

        if (SameCoord(monster->map_coord, rocket.current_coord)) {
          hit_monster = monster;
          impact_coord = rocket.current_coord;
          detonate_rocket = true;
          break;
        }
      }
    }

    if (detonate_rocket) {
      const RocketProjectile detonated_rocket = rocket;
      active_rockets.erase(active_rockets.begin() + static_cast<long>(index));
      DetonateRocket(detonated_rocket, impact_coord, hit_monster, hit_wall, now);
    } else {
      index++;
    }
  }
}

void Game::UpdateElectrifiedMonsters(Uint32 now) {
  const auto get_monster_world_center = [](const Monster *current) {
    SDL_FPoint center = MakeCellCenter(current->map_coord);
    center.x += static_cast<float>(current->px_delta.x) / 100.0f;
    center.y += static_cast<float>(current->px_delta.y) / 100.0f;
    return center;
  };
  const auto find_charge_target = [&](Monster *current) -> Monster * {
    if (map == nullptr || current == nullptr || !current->is_alive ||
        !current->is_electrified ||
        current->biohazard_paralyzed_until_ticks > now) {
      return nullptr;
    }

    Monster *best_target = nullptr;
    int best_distance = std::numeric_limits<int>::max();
    for (Monster *other : monsters) {
      if (other == nullptr || other == current || !other->is_alive) {
        continue;
      }

      Directions direction = Directions::None;
      if (!HasLineOfSight(map, current->map_coord, other->map_coord, direction)) {
        continue;
      }

      const int distance = std::abs(other->map_coord.u - current->map_coord.u) +
                           std::abs(other->map_coord.v - current->map_coord.v);
      if (distance >= best_distance) {
        continue;
      }

      best_distance = distance;
      best_target = other;
    }

    return best_target;
  };

  for (Monster *monster : monsters) {
    if (monster == nullptr || !monster->is_alive || !monster->is_electrified) {
      continue;
    }

    if (monster->biohazard_paralyzed_until_ticks > now) {
      monster->electrified_charge_target_id = -1;
      continue;
    }

    Monster *target = find_charge_target(monster);
    const int next_target_id = (target != nullptr) ? target->id : -1;
    if (next_target_id != -1 &&
        monster->electrified_charge_target_id != next_target_id) {
#ifdef AUDIO
      audio->PlayElectrifiedMonsterRoar();
#endif
    }
    monster->electrified_charge_target_id = next_target_id;

    for (Monster *other : monsters) {
      if (other == nullptr || other == monster || !other->is_alive) {
        continue;
      }

      if (!SameCoord(monster->map_coord, other->map_coord)) {
        continue;
      }

      const SDL_FPoint monster_center = get_monster_world_center(monster);
      const SDL_FPoint other_center = get_monster_world_center(other);
      const SDL_FPoint shared_center =
          ScalePoint(AddPoint(monster_center, other_center), 0.5f);
      GameEffect shockwave_effect{monster->map_coord, now,
                                  EffectType::PlasmaShockwave,
                                  PLASMA_SHOCKWAVE_RADIUS_CELLS};
      shockwave_effect.has_precise_world_center = true;
      shockwave_effect.precise_world_center = shared_center;
      effects.push_back(shockwave_effect);
#ifdef AUDIO
      audio->PlayElectrifiedMonsterImpact();
#endif

      const size_t effect_count_before = effects.size();
      EliminateMonster(monster, now);
      EliminateMonster(other, now);
      for (size_t effect_index = effect_count_before;
           effect_index < effects.size(); ++effect_index) {
        if (effects[effect_index].type == EffectType::MonsterExplosion) {
          effects[effect_index].has_precise_world_center = true;
          effects[effect_index].precise_world_center = shared_center;
        }
      }
      break;
    }
  }
}

void Game::ScheduleMonsterBlast(Monster *monster, Uint32 trigger_ticks) {
  if (monster == nullptr || !monster->is_alive) {
    return;
  }

  monster->is_alive = false;
  monster->px_delta.x = 0;
  monster->px_delta.y = 0;
  monster->electrified_charge_target_id = -1;
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
  const SDL_FPoint explosive_center = MakeCellCenter(explosive.coord);
  for (auto *monster : monsters) {
    if (!monster->is_alive) {
      continue;
    }

    SDL_FPoint monster_center = MakeCellCenter(monster->map_coord);
    monster_center.x += static_cast<float>(monster->px_delta.x) / 100.0f;
    monster_center.y += static_cast<float>(monster->px_delta.y) / 100.0f;
    if (PointDistance(explosive_center, monster_center) >
        kPlasticExplosiveMonsterHitRadiusCells) {
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

void Game::DetonateAirstrikeBomb(const AirstrikeBomb &bomb, Uint32 now) {
  effects.push_back(
      {bomb.coord, now, EffectType::AirstrikeExplosion,
       AIRSTRIKE_EXPLOSION_RADIUS_CELLS});
#ifdef AUDIO
  audio->PlayAirstrikeExplosion();
#endif

  for (auto *monster : monsters) {
    if (!monster->is_alive ||
        !IsWithinRadius(bomb.coord, monster->map_coord,
                        AIRSTRIKE_EXPLOSION_RADIUS_CELLS)) {
      continue;
    }

    EliminateMonster(monster, now);
  }

  if (!IsPacmanInvulnerable(now) &&
      IsWithinRadius(bomb.coord, pacman->map_coord,
                     AIRSTRIKE_EXPLOSION_RADIUS_CELLS)) {
    TriggerLoss(pacman->map_coord, now);
  }
}

void Game::DetonateRocket(const RocketProjectile &rocket, MapCoord impact_coord,
                          Monster *hit_monster, bool hit_wall, Uint32 now) {
#ifdef AUDIO
  audio->StopRocketLaunch();
  audio->PlayAirstrikeExplosion();
#endif
  GameEffect explosion_effect{impact_coord, now, EffectType::AirstrikeExplosion, 1};
  if (hit_wall) {
    explosion_effect.has_precise_world_center = true;
    explosion_effect.precise_world_center = MakeCellCenter(rocket.current_coord);
    switch (rocket.direction) {
    case Directions::Up:
      explosion_effect.precise_world_center.y -= 0.5f;
      break;
    case Directions::Down:
      explosion_effect.precise_world_center.y += 0.5f;
      break;
    case Directions::Left:
      explosion_effect.precise_world_center.x -= 0.5f;
      break;
    case Directions::Right:
      explosion_effect.precise_world_center.x += 0.5f;
      break;
    case Directions::None:
    default:
      break;
    }
  }
  effects.push_back(explosion_effect);

  if (hit_monster != nullptr && hit_monster->is_alive &&
      SameCoord(hit_monster->map_coord, impact_coord)) {
    EliminateMonsterWithDynamiteBlast(hit_monster, now);
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
  const DifficultyTuning tuning = GetDifficultyTuning(difficulty);
  TrySpawnInvulnerabilityPotion(now);
  if (!invulnerability_potion.is_visible) {
    return;
  }

  if (SameCoord(pacman->map_coord, invulnerability_potion.coord)) {
    ActivateInvulnerability(now);
#ifdef AUDIO
    audio->PlayCoin();
#endif
    invulnerability_potion.is_visible = false;
    invulnerability_potion.is_fading = false;
    invulnerability_potion.fade_started_ticks = 0;
    ScheduleNextInvulnerabilityPotionSpawn(now);
    return;
  }

  if (!invulnerability_potion.is_fading &&
      now - invulnerability_potion.appeared_ticks >= tuning.pickup_visible_ms) {
    invulnerability_potion.is_fading = true;
    invulnerability_potion.fade_started_ticks = now;
  }

  if (invulnerability_potion.is_fading &&
      now - invulnerability_potion.fade_started_ticks >= BLUE_POTION_FADE_MS) {
    invulnerability_potion.is_visible = false;
    invulnerability_potion.is_fading = false;
    invulnerability_potion.fade_started_ticks = 0;
    ScheduleNextInvulnerabilityPotionSpawn(now);
  }
}

void Game::UpdateInvulnerability(Uint32 now) {
  if (life_recovery_until_ticks != 0 && now >= life_recovery_until_ticks) {
    life_recovery_until_ticks = 0;
  }

  if (pacman->invulnerable_until_ticks != 0 &&
      now >= pacman->invulnerable_until_ticks) {
    pacman->invulnerable_until_ticks = 0;
#ifdef AUDIO
    audio->StopInvulnerabilityLoop();
#endif
  }
}

void Game::UpdateFinalLossSequence(Uint32 now) {
  if (dead || final_loss_commits_at_ticks == 0 ||
      now < final_loss_commits_at_ticks) {
    return;
  }

  final_loss_commits_at_ticks = 0;
#ifdef AUDIO
  audio->StartLoseMusic();
#endif
  dead = true;
}

bool Game::IsPacmanInvulnerable(Uint32 now) const {
  return pacman != nullptr &&
         (pacman->invulnerable_until_ticks > now ||
          life_recovery_until_ticks > now);
}

bool Game::IsPacmanRecoveringFromLifeLoss(Uint32 now) const {
  return pacman != nullptr && life_recovery_until_ticks > now;
}

Monster *Game::FindMonsterById(int monster_id) const {
  for (Monster *monster : monsters) {
    if (monster != nullptr && monster->id == monster_id) {
      return monster;
    }
  }

  return nullptr;
}

bool Game::IsMonsterHitByBiohazardBeam(const Monster *monster) const {
  if (monster == nullptr || !active_biohazard_beam.is_active || pacman == nullptr ||
      map == nullptr || !monster->is_alive || SameCoord(monster->map_coord, pacman->map_coord)) {
    return false;
  }

  Directions beam_direction = Directions::None;
  if (!HasLineOfSight(map, pacman->map_coord, monster->map_coord,
                      beam_direction)) {
    return false;
  }

  return beam_direction == active_biohazard_beam.direction;
}

void Game::ElectrifyMonster(Monster *monster, Uint32 now) {
  if (monster == nullptr || !monster->is_alive) {
    return;
  }

  monster->is_electrified = true;
  monster->electrified_charge_target_id = -1;
  monster->electrified_started_ticks = now;
  monster->electrified_visual_seed =
      static_cast<int>((now % 997) + monster->id * 41 + monster->map_coord.u * 13 +
                       monster->map_coord.v * 17);
}

void Game::UpdateFireballs(Uint32 now) {
  for (size_t index = 0; index < fireballs.size();) {
    Fireball &fireball = fireballs[index];
    const Uint32 step_duration_ms = std::max<Uint32>(1, fireball.step_duration_ms);
    bool remove_fireball = false;

    while (!remove_fireball &&
           now - fireball.segment_started_ticks >= step_duration_ms) {
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
      fireball.segment_started_ticks += step_duration_ms;

      if (SameCoord(pacman->map_coord, fireball.current_coord)) {
#ifdef AUDIO
        audio->PlayFireballWallHit();
#endif
        const Monster *owner = FindMonsterById(fireball.owner_id);
        if ((owner == nullptr || !owner->is_electrified) &&
            !IsPacmanInvulnerable(now)) {
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

void Game::UpdateSlimeballs(Uint32 now) {
  for (size_t index = 0; index < slimeballs.size();) {
    Slimeball &slimeball = slimeballs[index];
    const Uint32 step_duration_ms =
        std::max<Uint32>(1, slimeball.step_duration_ms);
    bool remove_slimeball = false;

    while (!remove_slimeball &&
           now - slimeball.segment_started_ticks >= step_duration_ms) {
      MapCoord next_coord = StepCoord(slimeball.current_coord, slimeball.direction);

      if (map->map_entry(next_coord.u, next_coord.v) == ElementType::TYPE_WALL) {
        GameEffect slime_impact_effect{
            slimeball.current_coord, now, EffectType::SlimeSplash, 1};
        slime_impact_effect.has_precise_world_center = true;
        slime_impact_effect.precise_world_center =
            MakeCellCenter(slimeball.current_coord);
        switch (slimeball.direction) {
        case Directions::Up:
          slime_impact_effect.precise_world_center.y -= 0.5f;
          break;
        case Directions::Down:
          slime_impact_effect.precise_world_center.y += 0.5f;
          break;
        case Directions::Left:
          slime_impact_effect.precise_world_center.x -= 0.5f;
          break;
        case Directions::Right:
          slime_impact_effect.precise_world_center.x += 0.5f;
          break;
        case Directions::None:
        default:
          break;
        }
        effects.push_back(slime_impact_effect);
#ifdef AUDIO
        audio->PlaySlimeImpact();
#endif
        remove_slimeball = true;
        break;
      }

      slimeball.current_coord = next_coord;
      slimeball.segment_started_ticks += step_duration_ms;

      if (SameCoord(pacman->map_coord, slimeball.current_coord)) {
#ifdef AUDIO
        audio->PlaySlimeImpact();
#endif
        const Monster *owner = FindMonsterById(slimeball.owner_id);
        if ((owner == nullptr || !owner->is_electrified) &&
            !IsPacmanInvulnerable(now)) {
          pacman->paralyzed_until_ticks =
              std::max(pacman->paralyzed_until_ticks,
                       now + static_cast<Uint32>(PACMAN_PARALYSIS_MS));
          pacman->px_delta.x = 0;
          pacman->px_delta.y = 0;
          ActivateSlimeCover(now);
        }
        remove_slimeball = true;
        break;
      }
    }

    if (remove_slimeball) {
      slimeballs.erase(slimeballs.begin() + static_cast<long>(index));
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
      const Monster *owner = FindMonsterById(cloud.owner_id);
      if ((owner == nullptr || !owner->is_electrified) &&
          !IsPacmanInvulnerable(now)) {
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

void Game::ActivateSlimeCover(Uint32 now) {
  pacman->slimed_until_ticks =
      std::max(pacman->slimed_until_ticks,
               now + static_cast<Uint32>(SLIME_OVERLAY_HOLD_MS +
                                         SLIME_OVERLAY_FADE_MS));
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
                                  EffectType::BiohazardImpactFlash) {
                         max_age = kBiohazardImpactFlashDurationMs;
                       } else if (effect.type ==
                                  EffectType::DynamiteExplosion) {
                         max_age = DYNAMITE_EXPLOSION_DURATION_MS;
                       } else if (effect.type ==
                                  EffectType::AirstrikeExplosion) {
                         max_age = AIRSTRIKE_EXPLOSION_DURATION_MS;
                       } else if (effect.type == EffectType::SlimeSplash) {
                         max_age = kSlimeSplashDurationMs;
                       } else if (effect.type == EffectType::PlasmaShockwave) {
                         max_age = kPlasmaShockwaveDurationMs;
                       }
                       return now - effect.started_ticks > max_age;
                     }),
      effects.end());
}

void Game::EliminateMonsterWithDynamiteBlast(Monster *monster, Uint32 now) {
  if (monster == nullptr || !monster->is_alive) {
    return;
  }

  SDL_FPoint sprite_center = MakeCellCenter(monster->map_coord);
  sprite_center.x += static_cast<float>(monster->px_delta.x) / 100.0f;
  sprite_center.y += static_cast<float>(monster->px_delta.y) / 100.0f;

  monster->is_alive = false;
  monster->px_delta.x = 0;
  monster->px_delta.y = 0;
  monster->electrified_charge_target_id = -1;
  monster->scheduled_dynamite_blast_ticks = 0;
  monster->death_coord = monster->map_coord;
  monster->death_started_ticks = now;
  GameEffect explosion_effect{monster->death_coord, now,
                              EffectType::MonsterExplosion, 1};
  explosion_effect.has_precise_world_center = true;
  explosion_effect.precise_world_center = sprite_center;
  effects.push_back(explosion_effect);
#ifdef AUDIO
  audio->PlayDynamiteExplosion();
#endif
}

void Game::EliminateMonster(Monster *monster, Uint32 now) {
  if (monster == nullptr || !monster->is_alive) {
    return;
  }

  SDL_FPoint sprite_center = MakeCellCenter(monster->map_coord);
  sprite_center.x += static_cast<float>(monster->px_delta.x) / 100.0f;
  sprite_center.y += static_cast<float>(monster->px_delta.y) / 100.0f;

  monster->is_alive = false;
  monster->px_delta.x = 0;
  monster->px_delta.y = 0;
  monster->scheduled_dynamite_blast_ticks = 0;
  monster->electrified_charge_target_id = -1;
  monster->death_coord = monster->map_coord;
  monster->death_started_ticks = now;
  GameEffect explosion_effect{monster->death_coord, now,
                              EffectType::MonsterExplosion, 1};
  explosion_effect.has_precise_world_center = true;
  explosion_effect.precise_world_center = sprite_center;
  effects.push_back(explosion_effect);
#ifdef AUDIO
  audio->PlayMonsterExplosion();
#endif
}
