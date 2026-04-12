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
#include <vector>

#include "audio.h"
#include "events.h"
#include "game.h"
#include "globaltypes.h"
#include "map.h"
#include "renderer.h"

namespace {

enum MenuSelection {
  MENU_START = 0,
  MENU_MAP = 1,
  MENU_CONFIG = 2,
  MENU_END = 3,
};
enum ConfigSelection {
  CONFIG_MONSTER_AMOUNT = 0,
  CONFIG_BACK = 1,
};
enum class MenuScreen { Main, Config, MapSelection };

MonsterAmount NextMonsterAmount(MonsterAmount monster_amount) {
  switch (monster_amount) {
  case MonsterAmount::Few:
    return MonsterAmount::Medium;
  case MonsterAmount::Medium:
    return MonsterAmount::Many;
  case MonsterAmount::Many:
    return MonsterAmount::Few;
  default:
    return MonsterAmount::Many;
  }
}

std::vector<std::string>
GetMapDisplayNames(const std::vector<MapDefinition> &available_maps) {
  std::vector<std::string> names;
  names.reserve(available_maps.size());
  for (const auto &map : available_maps) {
    names.push_back(map.display_name);
  }
  return names;
}

void processMainMenuEvents(int &selected_item, MenuScreen &menu_screen,
                           bool &start_requested, bool &quit_requested,
                           Audio *audio) {
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
      selected_item = (selected_item + 3) % 4;
      audio->PlayMenuMove();
      break;
    case SDLK_DOWN:
      selected_item = (selected_item + 1) % 4;
      audio->PlayMenuMove();
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      audio->PlayMenuSelect();
      if (selected_item == MENU_START) {
        start_requested = true;
      } else if (selected_item == MENU_MAP) {
        menu_screen = MenuScreen::MapSelection;
      } else if (selected_item == MENU_CONFIG) {
        menu_screen = MenuScreen::Config;
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

void processConfigMenuEvents(int &selected_item, MonsterAmount &monster_amount,
                             MenuScreen &menu_screen, bool &quit_requested,
                             bool &rebuild_menu_session,
                             MenuScreen &next_menu_screen_after_rebuild,
                             Audio *audio) {
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
      selected_item = (selected_item + 1) % 2;
      audio->PlayMenuMove();
      break;
    case SDLK_DOWN:
      selected_item = (selected_item + 1) % 2;
      audio->PlayMenuMove();
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      audio->PlayMenuSelect();
      if (selected_item == CONFIG_MONSTER_AMOUNT) {
        monster_amount = NextMonsterAmount(monster_amount);
        rebuild_menu_session = true;
        next_menu_screen_after_rebuild = MenuScreen::Config;
      } else if (selected_item == CONFIG_BACK) {
        menu_screen = MenuScreen::Main;
      }
      break;
    case SDLK_ESCAPE:
      menu_screen = MenuScreen::Main;
      break;
    default:
      break;
    }
  }
}

void processMapSelectionEvents(
    int &active_map_index, int &selected_map_index, MenuScreen &menu_screen,
    bool &quit_requested, bool &rebuild_menu_session,
    MenuScreen &next_menu_screen_after_rebuild, Audio *audio,
    int map_count) {
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
      if (map_count > 1) {
        active_map_index = (active_map_index + map_count - 1) % map_count;
        audio->PlayMenuMove();
        rebuild_menu_session = true;
        next_menu_screen_after_rebuild = MenuScreen::MapSelection;
      }
      break;
    case SDLK_DOWN:
      if (map_count > 1) {
        active_map_index = (active_map_index + 1) % map_count;
        audio->PlayMenuMove();
        rebuild_menu_session = true;
        next_menu_screen_after_rebuild = MenuScreen::MapSelection;
      }
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      audio->PlayMenuSelect();
      selected_map_index = active_map_index;
      menu_screen = MenuScreen::Main;
      break;
    case SDLK_ESCAPE:
      if (active_map_index != selected_map_index) {
        active_map_index = selected_map_index;
        rebuild_menu_session = true;
        next_menu_screen_after_rebuild = MenuScreen::Main;
      } else {
        menu_screen = MenuScreen::Main;
      }
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

void processEndScreenEvents(Events *events, bool &return_to_menu,
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
  const std::vector<MapDefinition> available_maps = Map::DiscoverAvailableMaps();
  if (available_maps.empty()) {
    std::cerr << "No maps found in " << MAPS_DIRECTORY_PATH << "\n";
    return 1;
  }
  const std::vector<std::string> map_display_names =
      GetMapDisplayNames(available_maps);

  bool quit_application = false;
  MonsterAmount monster_amount = MonsterAmount::Many;
  int selected_map_index = 0;
  int active_map_index = selected_map_index;
  int main_selected_item = MENU_START;
  int config_selected_item = CONFIG_MONSTER_AMOUNT;
  MenuScreen next_menu_screen_after_rebuild = MenuScreen::Main;
  std::shared_ptr<Audio> audio(new Audio());

  while (!quit_application) {
    std::shared_ptr<Map> map(new Map(available_maps[active_map_index].file_path,
                                     monster_amount));
    std::shared_ptr<Events> events(new Events());
    std::shared_ptr<Game> game(new Game(map.get(), events.get(), audio.get()));
    Renderer renderer(map.get(), game.get());

    bool rebuild_menu_session = false;
    bool start_requested = false;
    MenuScreen menu_screen = next_menu_screen_after_rebuild;
    next_menu_screen_after_rebuild = MenuScreen::Main;

    while (!quit_application && !start_requested && !rebuild_menu_session) {
      if (menu_screen == MenuScreen::Main) {
        processMainMenuEvents(main_selected_item, menu_screen, start_requested,
                              quit_application, audio.get());
        renderer.RenderStartMenu(main_selected_item,
                                 available_maps[selected_map_index].display_name,
                                 "");
      } else if (menu_screen == MenuScreen::Config) {
        processConfigMenuEvents(config_selected_item, monster_amount,
                                menu_screen, quit_application,
                                rebuild_menu_session,
                                next_menu_screen_after_rebuild, audio.get());
        renderer.RenderConfigMenu(config_selected_item, monster_amount);
      } else {
        processMapSelectionEvents(active_map_index, selected_map_index,
                                  menu_screen, quit_application,
                                  rebuild_menu_session,
                                  next_menu_screen_after_rebuild, audio.get(),
                                  static_cast<int>(available_maps.size()));
        renderer.RenderMapSelectionMenu(map_display_names, active_map_index);
      }
      sleep(40);
    }

    if (quit_application) {
      break;
    }

    if (rebuild_menu_session) {
      continue;
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
      } else {
        processEndScreenEvents(events.get(), return_to_menu, quit_application);
      }

      renderer.Render();
      sleep(40);
    }
  }

  std::cout << "Quitting app.\n";
}
