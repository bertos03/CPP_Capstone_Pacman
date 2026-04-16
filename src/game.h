/*
 * PROJECT COMMENT (game.h)
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

#ifndef GAME_H
#define GAME_H

#include <SDL.h>

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "audio.h"
#include "events.h"
#include "goodie.h"
#include "map.h"
#include "mapelement.h"
#include "monster.h"
#include "pacman.h"
#include "renderer.h"

class Map;
class Events;
class Pacman;
class Monster;
class Goodie;
class Renderer;

struct Fireball {
  // Current segment position of the projectile.
  MapCoord current_coord;
  Directions direction;
  Uint32 segment_started_ticks;
  // Monster ID so self-hits can be ignored.
  int owner_id;
};

struct GasCloud {
  MapCoord coord;
  Uint32 started_ticks;
  Uint32 fade_started_ticks;
  int animation_seed;
  bool is_fading;
  // Set once Pacman has walked into the cloud.
  bool triggered_by_pacman;
};

struct InvulnerabilityPotion {
  MapCoord coord{0, 0};
  Uint32 appeared_ticks = 0;
  Uint32 fade_started_ticks = 0;
  Uint32 next_spawn_ticks = 0;
  int animation_seed = 0;
  bool is_visible = false;
  bool is_fading = false;
};

struct DynamitePickup {
  MapCoord coord{0, 0};
  Uint32 appeared_ticks = 0;
  Uint32 fade_started_ticks = 0;
  Uint32 next_spawn_ticks = 0;
  int animation_seed = 0;
  bool is_visible = false;
  bool is_fading = false;
};

struct PlasticExplosivePickup {
  MapCoord coord{0, 0};
  Uint32 appeared_ticks = 0;
  Uint32 fade_started_ticks = 0;
  Uint32 next_spawn_ticks = 0;
  int animation_seed = 0;
  bool is_visible = false;
  bool is_fading = false;
};

struct WalkieTalkiePickup {
  MapCoord coord{0, 0};
  Uint32 appeared_ticks = 0;
  Uint32 fade_started_ticks = 0;
  Uint32 next_spawn_ticks = 0;
  int animation_seed = 0;
  bool is_visible = false;
  bool is_fading = false;
};

struct PlacedDynamite {
  MapCoord coord{0, 0};
  Uint32 placed_ticks = 0;
  Uint32 explode_at_ticks = 0;
  int animation_seed = 0;
};

struct PlacedPlasticExplosive {
  MapCoord coord{0, 0};
  Uint32 placed_ticks = 0;
  int animation_seed = 0;
  bool embedded_in_wall = false;
};

struct ScheduledMonsterBlast {
  Monster *monster = nullptr;
  MapCoord coord{0, 0};
  Uint32 trigger_ticks = 0;
};

struct AirstrikeBomb {
  MapCoord coord{0, 0};
  SDL_FPoint release_position{0.0f, 0.0f};
  double path_progress = 0.0;
  Uint32 release_ticks = 0;
  Uint32 explode_ticks = 0;
  bool exploded = false;
  int animation_seed = 0;
};

struct ActiveAirstrike {
  bool is_active = false;
  Directions flight_direction = Directions::Right;
  MapCoord target_coord{0, 0};
  Uint32 started_ticks = 0;
  Uint32 plane_launch_ticks = 0;
  Uint32 overflight_ticks = 0;
  Uint32 plane_finished_ticks = 0;
  Uint32 finished_ticks = 0;
  int animation_seed = 0;
  double target_progress = 0.5;
  SDL_FPoint flight_start{0.0f, 0.0f};
  SDL_FPoint flight_end{0.0f, 0.0f};
  std::vector<AirstrikeBomb> bombs;
};

enum class EffectType {
  MonsterExplosion,
  WallImpact,
  DynamiteExplosion,
  AirstrikeExplosion
};

struct GameEffect {
  MapCoord coord;
  Uint32 started_ticks;
  EffectType type;
  int radius_cells;
};

class Game {
public:
  Game(Map *, Events *, Audio *);
  ~Game();
  bool is_won();
  bool is_lost();
  void StartSimulation();
  void Update();

protected:
private:
  Map *map;
  Events *events;
  Pacman *pacman;
  Audio *audio;
  std::vector<Monster *> monsters;
  std::vector<Goodie *> goodies;
  std::vector<std::thread> monster_threads;
  std::thread pacman_thread;
  int score;
  bool simulation_started;
  Uint32 death_started_ticks;
  MapCoord death_coord;
  std::vector<Fireball> fireballs;
  std::vector<GasCloud> gas_clouds;
  InvulnerabilityPotion invulnerability_potion;
  DynamitePickup dynamite_pickup;
  PlasticExplosivePickup plastic_explosive_pickup;
  WalkieTalkiePickup walkie_talkie_pickup;
  std::vector<PlacedDynamite> placed_dynamites;
  PlacedPlasticExplosive placed_plastic_explosive;
  bool plastic_explosive_is_armed;
  ActiveAirstrike active_airstrike;
  std::vector<ScheduledMonsterBlast> scheduled_monster_blasts;
  std::vector<GameEffect> effects;
  Uint32 last_update_ticks;
  int dynamite_inventory;
  int plastic_explosive_inventory;
  int airstrike_inventory;
  friend class Renderer;
  bool dead;
  bool win;

  void TriggerLoss(MapCoord coord, Uint32 now);
  void ApplyTeleporters();
  void TryTeleportElement(MapElement *element);
  void TryShootFireballs(Uint32 now);
  void TrySpawnGasClouds(Uint32 now);
  void ScheduleNextInvulnerabilityPotionSpawn(Uint32 now);
  void TrySpawnInvulnerabilityPotion(Uint32 now);
  void UpdateInvulnerabilityPotion(Uint32 now);
  void UpdateInvulnerability(Uint32 now);
  void ActivateInvulnerability(Uint32 now);
  void ScheduleNextDynamiteSpawn(Uint32 now);
  void TrySpawnDynamite(Uint32 now);
  void UpdateDynamitePickup(Uint32 now);
  void TryPlaceDynamite(Uint32 now);
  void ScheduleNextPlasticExplosiveSpawn(Uint32 now);
  void TrySpawnPlasticExplosive(Uint32 now);
  void UpdatePlasticExplosivePickup(Uint32 now);
  void TryUsePlasticExplosive(Uint32 now);
  void ScheduleNextWalkieTalkieSpawn(Uint32 now);
  void TrySpawnWalkieTalkie(Uint32 now);
  void UpdateWalkieTalkiePickup(Uint32 now);
  void TryUseAirstrike(Uint32 now);
  void UpdateAirstrike(Uint32 now);
  void UpdatePlacedDynamites(Uint32 now);
  void DetonateDynamite(const PlacedDynamite &dynamite, Uint32 now);
  void DetonatePlasticExplosive(const PlacedPlasticExplosive &explosive,
                                Uint32 now);
  void DetonateAirstrikeBomb(const AirstrikeBomb &bomb, Uint32 now);
  void UpdateScheduledMonsterBlasts(Uint32 now);
  void ScheduleMonsterBlast(Monster *monster, Uint32 trigger_ticks);
  bool IsCellFreeForDynamitePickup(MapCoord coord) const;
  bool IsCellFreeForPlasticExplosivePickup(MapCoord coord) const;
  bool IsCellFreeForWalkieTalkiePickup(MapCoord coord) const;
  bool CanPlacePlasticExplosiveAt(MapCoord coord) const;
  bool IsWithinRadius(MapCoord center, MapCoord target, int radius_cells) const;
  bool IsWithinDynamiteRadius(MapCoord center, MapCoord target) const;
  bool IsCellFreeForInvulnerabilityPotion(MapCoord coord) const;
  bool IsPacmanInvulnerable(Uint32 now) const;
  void UpdateFireballs(Uint32 now);
  void UpdateGasClouds(Uint32 now);
  void CleanupEffects(Uint32 now);
  void EliminateMonster(Monster *monster, Uint32 now);
};

#endif
