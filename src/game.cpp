#include "game.h"

Game::Game(Map *_map, Events *_events, Audio *_audio)
    : map(_map), events(_events), audio(_audio) {
  win = false;
  dead = false;
  score = 0;
  // create Pacman object
  pacman = new Pacman(map->get_coord_pacman());

  // create Monster objects
  for (int i = 0; i < map->get_number_monsters(); i++) {
    monsters.emplace_back(new Monster(map->get_coord_monster(i), i));
  }

  // create Goodie objects
  for (int i = 0; i < map->get_number_goodies(); i++) {
    goodies.emplace_back(new Goodie(map->get_coord_goodie(i), i));
  }

  // start simulation loop for each monster in an own thread
  for (int i = 0; i < monsters.size(); i++) {
    monster_threads.emplace_back(
        std::thread(&Monster::simulate, monsters[i], events, map));
  }

  // start simulation loop for pacman in an own thread
  pacman_thread = std::thread(&Pacman::simulate, pacman, events, map);
};

void Game::Update() {
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

  // Check for collision with Monsters ... game is lost if collision occurs
  for (auto i : monsters) {
    if (pacman->map_coord.u == i->map_coord.u &&
        pacman->map_coord.v == i->map_coord.v) {
#ifdef AUDIO
      audio->PlayGameOver();
#endif
      dead = true;
    }
  }
}

Game::~Game() {
  // std::cout << "Waiting for threads to finish ... \n";
  for (int i = 0; i < monster_threads.size(); i++) {
    monster_threads[i].join();
  }
  pacman_thread.join();

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