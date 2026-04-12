/*
 * PROJEKTKOMMENTAR (monster.cpp)
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

#include "monster.h"


/**
 * @brief Construct a new Monster:: Monster object
 * 
 * @param _coord : the map coordinates for placing the monster
 * @param _id: A unique "monster ID" (currently not used)
 * @param _monster_char original monster tier marker from the map
 */
Monster::Monster(MapCoord _coord, int _id, char _monster_char) {
  id = _id;
  // std::cout << "Creating monster #" << id << "\n";
  map_coord = _coord;
  monster_char = _monster_char;
  px_delta.x = 0;
  px_delta.y = 0;
}

/**
 * @brief Simulation loop for monster object
 * 
 * @param events for reading keyboard input (signal for ending thread)
 * @param map for calculating the possible directions per move
 */
void Monster::simulate(Events *events, Map *map) {
  std::vector<Directions> options;
  Directions next_move;
  Directions last_move;
  std::unique_lock<std::mutex> lck(mtx);
  bool way_clear{false};
  // std::cout << "Simulating monster #" << id << "\n";
  lck.unlock();
  map->get_options(map_coord, options);
  next_move = options[rand() % options.size()];

  while (!events->is_quit()) {
    // carry out next monster move ... do it with a little dirty hack to enable
    // smooth moving
    switch (next_move) {
    case Directions::Up:
      for (int i = 0; i > -100; i--) {
        px_delta.y = i;
        sleep(10-SPEED_MONSTER);
      }
      px_delta.y = 0;
      map_coord.u--;
      break;
    case Directions::Down:
      for (int i = 0; i < 100; i++) {
        px_delta.y = i;
        sleep(10-SPEED_MONSTER);
      }
      px_delta.y = 0;
      map_coord.u++;
      break;
    case Directions::Left:
      for (int i = 0; i > -100; i--) {
        px_delta.x = i;
        sleep(10-SPEED_MONSTER);
      }
      px_delta.x = 0;
      map_coord.v--;
      break;
    case Directions::Right:
      for (int i = 0; i < 100; i++) {
        px_delta.x = i;
        sleep(10-SPEED_MONSTER);
      }
      px_delta.x = 0;
      map_coord.v++;
    default:
      break;
    };

    
    map->get_options(map_coord, options);
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
