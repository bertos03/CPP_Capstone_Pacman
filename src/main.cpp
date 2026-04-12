/*
 * PROJEKTKOMMENTAR (main.cpp)
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

#include <SDL.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

#include "audio.h"
#include "events.h"
#include "game.h"
#include "globaltypes.h"
#include "map.h"
#include "renderer.h"

namespace {

enum MenuSelection { MENU_START = 0, MENU_CONFIG = 1, MENU_END = 2 };

void processMenuEvents(int &selected_item, bool &start_requested,
                       bool &quit_requested, std::string &status_message,
                       Uint32 &status_message_until) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT || event.type == SDL_MOUSEBUTTONDOWN) {
      quit_requested = true;
      continue;
    }

    if (event.type != SDL_KEYDOWN) {
      continue;
    }

    switch (event.key.keysym.sym) {
    case SDLK_UP:
      selected_item = (selected_item + 2) % 3;
      break;
    case SDLK_DOWN:
      selected_item = (selected_item + 1) % 3;
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      if (selected_item == MENU_START) {
        start_requested = true;
      } else if (selected_item == MENU_CONFIG) {
        status_message = "Konfiguration ist noch ohne Funktion.";
        status_message_until = SDL_GetTicks() + 1800;
      } else if (selected_item == MENU_END) {
        quit_requested = true;
      }
      break;
    case SDLK_ESCAPE:
      quit_requested = true;
      break;
    default:
      break;
    }
  }
}

void processOverlayEvents(bool &quit_requested) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT || event.type == SDL_MOUSEBUTTONDOWN) {
      quit_requested = true;
    }

    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
      quit_requested = true;
    }
  }
}

} // namespace

int main() {
  std::cout << "Bobman starting up ...\n";

  std::shared_ptr<Map> map(new Map());
  std::shared_ptr<Events> events(new Events());
  std::shared_ptr<Audio> audio(new Audio());
  std::shared_ptr<Game> game(new Game(map.get(), events.get(), audio.get()));
  Renderer renderer(map.get(), game.get());

  const int countdown_seconds = std::clamp(GAME_START_COUNTDOWN, 3, 9);
  int selected_item = MENU_START;
  bool start_requested = false;
  bool quit_requested = false;
  std::string status_message;
  Uint32 status_message_until = 0;

  while (!quit_requested && !start_requested) {
    processMenuEvents(selected_item, start_requested, quit_requested,
                      status_message, status_message_until);

    const std::string visible_status =
        (SDL_GetTicks() < status_message_until) ? status_message : "";
    renderer.RenderStartMenu(selected_item, visible_status);
    sleep(40);
  }

  if (!quit_requested && start_requested) {
    for (int remaining = countdown_seconds; remaining > 0 && !quit_requested;
         remaining--) {
      const Uint32 second_started = SDL_GetTicks();
      while (!quit_requested && SDL_GetTicks() - second_started < 1000) {
        processOverlayEvents(quit_requested);
        renderer.RenderCountdown(remaining);
        sleep(16);
      }
    }
  }

  if (!quit_requested && start_requested) {
    game->StartSimulation();

    while (!events->is_quit()) {
      events->update();
      if (!game->is_lost() && !game->is_won()) {
        game->Update();
      }
      renderer.Render();
      sleep(40);
    }
  }

  std::cout << "Quitting app.\n";
}
