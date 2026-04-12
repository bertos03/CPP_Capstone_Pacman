/*
 * PROJEKTKOMMENTAR (game.cpp)
 * ---------------------------------------------------------------------------
 * Diese Datei ist Teil von "BobMan", einem Pacman-inspirierten SDL2-Spiel.
 * Der Code in dieser Einheit kapselt einen klaren Verantwortungsbereich, damit
 * Einsteiger die Architektur schnell verstehen: Datenmodell (Map, Elemente),
 * Laufzeitlogik (Game, Events), Darstellung (Renderer) und optionale Audio-
 * Ausgabe.
 *
 * Wichtige Hinweise fuer Newbies:
 * - Header-Dateien deklarieren Klassen, Methoden und Datentypen.
 * - CPP-Dateien enthalten die konkrete Implementierung der Logik.
 * - Mehrere Threads bewegen Spielfiguren parallel; gemeinsame Daten werden
 *   deshalb kontrolliert gelesen/geschrieben.
 * - Makros in definitions.h steuern Ressourcenpfade, Farben und Features.
 */

#include "game.h"

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
