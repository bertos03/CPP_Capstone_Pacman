/*
 * PROJEKTKOMMENTAR (pacman.cpp)
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

#include "pacman.h"

/**
 * @brief Construct a new Pacman:: Pacman object
 * 
 * @param _coord map coordinates for placing pacman
 */
Pacman::Pacman(MapCoord _coord) {
  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Creating Pacman\n";
  lck.unlock();
  map_coord = _coord;
  px_delta.x = 0;
  px_delta.y = 0;
  paralyzed_until_ticks = 0;
  teleport_animation_active = false;
  teleport_animation_started_ticks = 0;
  teleport_animation_from_coord = _coord;
  teleport_animation_to_coord = _coord;
  teleport_arrival_sound_played = false;
};

/**
 * @brief Destroy the Pacman:: Pacman object
 * 
 */
Pacman::~Pacman() {
  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Pacman is being destroyed \n";
  lck.unlock();
};

/**
 * @brief The simulation loop for pacman
 * 
 * @param events Events object to read keyboard inputs
 * @param map  Map object to check the possible options for pacman
 */
void Pacman::simulate(Events *events, Map *map) {
  std::vector<Directions> options;
  bool valid_choice = false;
  Directions next_move;

  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Simulating Pacman\n";
  lck.unlock();

  while (!events->is_quit()) {
    if (SDL_GetTicks() < paralyzed_until_ticks) {
      px_delta.x = 0;
      px_delta.y = 0;
      events->Keyreset();
      sleep(15);
      continue;
    }

    next_move = events->get_next_move();
    map->get_options(map_coord, options);
    events->Keyreset();
    sleep(5);
    for (auto i : options) {
      if (i == next_move) {
        switch (next_move) {
        case Directions::Up:
          for (int i = 0; i > -100 && SDL_GetTicks() >= paralyzed_until_ticks;
               i--) {
            px_delta.y = i;
            sleep(10 - SPEED_PACMAN);
          }
          if (SDL_GetTicks() < paralyzed_until_ticks) {
            px_delta.y = 0;
            break;
          }
          px_delta.y = 0;
          map_coord.u--;
          break;
        case Directions::Down:
          for (int i = 0; i < 100 && SDL_GetTicks() >= paralyzed_until_ticks;
               i++) {
            px_delta.y = i;
            sleep(10 - SPEED_PACMAN);
          }
          if (SDL_GetTicks() < paralyzed_until_ticks) {
            px_delta.y = 0;
            break;
          }
          px_delta.y = 0;
          map_coord.u++;
          break;
        case Directions::Left:
          for (int i = 0; i > -100 && SDL_GetTicks() >= paralyzed_until_ticks;
               i--) {
            px_delta.x = i;
            sleep(10 - SPEED_PACMAN);
          }
          if (SDL_GetTicks() < paralyzed_until_ticks) {
            px_delta.x = 0;
            break;
          }
          px_delta.x = 0;
          map_coord.v--;
          break;
        case Directions::Right:
          for (int i = 0; i < 100 && SDL_GetTicks() >= paralyzed_until_ticks;
               i++) {
            px_delta.x = i;
            sleep(10 - SPEED_PACMAN);
          }
          if (SDL_GetTicks() < paralyzed_until_ticks) {
            px_delta.x = 0;
            break;
          }
          px_delta.x = 0;
          map_coord.v++;
        default:
          break;
        };
      }
    }
  }
}
