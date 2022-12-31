#ifndef GAME_H
#define GAME_H

#include <vector>
#include <thread>
#include <mutex>

#include "events.h"
#include "mapelement.h"
#include "monster.h"
#include "goodie.h"
#include "pacman.h"
#include "renderer.h"
#include "map.h"
#include "audio.h"

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
  std::vector<Monster*> monsters;
  std::vector<Goodie*> goodies;
  std::vector<std::thread> monster_threads;
  std::thread pacman_thread;
  int score;
  friend class Renderer;
  bool dead;
  bool win;
};

#endif