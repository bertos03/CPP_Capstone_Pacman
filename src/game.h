#ifndef GAME_H
#define GAME_H

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

class Game {
public:
  Game(Map *, Events *, Audio *);
  ~Game();
  bool is_won();
  bool is_lost();
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
  friend class Renderer;
  bool dead;
  bool win;
};

#endif