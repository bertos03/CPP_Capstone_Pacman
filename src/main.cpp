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
                       Uint32 &status_message_until, Audio *audio) {
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
      audio->PlayMenuMove();
      break;
    case SDLK_DOWN:
      selected_item = (selected_item + 1) % 3;
      audio->PlayMenuMove();
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      audio->PlayMenuSelect();
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

void processGameOverEvents(Events *events, bool &return_to_menu,
                           bool &quit_requested) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT || event.type == SDL_MOUSEBUTTONDOWN) {
      quit_requested = true;
      events->RequestQuit();
      return;
    }

    if (event.type == SDL_KEYDOWN) {
      return_to_menu = true;
      events->RequestQuit();
      return;
    }
  }
}

} // namespace

int main() {
  std::cout << "Bobman starting up ...\n";

  const int countdown_seconds = std::clamp(GAME_START_COUNTDOWN, 3, 9);
  bool quit_application = false;
  std::shared_ptr<Audio> audio(new Audio());

  while (!quit_application) {
    std::shared_ptr<Map> map(new Map());
    std::shared_ptr<Events> events(new Events());
    std::shared_ptr<Game> game(new Game(map.get(), events.get(), audio.get()));
    Renderer renderer(map.get(), game.get());

    int selected_item = MENU_START;
    bool start_requested = false;
    std::string status_message;
    Uint32 status_message_until = 0;

    while (!quit_application && !start_requested) {
      processMenuEvents(selected_item, start_requested, quit_application,
                        status_message, status_message_until, audio.get());

      const std::string visible_status =
          (SDL_GetTicks() < status_message_until) ? status_message : "";
      renderer.RenderStartMenu(selected_item, visible_status);
      sleep(40);
    }

    if (quit_application) {
      break;
    }

    for (int remaining = countdown_seconds; remaining > 0 && !quit_application;
         remaining--) {
      audio->PlayCountdownTick();
      const Uint32 second_started = SDL_GetTicks();
      while (!quit_application && SDL_GetTicks() - second_started < 1000) {
        processOverlayEvents(quit_application);
        renderer.RenderCountdown(remaining);
        sleep(16);
      }
    }

    if (quit_application) {
      break;
    }

    audio->PlayStartTrumpet();
    game->StartSimulation();

    bool return_to_menu = false;
    while (!events->is_quit() && !return_to_menu) {
      if (!game->is_lost() && !game->is_won()) {
        events->update();
        if (!events->is_quit()) {
          game->Update();
        }
      } else if (game->is_lost()) {
        processGameOverEvents(events.get(), return_to_menu, quit_application);
      } else {
        events->update();
        if (events->is_quit()) {
          quit_application = true;
        }
      }

      renderer.Render();
      sleep(40);
    }
  }

  std::cout << "Quitting app.\n";
}
