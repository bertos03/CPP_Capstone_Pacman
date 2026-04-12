/*
 * PROJEKTKOMMENTAR (events.cpp)
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

#include "events.h"

/**
 * @brief Construct a new Events:: Events object
 * Events object takes care of all keyboard inputs which are required for the
 * game
 */
Events::Events() {
  sdl_events = new SDL_Event;
  quit = false;
  current_direction = Directions::None;
}

/**
 * @brief Destroy the Events:: Events object
 *
 */
Events::~Events() { delete sdl_events; }

/**
 * @brief Make sure there is only one pacman move per key press
 *
 */
void Events::Keyreset() { current_direction = Directions::None; }

void Events::RequestQuit() { quit = true; }

/**
 * @brief Updates the input buffer each cycle
 *
 */
void Events::update() {
  SDL_PollEvent(sdl_events);
  if (sdl_events->type == SDL_QUIT) {
    quit = true;
  }

  if (sdl_events->type == SDL_MOUSEBUTTONDOWN) {
   quit = true;
  }
  if (sdl_events->type == SDL_KEYDOWN) {
    switch (sdl_events->key.keysym.sym) {
    case SDLK_UP:
      current_direction = Directions::Up;
      break;
    case SDLK_DOWN:
      current_direction = Directions::Down;
      break;
    case SDLK_LEFT:
      current_direction = Directions::Left;
      break;
    case SDLK_RIGHT:
      current_direction = Directions::Right;
      break;
    case SDLK_ESCAPE: // Quit the program with escape key
      quit = true;
      break;
    default:
      current_direction = Directions::None;
    }
  }
}
