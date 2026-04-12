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

enum class EffectType { MonsterExplosion, WallImpact };

struct GameEffect {
  MapCoord coord;
  Uint32 started_ticks;
  EffectType type;
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
  std::vector<GameEffect> effects;
  Uint32 last_update_ticks;
  friend class Renderer;
  bool dead;
  bool win;

  void TriggerLoss(MapCoord coord, Uint32 now);
  void ApplyTeleporters();
  void TryTeleportElement(MapElement *element);
  void TryShootFireballs(Uint32 now);
  void TrySpawnGasClouds(Uint32 now);
  void UpdateFireballs(Uint32 now);
  void UpdateGasClouds(Uint32 now);
  void CleanupEffects(Uint32 now);
  void EliminateMonster(Monster *monster, Uint32 now);
};

#endif
