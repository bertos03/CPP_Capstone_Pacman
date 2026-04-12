/*
 * PROJEKTKOMMENTAR (renderer.cpp)
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

#include "renderer.h"

#include <algorithm>
#include <cmath>

#include "game.h"
#include "goodie.h"
#include "map.h"
#include "monster.h"
#include "pacman.h"

namespace {

const SDL_Color kHudTextColor{243, 236, 222, 255};
const SDL_Color kMenuTextColor{225, 223, 218, 255};
const SDL_Color kSelectedMenuTextColor{255, 248, 238, 255};
const SDL_Color kStatusTextColor{255, 189, 163, 255};
const SDL_Color kWarningTextColor{255, 110, 110, 255};
const SDL_Color kPanelFillColor{10, 6, 18, 205};
const SDL_Color kPanelBorderColor{196, 130, 92, 255};
const SDL_Color kBrickOutlineColor{78, 20, 14, 255};
const SDL_Color kFewMonsterGlowColor{120, 210, 255, 255};
const SDL_Color kMediumMonsterGlowColor{255, 170, 72, 255};
const SDL_Color kManyMonsterGlowColor{255, 82, 82, 255};
const SDL_Color kTeleporterBlue{78, 160, 255, 255};
const SDL_Color kTeleporterRed{255, 104, 104, 255};
const SDL_Color kTeleporterGreen{110, 205, 118, 255};
const SDL_Color kTeleporterGrey{172, 176, 186, 255};
const SDL_Color kTeleporterYellow{244, 214, 88, 255};

const char *MonsterAmountLabel(MonsterAmount monster_amount) {
  switch (monster_amount) {
  case MonsterAmount::Few:
    return "wenig";
  case MonsterAmount::Medium:
    return "mittel";
  case MonsterAmount::Many:
    return "viel";
  default:
    return "mittel";
  }
}

SDL_Color MonsterGlowColor(char monster_char) {
  switch (monster_char) {
  case MONSTER_FEW:
    return kFewMonsterGlowColor;
  case MONSTER_MEDIUM:
    return kMediumMonsterGlowColor;
  case MONSTER_MANY:
  default:
    return kManyMonsterGlowColor;
  }
}

SDL_Color TeleporterColor(char teleporter_digit) {
  switch (teleporter_digit) {
  case '1':
    return kTeleporterBlue;
  case '2':
    return kTeleporterRed;
  case '3':
    return kTeleporterGreen;
  case '4':
    return kTeleporterGrey;
  case '5':
  default:
    return kTeleporterYellow;
  }
}

MapCoord StepRenderCoord(MapCoord coord, Directions direction) {
  switch (direction) {
  case Directions::Up:
    coord.u--;
    break;
  case Directions::Down:
    coord.u++;
    break;
  case Directions::Left:
    coord.v--;
    break;
  case Directions::Right:
    coord.v++;
    break;
  default:
    break;
  }
  return coord;
}

} // namespace

Renderer::Renderer(Map *_map, Game *_game)
    : screen_res_x(0), screen_res_y(0), element_size(0), offset_x(0),
      offset_y(0), rows(0), cols(0), map(_map), game(_game),
      sdl_window(nullptr), sdl_renderer(nullptr), sdl_wall_surface(nullptr),
      sdl_wall_texture(nullptr), sdl_goodie_surface(nullptr),
      sdl_goodie_texture(nullptr), sdl_pacman_surface(nullptr),
      sdl_pacman_texture(nullptr), sdl_logo_brick_surface(nullptr),
      sdl_font_hud(nullptr), sdl_font_menu(nullptr), sdl_font_logo(nullptr),
      sdl_font_display(nullptr), sdl_font_color{255, 255, 255, 255},
      sdl_font_back_color{COLOR_BACK, 255}, texW(0), texH(0) {
  initializeRenderer(map->get_map_rows(), map->get_map_cols());
}

Renderer::Renderer(size_t row_count, size_t col_count)
    : screen_res_x(0), screen_res_y(0), element_size(0), offset_x(0),
      offset_y(0), rows(0), cols(0), map(nullptr), game(nullptr),
      sdl_window(nullptr), sdl_renderer(nullptr), sdl_wall_surface(nullptr),
      sdl_wall_texture(nullptr), sdl_goodie_surface(nullptr),
      sdl_goodie_texture(nullptr), sdl_pacman_surface(nullptr),
      sdl_pacman_texture(nullptr), sdl_logo_brick_surface(nullptr),
      sdl_font_hud(nullptr), sdl_font_menu(nullptr), sdl_font_logo(nullptr),
      sdl_font_display(nullptr), sdl_font_color{255, 255, 255, 255},
      sdl_font_back_color{COLOR_BACK, 255}, texW(0), texH(0) {
  initializeRenderer(row_count, col_count);
}

void Renderer::initializeRenderer(size_t row_count_value,
                                  size_t col_count_value) {
  if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0 &&
      SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL video could not initialize.\n";
    std::cerr << "SDL_Error: " << SDL_GetError() << "\n";
    exit(1);
  }

  if (TTF_Init() < 0) {
    std::cerr << "SDL_ttf could not initialize.\n";
    std::cerr << "SDL_Error: " << TTF_GetError() << "\n";
    exit(1);
  }

  const int image_flags = IMG_INIT_PNG;
  if ((IMG_Init(image_flags) & image_flags) != image_flags) {
    std::cerr << "SDL_image PNG support not fully available: "
              << IMG_GetError() << "\n";
  }

  SDL_DisplayMode display_mode;
  SDL_GetCurrentDisplayMode(0, &display_mode);
  screen_res_x = display_mode.w;
  screen_res_y = display_mode.h;
  rows = row_count_value;
  cols = col_count_value;
  const int row_count = static_cast<int>(rows);
  const int col_count = static_cast<int>(cols);

  const int hud_fontsize = std::max(22, screen_res_y / 35);
  const int reserved_top_space = hud_fontsize * 2;

  const int element_size_x =
      (screen_res_y - row_count - 1 - reserved_top_space) /
      std::max(1, row_count);
  const int element_size_y = (screen_res_x - col_count - 1) /
                             std::max(1, col_count);
  element_size = std::max(8, std::min(element_size_x, element_size_y));

  offset_x = (screen_res_x - static_cast<int>((cols + 1) * element_size)) / 2;
  offset_y =
      (screen_res_y - static_cast<int>((rows + 1) * element_size)) / 2 +
      reserved_top_space;

  sdl_window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, display_mode.w,
                                display_mode.h, SDL_WINDOW_BORDERLESS);
  if (sdl_window == nullptr) {
    std::cerr << "Window could not be created.\n";
    std::cerr << "SDL_Error: " << SDL_GetError() << "\n";
    exit(1);
  }

  sdl_renderer = SDL_CreateRenderer(
      sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (sdl_renderer == nullptr) {
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (sdl_renderer == nullptr) {
    std::cerr << "Renderer could not be created.\n";
    std::cerr << "SDL_Error: " << SDL_GetError() << "\n";
    exit(1);
  }

  SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(sdl_renderer, COLOR_BACK, 255);
  SDL_RenderClear(sdl_renderer);

  sdl_font_hud = TTF_OpenFont(FONT_PATH, hud_fontsize);
  sdl_font_menu = TTF_OpenFont(FONT_PATH, std::max(30, screen_res_y / 22));
  sdl_font_logo = TTF_OpenFont(FONT_PATH, std::max(84, screen_res_y / 5));
  sdl_font_display = TTF_OpenFont(FONT_PATH, std::max(72, screen_res_y / 7));

  if (sdl_font_hud == nullptr || sdl_font_menu == nullptr ||
      sdl_font_logo == nullptr || sdl_font_display == nullptr) {
    std::cerr << "Could not open font: " << TTF_GetError() << "\n";
    exit(1);
  }

  sdl_font_color = kHudTextColor;

  sdl_wall_surface = SDL_LoadBMP(WALL_PATH);
  sdl_goodie_surface = SDL_LoadBMP(GOODIE_PATH);
  sdl_pacman_surface = SDL_LoadBMP(PACMAN_PATH);

  const std::vector<const char *> monster_frame_paths{
      MONSTER_PATH, MONSTER_FRAME_1_PATH, MONSTER_FRAME_2_PATH,
      MONSTER_FRAME_3_PATH};
  for (const char *monster_frame_path : monster_frame_paths) {
    SDL_Surface *monster_surface = SDL_LoadBMP(monster_frame_path);
    if (monster_surface == nullptr) {
      std::cerr << "Could not open monster sprite asset "
                << monster_frame_path << ": " << SDL_GetError() << "\n";
      exit(1);
    }
    sdl_monster_surfaces.push_back(monster_surface);
  }

  if (sdl_goodie_surface == nullptr || sdl_wall_surface == nullptr ||
      sdl_pacman_surface == nullptr) {
    std::cerr << "Could not open sprite assets: " << SDL_GetError() << "\n";
    exit(1);
  }

  sdl_wall_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_wall_surface);
  sdl_goodie_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_goodie_surface);
  sdl_pacman_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_pacman_surface);
  for (SDL_Surface *monster_surface : sdl_monster_surfaces) {
    SDL_Texture *monster_texture =
        SDL_CreateTextureFromSurface(sdl_renderer, monster_surface);
    if (monster_texture == nullptr) {
      std::cerr << "Could not create monster texture: " << SDL_GetError()
                << "\n";
      exit(1);
    }
    sdl_monster_textures.push_back(monster_texture);
  }

  SDL_Surface *brick_surface = IMG_Load(LOGO_BRICK_TEXTURE_PATH);
  if (brick_surface == nullptr) {
    brick_surface = SDL_LoadBMP(WALL_PATH);
  }
  if (brick_surface == nullptr) {
    std::cerr << "Could not open brick texture for logo: " << IMG_GetError()
              << "\n";
    exit(1);
  }
  sdl_logo_brick_surface =
      SDL_ConvertSurfaceFormat(brick_surface, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(brick_surface);

  if (sdl_logo_brick_surface == nullptr) {
    std::cerr << "Could not convert brick texture: " << SDL_GetError() << "\n";
    exit(1);
  }
}

Renderer::~Renderer() {
  if (sdl_wall_texture != nullptr) {
    SDL_DestroyTexture(sdl_wall_texture);
  }
  if (sdl_goodie_texture != nullptr) {
    SDL_DestroyTexture(sdl_goodie_texture);
  }
  for (SDL_Texture *monster_texture : sdl_monster_textures) {
    if (monster_texture != nullptr) {
      SDL_DestroyTexture(monster_texture);
    }
  }
  if (sdl_pacman_texture != nullptr) {
    SDL_DestroyTexture(sdl_pacman_texture);
  }
  if (sdl_renderer != nullptr) {
    SDL_DestroyRenderer(sdl_renderer);
  }
  if (sdl_window != nullptr) {
    SDL_DestroyWindow(sdl_window);
  }

  if (sdl_wall_surface != nullptr) {
    SDL_FreeSurface(sdl_wall_surface);
  }
  if (sdl_goodie_surface != nullptr) {
    SDL_FreeSurface(sdl_goodie_surface);
  }
  for (SDL_Surface *monster_surface : sdl_monster_surfaces) {
    if (monster_surface != nullptr) {
      SDL_FreeSurface(monster_surface);
    }
  }
  if (sdl_pacman_surface != nullptr) {
    SDL_FreeSurface(sdl_pacman_surface);
  }
  if (sdl_logo_brick_surface != nullptr) {
    SDL_FreeSurface(sdl_logo_brick_surface);
  }

  if (sdl_font_hud != nullptr) {
    TTF_CloseFont(sdl_font_hud);
  }
  if (sdl_font_menu != nullptr) {
    TTF_CloseFont(sdl_font_menu);
  }
  if (sdl_font_logo != nullptr) {
    TTF_CloseFont(sdl_font_logo);
  }
  if (sdl_font_display != nullptr) {
    TTF_CloseFont(sdl_font_display);
  }

  IMG_Quit();
  TTF_Quit();
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void Renderer::Render() {
  renderFrame(true);
  SDL_RenderPresent(sdl_renderer);
}

void Renderer::RenderStartMenu(int selected_item, const std::string &map_name,
                               const std::string &status_message) {
  renderFrame(false);
  drawDimmer(120);
  drawStartMenuOverlay(selected_item, map_name, status_message);
  SDL_RenderPresent(sdl_renderer);
}

void Renderer::RenderConfigMenu(int selected_item,
                                MonsterAmount monster_amount) {
  renderFrame(false);
  drawDimmer(120);
  drawConfigMenuOverlay(selected_item, monster_amount);
  SDL_RenderPresent(sdl_renderer);
}

void Renderer::RenderMapSelectionMenu(const std::vector<std::string> &map_names,
                                      int selected_index) {
  renderFrame(false);
  drawDimmer(120);
  drawMapSelectionOverlay(map_names, selected_index);
  SDL_RenderPresent(sdl_renderer);
}

void Renderer::RenderEditorSelectionMenu(const std::vector<std::string> &items,
                                         int selected_index) {
  renderFrame(false);
  drawDimmer(120);
  drawEditorSelectionOverlay(items, selected_index);
  SDL_RenderPresent(sdl_renderer);
}

void Renderer::RenderEditorSizeSelectionMenu(int selected_index) {
  renderFrame(false);
  drawDimmer(120);
  drawEditorSizeSelectionOverlay(selected_index);
  SDL_RenderPresent(sdl_renderer);
}

void Renderer::RenderEditor(const std::vector<std::string> &layout,
                            const std::string &map_name, MapCoord cursor,
                            const std::string &warning_message,
                            bool show_exit_dialog, int exit_dialog_selected,
                            bool show_name_dialog,
                            const std::string &name_input,
                            const std::string &name_dialog_message) {
  renderLayoutFrame(layout);
  drawDimmer(20);
  drawEditorOverlay(map_name, cursor, warning_message, show_exit_dialog,
                    exit_dialog_selected, show_name_dialog, name_input,
                    name_dialog_message);
  SDL_RenderPresent(sdl_renderer);
}

void Renderer::RenderCountdown(int seconds_left) {
  renderFrame(false);
  drawDimmer(145);
  drawCountdownOverlay(seconds_left);
  SDL_RenderPresent(sdl_renderer);
}

void Renderer::renderFrame(bool show_hud) {
  SDL_SetRenderDrawColor(sdl_renderer, COLOR_BACK, 255);
  SDL_RenderClear(sdl_renderer);
  drawmap();
  drawteleporters();
  if (game != nullptr) {
    drawgoodies();
    drawpacman();
    drawmonsters();
    drawgasclouds();
    drawfireballs();
    draweffects();
  } else {
    drawStaticGoodies();
    drawStaticPacman();
    drawStaticMonsters();
  }
  if (show_hud) {
    drawhud();
  }
}

void Renderer::renderLayoutFrame(const std::vector<std::string> &layout) {
  SDL_SetRenderDrawColor(sdl_renderer, COLOR_BACK, 255);
  SDL_RenderClear(sdl_renderer);
  drawLayout(layout);
  drawLayoutTeleporters(layout);
  drawLayoutEntities(layout);
}

void Renderer::drawhud() {
  if (game == nullptr) {
    return;
  }

  if (game->dead) {
    renderBrickText(sdl_font_display, "Game Over", screen_res_x / 2,
                    (screen_res_y - TTF_FontHeight(sdl_font_display)) / 2,
                    kBrickOutlineColor);
    return;
  }

  const std::string title_text =
      "BOBMAN        Score: " + std::to_string(game->score);
  renderSimpleText(sdl_font_hud, title_text, sdl_font_color, screen_res_x / 2,
                   30);

  if (game->win) {
    renderBrickText(sdl_font_display, "You Won", screen_res_x / 2,
                    (screen_res_y - TTF_FontHeight(sdl_font_display)) / 2,
                    kBrickOutlineColor);
  }
}

void Renderer::drawDimmer(Uint8 alpha) {
  SDL_Rect screen_rect{0, 0, screen_res_x, screen_res_y};
  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, alpha);
  SDL_RenderFillRect(sdl_renderer, &screen_rect);
}

void Renderer::drawPanel(const SDL_Rect &panel, const SDL_Color &fill_color,
                         const SDL_Color &border_color) {
  SDL_SetRenderDrawColor(sdl_renderer, fill_color.r, fill_color.g, fill_color.b,
                         fill_color.a);
  SDL_RenderFillRect(sdl_renderer, &panel);

  SDL_SetRenderDrawColor(sdl_renderer, border_color.r, border_color.g,
                         border_color.b, border_color.a);
  SDL_RenderDrawRect(sdl_renderer, &panel);

  SDL_Rect inner_panel{panel.x + 6, panel.y + 6, panel.w - 12, panel.h - 12};
  SDL_SetRenderDrawColor(sdl_renderer, border_color.r, border_color.g,
                         border_color.b, 100);
  SDL_RenderDrawRect(sdl_renderer, &inner_panel);
}

void Renderer::drawStartMenuOverlay(int selected_item,
                                    const std::string &map_name,
                                    const std::string &status_message) {
  const int logo_top = screen_res_y / 18;
  renderBrickText(sdl_font_logo, "Bobman", screen_res_x / 2, logo_top,
                  kBrickOutlineColor);

  const std::vector<std::string> menu_items{
      "Start Spiel", "Karte: " + map_name, "Map Editor", "Konfiguration",
      "Ende"};
  const int item_height = std::max(52, TTF_FontHeight(sdl_font_menu) + 18);
  const int panel_width = std::min(780, screen_res_x * 46 / 100);
  const int panel_height =
      std::max(360, 88 + static_cast<int>(menu_items.size()) * item_height);
  const int panel_top =
      std::min(screen_res_y - panel_height - 70,
               logo_top + TTF_FontHeight(sdl_font_logo) + screen_res_y / 20);
  SDL_Rect panel{(screen_res_x - panel_width) / 2, panel_top, panel_width,
                 panel_height};

  drawPanel(panel, kPanelFillColor, kPanelBorderColor);

  const int highlight_width = panel.w - 56;
  const int item_start_y =
      panel.y + 22 + std::max(0, (panel.h - 44 - static_cast<int>(menu_items.size()) *
                                               item_height) /
                                       2);

  for (int i = 0; i < static_cast<int>(menu_items.size()); i++) {
    const SDL_Rect highlight_rect{panel.x + 28, item_start_y + i * item_height,
                                  highlight_width, item_height - 8};
    if (i == selected_item) {
      SDL_SetRenderDrawColor(sdl_renderer, 138, 46, 29, 185);
      SDL_RenderFillRect(sdl_renderer, &highlight_rect);
      SDL_SetRenderDrawColor(sdl_renderer, 235, 182, 140, 255);
      SDL_RenderDrawRect(sdl_renderer, &highlight_rect);

      SDL_Rect accent_rect{highlight_rect.x + 10, highlight_rect.y + 8, 10,
                           highlight_rect.h - 16};
      SDL_SetRenderDrawColor(sdl_renderer, 255, 215, 160, 255);
      SDL_RenderFillRect(sdl_renderer, &accent_rect);
    }

    const SDL_Color item_color =
        (i == selected_item) ? kSelectedMenuTextColor : kMenuTextColor;
    const int text_top =
        highlight_rect.y +
        (highlight_rect.h - TTF_FontHeight(sdl_font_menu)) / 2 - 2;
    renderSimpleText(sdl_font_menu, menu_items[i], item_color, screen_res_x / 2,
                     text_top);
  }

  if (!status_message.empty()) {
    renderSimpleText(sdl_font_hud, status_message, kStatusTextColor,
                     screen_res_x / 2,
                     panel.y + panel.h - TTF_FontHeight(sdl_font_hud) - 22);
  }
}

void Renderer::drawConfigMenuOverlay(int selected_item,
                                     MonsterAmount monster_amount) {
  const int logo_top = screen_res_y / 18;
  renderBrickText(sdl_font_logo, "Bobman", screen_res_x / 2, logo_top,
                  kBrickOutlineColor);
  renderSimpleText(sdl_font_menu, "Konfiguration", kHudTextColor,
                   screen_res_x / 2,
                   logo_top + TTF_FontHeight(sdl_font_logo) - 4);

  const int panel_width = std::min(860, screen_res_x * 52 / 100);
  const int panel_height = std::max(330, screen_res_y * 38 / 100);
  const int panel_top =
      std::min(screen_res_y - panel_height - 70,
               logo_top + TTF_FontHeight(sdl_font_logo) + screen_res_y / 14);
  SDL_Rect panel{(screen_res_x - panel_width) / 2, panel_top, panel_width,
                 panel_height};

  drawPanel(panel, kPanelFillColor, kPanelBorderColor);

  const std::vector<std::string> menu_items{
      "Monsteranzahl", "Zurueck"};
  const std::vector<std::string> value_items{
      MonsterAmountLabel(monster_amount), ""};
  const int item_height = std::max(62, TTF_FontHeight(sdl_font_menu) + 24);
  const int highlight_width = panel.w - 56;
  const int item_start_y =
      panel.y + 32 +
      std::max(0, (panel.h - 86 - static_cast<int>(menu_items.size()) * item_height) /
                       2);

  for (int i = 0; i < static_cast<int>(menu_items.size()); i++) {
    const SDL_Rect highlight_rect{panel.x + 28, item_start_y + i * item_height,
                                  highlight_width, item_height - 8};
    if (i == selected_item) {
      SDL_SetRenderDrawColor(sdl_renderer, 138, 46, 29, 185);
      SDL_RenderFillRect(sdl_renderer, &highlight_rect);
      SDL_SetRenderDrawColor(sdl_renderer, 235, 182, 140, 255);
      SDL_RenderDrawRect(sdl_renderer, &highlight_rect);
    }

    const SDL_Color item_color =
        (i == selected_item) ? kSelectedMenuTextColor : kMenuTextColor;
    const int text_top =
        highlight_rect.y +
        (highlight_rect.h - TTF_FontHeight(sdl_font_menu)) / 2 - 2;

    if (i == 0) {
      renderSimpleText(sdl_font_menu, menu_items[i], item_color,
                       panel.x + panel.w / 3, text_top);
      renderSimpleText(sdl_font_menu, value_items[i], item_color,
                       panel.x + panel.w * 3 / 4, text_top);
    } else {
      renderSimpleText(sdl_font_menu, menu_items[i], item_color, screen_res_x / 2,
                       text_top);
    }
  }
}

void Renderer::drawMapSelectionOverlay(const std::vector<std::string> &map_names,
                                       int selected_index) {
  const int logo_top = screen_res_y / 18;
  renderBrickText(sdl_font_logo, "Bobman", screen_res_x / 2, logo_top,
                  kBrickOutlineColor);
  renderSimpleText(sdl_font_menu, "Kartenauswahl", kHudTextColor,
                   screen_res_x / 2,
                   logo_top + TTF_FontHeight(sdl_font_logo) - 4);

  const int item_height = std::max(52, TTF_FontHeight(sdl_font_menu) + 18);
  const int panel_width = std::min(860, screen_res_x * 50 / 100);
  const int desired_panel_height =
      136 + static_cast<int>(map_names.size()) * item_height;
  const int panel_height = std::min(std::max(320, desired_panel_height),
                                    screen_res_y * 72 / 100);
  const int panel_top =
      std::min(screen_res_y - panel_height - 70,
               logo_top + TTF_FontHeight(sdl_font_logo) + screen_res_y / 14);
  SDL_Rect panel{(screen_res_x - panel_width) / 2, panel_top, panel_width,
                 panel_height};

  drawPanel(panel, kPanelFillColor, kPanelBorderColor);

  const int highlight_width = panel.w - 56;
  const int max_visible_items =
      std::max(1, (panel.h - 72) / std::max(1, item_height));
  const int first_visible =
      std::clamp(selected_index - max_visible_items / 2, 0,
                 std::max(0, static_cast<int>(map_names.size()) - max_visible_items));
  const int visible_items =
      std::min(max_visible_items,
               static_cast<int>(map_names.size()) - first_visible);
  const int item_start_y =
      panel.y + 28 +
      std::max(0, (panel.h - 72 - visible_items * item_height) / 2);

  for (int visible_index = 0; visible_index < visible_items; visible_index++) {
    const int i = first_visible + visible_index;
    const SDL_Rect highlight_rect{panel.x + 28,
                                  item_start_y + visible_index * item_height,
                                  highlight_width, item_height - 8};
    if (i == selected_index) {
      SDL_SetRenderDrawColor(sdl_renderer, 138, 46, 29, 185);
      SDL_RenderFillRect(sdl_renderer, &highlight_rect);
      SDL_SetRenderDrawColor(sdl_renderer, 235, 182, 140, 255);
      SDL_RenderDrawRect(sdl_renderer, &highlight_rect);
    }

    const SDL_Color item_color =
        (i == selected_index) ? kSelectedMenuTextColor : kMenuTextColor;
    const int text_top =
        highlight_rect.y +
        (highlight_rect.h - TTF_FontHeight(sdl_font_menu)) / 2 - 2;
    renderSimpleText(sdl_font_menu, map_names[i], item_color, screen_res_x / 2,
                     text_top);
  }
  if (first_visible > 0) {
    renderSimpleText(sdl_font_hud, "...", kHudTextColor, screen_res_x / 2,
                     panel.y + 6);
  }
  if (first_visible + visible_items < static_cast<int>(map_names.size())) {
    renderSimpleText(sdl_font_hud, "...", kHudTextColor, screen_res_x / 2,
                     panel.y + panel.h - TTF_FontHeight(sdl_font_hud) - 8);
  }
}

void Renderer::drawEditorSelectionOverlay(const std::vector<std::string> &items,
                                          int selected_index) {
  const int logo_top = screen_res_y / 18;
  renderBrickText(sdl_font_logo, "Bobman", screen_res_x / 2, logo_top,
                  kBrickOutlineColor);
  renderSimpleText(sdl_font_menu, "Map Editor", kHudTextColor, screen_res_x / 2,
                   logo_top + TTF_FontHeight(sdl_font_logo) - 4);

  const int item_height = std::max(50, TTF_FontHeight(sdl_font_menu) + 16);
  const int panel_width = std::min(900, screen_res_x * 56 / 100);
  const int desired_panel_height =
      138 + static_cast<int>(items.size()) * item_height;
  const int panel_height = std::min(std::max(340, desired_panel_height),
                                    screen_res_y * 74 / 100);
  const int panel_top =
      std::min(screen_res_y - panel_height - 70,
               logo_top + TTF_FontHeight(sdl_font_logo) + screen_res_y / 14);
  SDL_Rect panel{(screen_res_x - panel_width) / 2, panel_top, panel_width,
                 panel_height};

  drawPanel(panel, kPanelFillColor, kPanelBorderColor);

  const int highlight_width = panel.w - 56;
  const int max_visible_items =
      std::max(1, (panel.h - 78) / std::max(1, item_height));
  const int first_visible =
      std::clamp(selected_index - max_visible_items / 2, 0,
                 std::max(0, static_cast<int>(items.size()) - max_visible_items));
  const int visible_items =
      std::min(max_visible_items,
               static_cast<int>(items.size()) - first_visible);
  const int item_start_y =
      panel.y + 30 +
      std::max(0, (panel.h - 78 - visible_items * item_height) / 2);

  for (int visible_index = 0; visible_index < visible_items; visible_index++) {
    const int i = first_visible + visible_index;
    const SDL_Rect highlight_rect{panel.x + 28,
                                  item_start_y + visible_index * item_height,
                                  highlight_width, item_height - 8};
    if (i == selected_index) {
      SDL_SetRenderDrawColor(sdl_renderer, 138, 46, 29, 185);
      SDL_RenderFillRect(sdl_renderer, &highlight_rect);
      SDL_SetRenderDrawColor(sdl_renderer, 235, 182, 140, 255);
      SDL_RenderDrawRect(sdl_renderer, &highlight_rect);
    }

    const SDL_Color item_color =
        (i == selected_index) ? kSelectedMenuTextColor : kMenuTextColor;
    const int text_top =
        highlight_rect.y +
        (highlight_rect.h - TTF_FontHeight(sdl_font_menu)) / 2 - 2;
    renderSimpleText(sdl_font_menu, items[i], item_color, screen_res_x / 2,
                     text_top);
  }
  if (first_visible > 0) {
    renderSimpleText(sdl_font_hud, "...", kHudTextColor, screen_res_x / 2,
                     panel.y + 6);
  }
  if (first_visible + visible_items < static_cast<int>(items.size())) {
    renderSimpleText(sdl_font_hud, "...", kHudTextColor, screen_res_x / 2,
                     panel.y + panel.h - TTF_FontHeight(sdl_font_hud) - 8);
  }
}

void Renderer::drawEditorSizeSelectionOverlay(int selected_index) {
  const int logo_top = screen_res_y / 18;
  renderBrickText(sdl_font_logo, "Bobman", screen_res_x / 2, logo_top,
                  kBrickOutlineColor);
  renderSimpleText(sdl_font_menu, "Neue Karte", kHudTextColor,
                   screen_res_x / 2,
                   logo_top + TTF_FontHeight(sdl_font_logo) - 4);

  const std::vector<std::string> items{"Klein", "Mittel", "Gross"};
  const int item_height = std::max(56, TTF_FontHeight(sdl_font_menu) + 18);
  const int panel_width = std::min(740, screen_res_x * 46 / 100);
  const int panel_height = std::max(340, screen_res_y * 40 / 100);
  const int panel_top =
      std::min(screen_res_y - panel_height - 70,
               logo_top + TTF_FontHeight(sdl_font_logo) + screen_res_y / 14);
  SDL_Rect panel{(screen_res_x - panel_width) / 2, panel_top, panel_width,
                 panel_height};

  drawPanel(panel, kPanelFillColor, kPanelBorderColor);

  const int highlight_width = panel.w - 56;
  const int item_start_y =
      panel.y + 38 +
      std::max(0, (panel.h - 94 - static_cast<int>(items.size()) * item_height) /
                       2);

  for (int i = 0; i < static_cast<int>(items.size()); i++) {
    const SDL_Rect highlight_rect{panel.x + 28, item_start_y + i * item_height,
                                  highlight_width, item_height - 8};
    if (i == selected_index) {
      SDL_SetRenderDrawColor(sdl_renderer, 138, 46, 29, 185);
      SDL_RenderFillRect(sdl_renderer, &highlight_rect);
      SDL_SetRenderDrawColor(sdl_renderer, 235, 182, 140, 255);
      SDL_RenderDrawRect(sdl_renderer, &highlight_rect);
    }

    const SDL_Color item_color =
        (i == selected_index) ? kSelectedMenuTextColor : kMenuTextColor;
    const int text_top =
        highlight_rect.y +
        (highlight_rect.h - TTF_FontHeight(sdl_font_menu)) / 2 - 2;
    renderSimpleText(sdl_font_menu, items[i], item_color, screen_res_x / 2,
                     text_top);
  }
}

void Renderer::drawEditorOverlay(const std::string &map_name, MapCoord cursor,
                                 const std::string &warning_message,
                                 bool show_exit_dialog,
                                 int exit_dialog_selected,
                                 bool show_name_dialog,
                                 const std::string &name_input,
                                 const std::string &name_dialog_message) {
  const int header_top = 16;
  renderBrickText(sdl_font_logo, "Map Editor", screen_res_x / 2, header_top,
                  kBrickOutlineColor);
  renderSimpleText(sdl_font_hud, map_name, kHudTextColor, screen_res_x / 2,
                   header_top + TTF_FontHeight(sdl_font_logo) - 6);

  const int x = offset_x + 1 + cursor.v * (element_size + 1);
  const int y = offset_y + 1 + cursor.u * (element_size + 1);
  const bool blink_on = ((SDL_GetTicks() / 420) % 2) == 0;
  const Uint8 alpha = blink_on ? 255 : 120;
  SDL_Rect cursor_rect{x - 2, y - 2, element_size + 4, element_size + 4};
  SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, alpha);
  SDL_RenderDrawRect(sdl_renderer, &cursor_rect);
  SDL_Rect inner_cursor{x - 1, y - 1, element_size + 2, element_size + 2};
  SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, alpha / 2);
  SDL_RenderDrawRect(sdl_renderer, &inner_cursor);

  const int info_width = std::min(760, screen_res_x - 80);
  const int info_height = std::max(116, screen_res_y / 6);
  SDL_Rect info_panel{(screen_res_x - info_width) / 2, screen_res_y - info_height - 28,
                      info_width, info_height};
  SDL_SetRenderDrawColor(sdl_renderer, 10, 6, 18, 92);
  SDL_RenderFillRect(sdl_renderer, &info_panel);
  SDL_SetRenderDrawColor(sdl_renderer, 196, 130, 92, 150);
  SDL_RenderDrawRect(sdl_renderer, &info_panel);
  SDL_Rect info_inner_panel{info_panel.x + 6, info_panel.y + 6,
                            info_panel.w - 12, info_panel.h - 12};
  SDL_SetRenderDrawColor(sdl_renderer, 196, 130, 92, 70);
  SDL_RenderDrawRect(sdl_renderer, &info_inner_panel);

  const std::string line_one =
      "Pfeile bewegen | X/W=Mauer | Leer/Entf=Weg | G=Goodie";
  const std::string line_two =
      "P=Spielfigur | M/N/O=Monster | 1-5=Teleport | Esc oeffnet Dialog";
  renderSimpleText(sdl_font_hud, line_one, kHudTextColor, screen_res_x / 2,
                   info_panel.y + 18);
  renderSimpleText(sdl_font_hud, line_two, kHudTextColor, screen_res_x / 2,
                   info_panel.y + 18 + TTF_FontHeight(sdl_font_hud) + 8);

  const SDL_Color status_color =
      warning_message.empty() ? kStatusTextColor : kWarningTextColor;
  const std::string status_text =
      warning_message.empty() ? "Karte ist gueltig" : warning_message;
  renderSimpleText(sdl_font_hud, status_text, status_color, screen_res_x / 2,
                   info_panel.y + info_panel.h - TTF_FontHeight(sdl_font_hud) - 16);

  if (!show_exit_dialog && !show_name_dialog) {
    return;
  }

  drawDimmer(160);
  if (show_name_dialog) {
    const int dialog_width = std::min(820, screen_res_x * 48 / 100);
    const int dialog_height = std::max(280, screen_res_y * 30 / 100);
    SDL_Rect dialog{(screen_res_x - dialog_width) / 2,
                    (screen_res_y - dialog_height) / 2, dialog_width,
                    dialog_height};
    drawPanel(dialog, kPanelFillColor, kPanelBorderColor);
    renderSimpleText(sdl_font_menu, "Kartenname", kHudTextColor,
                     screen_res_x / 2, dialog.y + 22);

    SDL_Rect input_rect{dialog.x + 40, dialog.y + 92, dialog.w - 80, 68};
    SDL_SetRenderDrawColor(sdl_renderer, 22, 16, 32, 220);
    SDL_RenderFillRect(sdl_renderer, &input_rect);
    SDL_SetRenderDrawColor(sdl_renderer, 235, 182, 140, 255);
    SDL_RenderDrawRect(sdl_renderer, &input_rect);

    std::string visible_name = name_input;
    if (((SDL_GetTicks() / 500) % 2) == 0) {
      visible_name.push_back('_');
    }
    if (visible_name.empty()) {
      visible_name = "_";
    }
    renderSimpleText(sdl_font_menu, visible_name, kSelectedMenuTextColor,
                     screen_res_x / 2,
                     input_rect.y +
                         (input_rect.h - TTF_FontHeight(sdl_font_menu)) / 2 - 2);

    const SDL_Color message_color =
        name_dialog_message.empty() ? kHudTextColor : kWarningTextColor;
    const std::string message_text =
        name_dialog_message.empty() ? "Enter speichert, Esc kehrt zum Dialog zurueck"
                                    : name_dialog_message;
    renderSimpleText(sdl_font_hud, message_text, message_color,
                     screen_res_x / 2, dialog.y + dialog.h - 56);
    return;
  }

  const int dialog_width = std::min(720, screen_res_x * 42 / 100);
  const int dialog_height = std::max(260, screen_res_y * 28 / 100);
  SDL_Rect dialog{(screen_res_x - dialog_width) / 2,
                  (screen_res_y - dialog_height) / 2, dialog_width,
                  dialog_height};
  drawPanel(dialog, kPanelFillColor, kPanelBorderColor);
  renderSimpleText(sdl_font_menu, "Editor verlassen?", kHudTextColor,
                   screen_res_x / 2, dialog.y + 24);

  const std::vector<std::string> items{"Speichern", "Abbruch",
                                       "Nicht speichern"};
  const int item_height = std::max(52, TTF_FontHeight(sdl_font_menu) + 16);
  const int start_y = dialog.y + 82;
  for (int i = 0; i < static_cast<int>(items.size()); i++) {
    SDL_Rect highlight_rect{dialog.x + 28, start_y + i * item_height,
                            dialog.w - 56, item_height - 8};
    if (i == exit_dialog_selected) {
      SDL_SetRenderDrawColor(sdl_renderer, 138, 46, 29, 185);
      SDL_RenderFillRect(sdl_renderer, &highlight_rect);
      SDL_SetRenderDrawColor(sdl_renderer, 235, 182, 140, 255);
      SDL_RenderDrawRect(sdl_renderer, &highlight_rect);
    }

    const SDL_Color item_color =
        (i == exit_dialog_selected) ? kSelectedMenuTextColor : kMenuTextColor;
    const int text_top =
        highlight_rect.y +
        (highlight_rect.h - TTF_FontHeight(sdl_font_menu)) / 2 - 2;
    renderSimpleText(sdl_font_menu, items[i], item_color, screen_res_x / 2,
                     text_top);
  }
}

void Renderer::drawCountdownOverlay(int seconds_left) {
  const int panel_width = std::max(260, screen_res_x / 4);
  const int panel_height = std::max(240, screen_res_y / 3);
  SDL_Rect panel{(screen_res_x - panel_width) / 2,
                 (screen_res_y - panel_height) / 2, panel_width, panel_height};

  drawPanel(panel, kPanelFillColor, kPanelBorderColor);
  renderSimpleText(sdl_font_hud, "Spielstart in", kHudTextColor,
                   screen_res_x / 2, panel.y + 30);
  renderBrickText(
      sdl_font_display, std::to_string(seconds_left), screen_res_x / 2,
      panel.y + (panel.h - TTF_FontHeight(sdl_font_display)) / 2,
      kBrickOutlineColor);
}

int Renderer::getMonsterAnimationFrame(int animation_seed) const {
  if (sdl_monster_textures.empty()) {
    return 0;
  }

  const Uint32 total_offset =
      static_cast<Uint32>((animation_seed * 137) %
                          (MONSTER_ANIMATION_FRAME_MS *
                           static_cast<int>(sdl_monster_textures.size())));
  const Uint32 frame_clock = SDL_GetTicks() + total_offset;
  return static_cast<int>(
      (frame_clock / MONSTER_ANIMATION_FRAME_MS) % sdl_monster_textures.size());
}

SDL_Texture *Renderer::getMonsterTexture(int animation_seed) {
  if (sdl_monster_textures.empty()) {
    return nullptr;
  }
  return sdl_monster_textures[getMonsterAnimationFrame(animation_seed)];
}

void Renderer::renderSimpleText(TTF_Font *font, const std::string &text,
                                const SDL_Color &color, int center_x,
                                int top_y) {
  if (text.empty()) {
    return;
  }

  SDL_Surface *text_surface =
      TTF_RenderUTF8_Blended(font, text.c_str(), color);
  if (text_surface == nullptr) {
    return;
  }

  SDL_Texture *text_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, text_surface);
  if (text_texture != nullptr) {
    SDL_Rect destination{center_x - text_surface->w / 2, top_y, text_surface->w,
                         text_surface->h};
    SDL_RenderCopy(sdl_renderer, text_texture, nullptr, &destination);
    SDL_DestroyTexture(text_texture);
  }

  SDL_FreeSurface(text_surface);
}

void Renderer::renderBrickText(TTF_Font *font, const std::string &text,
                               int center_x, int top_y,
                               const SDL_Color &outline_color) {
  if (text.empty()) {
    return;
  }

  const int shadow_offset = std::max(4, TTF_FontHeight(font) / 28);
  SDL_Surface *shadow_surface =
      TTF_RenderUTF8_Blended(font, text.c_str(), SDL_Color{0, 0, 0, 220});
  if (shadow_surface != nullptr) {
    SDL_Texture *shadow_texture =
        SDL_CreateTextureFromSurface(sdl_renderer, shadow_surface);
    if (shadow_texture != nullptr) {
      SDL_SetTextureAlphaMod(shadow_texture, 170);
      SDL_Rect shadow_rect{center_x - shadow_surface->w / 2 + shadow_offset,
                           top_y + shadow_offset, shadow_surface->w,
                           shadow_surface->h};
      SDL_RenderCopy(sdl_renderer, shadow_texture, nullptr, &shadow_rect);
      SDL_DestroyTexture(shadow_texture);
    }
    SDL_FreeSurface(shadow_surface);
  }

  SDL_Surface *brick_surface = createBrickTextSurface(font, text, outline_color);
  if (brick_surface == nullptr) {
    return;
  }

  SDL_Texture *brick_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, brick_surface);
  if (brick_texture != nullptr) {
    SDL_Rect destination{center_x - brick_surface->w / 2, top_y, brick_surface->w,
                         brick_surface->h};
    SDL_RenderCopy(sdl_renderer, brick_texture, nullptr, &destination);
    SDL_DestroyTexture(brick_texture);
  }

  SDL_FreeSurface(brick_surface);
}

SDL_Surface *Renderer::createBrickTextSurface(TTF_Font *font,
                                              const std::string &text,
                                              const SDL_Color &outline_color) {
  SDL_Surface *glyph_surface_raw =
      TTF_RenderUTF8_Blended(font, text.c_str(), SDL_Color{255, 255, 255, 255});
  if (glyph_surface_raw == nullptr) {
    return nullptr;
  }

  SDL_Surface *glyph_surface =
      SDL_ConvertSurfaceFormat(glyph_surface_raw, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(glyph_surface_raw);
  if (glyph_surface == nullptr) {
    return nullptr;
  }

  SDL_Surface *output_surface = SDL_CreateRGBSurfaceWithFormat(
      0, glyph_surface->w, glyph_surface->h, 32, SDL_PIXELFORMAT_RGBA32);
  if (output_surface == nullptr) {
    SDL_FreeSurface(glyph_surface);
    return nullptr;
  }

  SDL_FillRect(output_surface, nullptr,
               SDL_MapRGBA(output_surface->format, 0, 0, 0, 0));

  const bool lock_glyph = SDL_MUSTLOCK(glyph_surface);
  const bool lock_output = SDL_MUSTLOCK(output_surface);
  const bool lock_brick =
      sdl_logo_brick_surface != nullptr && SDL_MUSTLOCK(sdl_logo_brick_surface);

  if (lock_glyph) {
    SDL_LockSurface(glyph_surface);
  }
  if (lock_output) {
    SDL_LockSurface(output_surface);
  }
  if (lock_brick) {
    SDL_LockSurface(sdl_logo_brick_surface);
  }

  const SDL_Color accent_color{177, 60, 45, 255};
  const int glyph_height = std::max(1, glyph_surface->h - 1);
  auto glyph_alpha = [&](int px, int py) -> Uint8 {
    if (px < 0 || py < 0 || px >= glyph_surface->w || py >= glyph_surface->h) {
      return 0;
    }

    Uint8 red = 0;
    Uint8 green = 0;
    Uint8 blue = 0;
    Uint8 alpha = 0;
    SDL_GetRGBA(readPixel(glyph_surface, px, py), glyph_surface->format, &red,
                &green, &blue, &alpha);
    return alpha;
  };

  for (int y = 0; y < glyph_surface->h; y++) {
    for (int x = 0; x < glyph_surface->w; x++) {
      const Uint8 alpha = glyph_alpha(x, y);
      if (alpha == 0) {
        continue;
      }

      Uint8 brick_r = accent_color.r;
      Uint8 brick_g = accent_color.g;
      Uint8 brick_b = accent_color.b;
      Uint8 brick_a = 255;

      if (sdl_logo_brick_surface != nullptr) {
        const int sample_x =
            (x * 3 + (y / 6) * 5) % sdl_logo_brick_surface->w;
        const int sample_y =
            (y * 3 + (x / 8) * 3) % sdl_logo_brick_surface->h;
        SDL_GetRGBA(readPixel(sdl_logo_brick_surface, sample_x, sample_y),
                    sdl_logo_brick_surface->format, &brick_r, &brick_g, &brick_b,
                    &brick_a);
      }

      float brightness = 1.16f - 0.36f * static_cast<float>(y) / glyph_height;
      int red = static_cast<int>((brick_r * 0.72f + accent_color.r * 0.28f) *
                                 brightness);
      int green =
          static_cast<int>((brick_g * 0.64f + accent_color.g * 0.36f) *
                           brightness);
      int blue =
          static_cast<int>((brick_b * 0.54f + accent_color.b * 0.46f) *
                           brightness);

      const Uint8 left = glyph_alpha(x - 1, y);
      const Uint8 right = glyph_alpha(x + 1, y);
      const Uint8 up = glyph_alpha(x, y - 1);
      const Uint8 down = glyph_alpha(x, y + 1);
      const bool edge = left == 0 || right == 0 || up == 0 || down == 0;
      const bool highlight_edge = left == 0 || up == 0;
      const bool shadow_edge = right == 0 || down == 0;

      if (highlight_edge) {
        red = std::min(255, static_cast<int>(red * 1.08f + 16));
        green = std::min(255, static_cast<int>(green * 1.05f + 10));
        blue = std::min(255, static_cast<int>(blue * 1.02f + 8));
      }
      if (shadow_edge) {
        red = static_cast<int>(red * 0.82f);
        green = static_cast<int>(green * 0.78f);
        blue = static_cast<int>(blue * 0.74f);
      }
      if (edge) {
        red = (red * 2 + outline_color.r * 3) / 5;
        green = (green * 2 + outline_color.g * 3) / 5;
        blue = (blue * 2 + outline_color.b * 3) / 5;
      }

      red = std::clamp(red, 0, 255);
      green = std::clamp(green, 0, 255);
      blue = std::clamp(blue, 0, 255);

      writePixel(output_surface, x, y,
                 SDL_MapRGBA(output_surface->format, red, green, blue, alpha));
    }
  }

  if (lock_brick) {
    SDL_UnlockSurface(sdl_logo_brick_surface);
  }
  if (lock_output) {
    SDL_UnlockSurface(output_surface);
  }
  if (lock_glyph) {
    SDL_UnlockSurface(glyph_surface);
  }

  SDL_FreeSurface(glyph_surface);
  return output_surface;
}

Uint32 Renderer::readPixel(SDL_Surface *surface, int x, int y) {
  Uint32 *pixels = static_cast<Uint32 *>(surface->pixels);
  return pixels[(y * surface->pitch / 4) + x];
}

void Renderer::writePixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
  Uint32 *pixels = static_cast<Uint32 *>(surface->pixels);
  pixels[(y * surface->pitch / 4) + x] = pixel;
}

void Renderer::drawpacman() {
  if (game == nullptr || game->pacman == nullptr) {
    return;
  }

  if (game->dead) {
    drawDefeatEffect();
    return;
  }

  if (game->pacman->teleport_animation_active) {
    const Uint32 elapsed =
        SDL_GetTicks() - game->pacman->teleport_animation_started_ticks;
    const bool in_departure_phase = elapsed < TELEPORT_ANIMATION_PHASE_MS;
    const double phase_progress =
        std::clamp(static_cast<double>(
                       in_departure_phase ? elapsed
                                          : (elapsed - TELEPORT_ANIMATION_PHASE_MS)) /
                       static_cast<double>(TELEPORT_ANIMATION_PHASE_MS),
                   0.0, 1.0);
    const MapCoord render_coord =
        in_departure_phase ? game->pacman->teleport_animation_from_coord
                           : game->pacman->teleport_animation_to_coord;
    const double scale = in_departure_phase ? (1.0 - phase_progress)
                                            : phase_progress;
    const double rotation =
        (in_departure_phase ? phase_progress : (1.0 + phase_progress)) * 1080.0;
    const PixelCoord base_px = getPixelCoord(render_coord, 0, 0);
    const int size =
        std::max(1, static_cast<int>(element_size * 0.9 * std::max(0.0, scale)));
    const SDL_Rect animated_rect{
        base_px.x + element_size / 2 - size / 2,
        base_px.y + element_size / 2 - size / 2, size, size};
    const SDL_Point rotation_center{size / 2, size / 2};

    const Uint8 spark_alpha = static_cast<Uint8>(
        std::clamp((in_departure_phase ? (1.0 - phase_progress)
                                       : (0.35 + 0.65 * phase_progress)) *
                       170.0,
                   0.0, 200.0));
    SDL_SetRenderDrawColor(sdl_renderer, 130, 214, 255, spark_alpha / 2);
    SDL_RenderFillCircle(sdl_renderer, base_px.x + element_size / 2,
                         base_px.y + element_size / 2,
                         std::max(3, static_cast<int>(element_size * 0.42 * scale)));
    SDL_SetRenderDrawColor(sdl_renderer, 220, 245, 255, spark_alpha);
    for (int spark = 0; spark < 5; spark++) {
      const double angle =
          (rotation / 180.0) * 3.1415926535 + spark * (2.0 * 3.1415926535 / 5.0);
      const int spark_length =
          std::max(4, static_cast<int>(element_size * (0.10 + 0.14 * scale)));
      SDL_RenderDrawLine(
          sdl_renderer, base_px.x + element_size / 2,
          base_px.y + element_size / 2,
          base_px.x + element_size / 2 +
              static_cast<int>(std::cos(angle) * spark_length),
          base_px.y + element_size / 2 +
              static_cast<int>(std::sin(angle) * spark_length));
    }

    if (size > 1) {
      SDL_RenderCopyEx(sdl_renderer, sdl_pacman_texture, nullptr, &animated_rect,
                       rotation, &rotation_center, SDL_FLIP_NONE);
    }
    return;
  }

  game->pacman->px_coord = getPixelCoord(
      game->pacman->map_coord, element_size * game->pacman->px_delta.x / 100.0,
      element_size * game->pacman->px_delta.y / 100.0);
  sdl_pacman_rect =
      SDL_Rect{game->pacman->px_coord.x + int(element_size * 0.05),
               game->pacman->px_coord.y + int(element_size * 0.05),
               int(element_size * 0.9), int(element_size * 0.9)};
  SDL_RenderCopy(sdl_renderer, sdl_pacman_texture, nullptr, &sdl_pacman_rect);
}

void Renderer::drawDefeatEffect() {
  if (game == nullptr) {
    return;
  }

  const PixelCoord base_coord = getPixelCoord(game->death_coord, 0, 0);
  const int center_x = base_coord.x + element_size / 2;
  const int center_y = base_coord.y + element_size / 2;
  const Uint32 elapsed =
      (game->death_started_ticks == 0)
          ? 1000
          : (SDL_GetTicks() - game->death_started_ticks);

  if (elapsed < 900) {
    const double progress =
        std::clamp(static_cast<double>(elapsed) / 900.0, 0.0, 1.0);
    const double pulse = 0.70 + 0.30 * std::sin(progress * 5.0 * 3.1415926535);
    const int outer_radius =
        std::max(8, static_cast<int>(element_size * (0.32 + progress * 0.72)));
    const int mid_radius =
        std::max(5, static_cast<int>(element_size * (0.22 + progress * 0.52)));
    const int core_radius =
        std::max(3, static_cast<int>(element_size * (0.12 + progress * 0.28)));
    const Uint8 outer_alpha =
        static_cast<Uint8>(std::clamp(120.0 * (1.0 - progress), 0.0, 120.0));
    const Uint8 mid_alpha = static_cast<Uint8>(
        std::clamp(180.0 * (1.0 - progress * 0.55), 0.0, 180.0));
    const Uint8 core_alpha = static_cast<Uint8>(
        std::clamp(220.0 * (1.0 - progress * 0.35), 0.0, 220.0));

    SDL_SetRenderDrawColor(sdl_renderer, 255, 92, 28, outer_alpha);
    SDL_RenderFillCircle(sdl_renderer, center_x, center_y, outer_radius);
    SDL_SetRenderDrawColor(sdl_renderer, 255, 170, 48, mid_alpha);
    SDL_RenderFillCircle(sdl_renderer, center_x, center_y, mid_radius);
    SDL_SetRenderDrawColor(sdl_renderer, 255, 228, 108, core_alpha);
    SDL_RenderFillCircle(sdl_renderer, center_x, center_y, core_radius);

    const int spark_count = 7;
    for (int spark = 0; spark < spark_count; spark++) {
      const double angle =
          progress * 4.6 + spark * (2.0 * 3.1415926535 / spark_count);
      const int spark_start =
          std::max(2, static_cast<int>(element_size * (0.10 + 0.20 * pulse)));
      const int spark_end = std::max(
          spark_start + 2,
          static_cast<int>(element_size * (0.42 + 0.70 * progress)));
      const int x1 = center_x + static_cast<int>(std::cos(angle) * spark_start);
      const int y1 = center_y + static_cast<int>(std::sin(angle) * spark_start);
      const int x2 = center_x + static_cast<int>(std::cos(angle) * spark_end);
      const int y2 = center_y + static_cast<int>(std::sin(angle) * spark_end);
      SDL_SetRenderDrawColor(
          sdl_renderer, 255, 210 - spark * 10, 90,
          static_cast<Uint8>(std::clamp(210.0 * (1.0 - progress), 0.0, 210.0)));
      SDL_RenderDrawLine(sdl_renderer, x1, y1, x2, y2);
    }
    return;
  }

  const int skull_radius = std::max(6, static_cast<int>(element_size * 0.24f));
  SDL_SetRenderDrawColor(sdl_renderer, 240, 236, 230, 220);
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y - skull_radius / 3,
                       skull_radius);

  SDL_Rect jaw_rect{center_x - skull_radius / 2, center_y, skull_radius,
                    std::max(4, skull_radius / 2)};
  SDL_SetRenderDrawColor(sdl_renderer, 240, 236, 230, 220);
  SDL_RenderFillRect(sdl_renderer, &jaw_rect);

  SDL_SetRenderDrawColor(sdl_renderer, 28, 18, 18, 230);
  SDL_RenderFillCircle(sdl_renderer, center_x - skull_radius / 3,
                       center_y - skull_radius / 2, std::max(2, skull_radius / 4));
  SDL_RenderFillCircle(sdl_renderer, center_x + skull_radius / 3,
                       center_y - skull_radius / 2, std::max(2, skull_radius / 4));
  const SDL_Point nose_points[4] = {
      {center_x, center_y - skull_radius / 5},
      {center_x - std::max(2, skull_radius / 6), center_y + skull_radius / 6},
      {center_x + std::max(2, skull_radius / 6), center_y + skull_radius / 6},
      {center_x, center_y - skull_radius / 5}};
  SDL_RenderDrawLines(sdl_renderer, nose_points, 4);

  SDL_SetRenderDrawColor(sdl_renderer, 210, 206, 198, 220);
  for (int tooth = -1; tooth <= 1; tooth++) {
    const int tooth_x = center_x + tooth * std::max(2, skull_radius / 4);
    SDL_RenderDrawLine(sdl_renderer, tooth_x, center_y + 1, tooth_x,
                       center_y + jaw_rect.h - 2);
  }
}

void Renderer::drawgoodies() {
  if (game == nullptr) {
    return;
  }

  if (game->dead) {
    return;
  }

  for (auto goodie : game->goodies) {
    if (goodie->is_active) {
      goodie->px_coord = getPixelCoord(goodie->map_coord, 0, 0);
      sdl_goodie_rect =
          SDL_Rect{goodie->px_coord.x + int(element_size * 0.15),
                   goodie->px_coord.y + int(element_size * 0.15),
                   int(element_size * 0.7), int(element_size * 0.7)};
      SDL_RenderCopy(sdl_renderer, sdl_goodie_texture, nullptr,
                     &sdl_goodie_rect);
    }
  }
}

void Renderer::drawmonsters() {
  if (game == nullptr) {
    return;
  }

  if (game->dead) {
    return;
  }

  for (auto monster : game->monsters) {
    if (!monster->is_alive) {
      continue;
    }

    SDL_Texture *monster_texture = getMonsterTexture(monster->id);
    if (monster_texture == nullptr) {
      continue;
    }

    monster->px_coord = getPixelCoord(
        monster->map_coord, element_size * monster->px_delta.x / 100.0,
        element_size * monster->px_delta.y / 100.0);
    sdl_monster_rect =
        SDL_Rect{monster->px_coord.x + int(element_size * 0.05),
                 monster->px_coord.y + int(element_size * 0.05),
                 int(element_size * 0.9), int(element_size * 0.9)};
    drawMonsterGlow(sdl_monster_rect, monster->monster_char, monster->id);
    SDL_RenderCopy(sdl_renderer, monster_texture, nullptr, &sdl_monster_rect);
  }
}

void Renderer::drawfireballs() {
  if (game == nullptr || game->dead) {
    return;
  }

  for (const auto &fireball : game->fireballs) {
    const MapCoord next_coord = StepRenderCoord(fireball.current_coord,
                                                fireball.direction);
    const PixelCoord start_px = getPixelCoord(fireball.current_coord, 0, 0);
    const PixelCoord end_px = getPixelCoord(next_coord, 0, 0);
    const double progress = std::clamp(
        static_cast<double>(SDL_GetTicks() - fireball.segment_started_ticks) /
            static_cast<double>(FIREBALL_STEP_DURATION_MS),
        0.0, 1.0);

    const int start_center_x = start_px.x + element_size / 2;
    const int start_center_y = start_px.y + element_size / 2;
    const int end_center_x = end_px.x + element_size / 2;
    const int end_center_y = end_px.y + element_size / 2;
    const int center_x =
        start_center_x + static_cast<int>((end_center_x - start_center_x) * progress);
    const int center_y =
        start_center_y + static_cast<int>((end_center_y - start_center_y) * progress);
    const int trail_x =
        center_x - static_cast<int>((end_center_x - start_center_x) * 0.28);
    const int trail_y =
        center_y - static_cast<int>((end_center_y - start_center_y) * 0.28);

    SDL_SetRenderDrawColor(sdl_renderer, 255, 110, 24, 120);
    SDL_RenderFillCircle(sdl_renderer, center_x, center_y,
                         std::max(4, element_size / 4));
    SDL_SetRenderDrawColor(sdl_renderer, 255, 190, 70, 175);
    SDL_RenderFillCircle(sdl_renderer, center_x, center_y,
                         std::max(3, element_size / 6));
    SDL_SetRenderDrawColor(sdl_renderer, 255, 230, 140, 220);
    SDL_RenderDrawLine(sdl_renderer, trail_x, trail_y, center_x, center_y);
  }
}

void Renderer::drawgasclouds() {
  if (game == nullptr || game->dead) {
    return;
  }

  const Uint32 now = SDL_GetTicks();
  for (const auto &cloud : game->gas_clouds) {
    double alpha_factor = 1.0;
    if (cloud.is_fading) {
      alpha_factor =
          1.0 - std::clamp(static_cast<double>(now - cloud.fade_started_ticks) /
                               static_cast<double>(GAS_CLOUD_FADE_MS),
                           0.0, 1.0);
    }

    if (alpha_factor <= 0.0) {
      continue;
    }

    const PixelCoord cloud_px = getPixelCoord(cloud.coord, 0, 0);
    const int center_x = cloud_px.x + element_size / 2;
    const int center_y = cloud_px.y + element_size / 2;
    const double wobble_clock =
        (static_cast<double>(now + cloud.animation_seed * 67) / 210.0);
    const int base_radius = std::max(5, static_cast<int>(element_size * 0.23));
    const Uint8 base_alpha =
        static_cast<Uint8>(std::clamp(145.0 * alpha_factor, 0.0, 190.0));

    for (int puff = 0; puff < 5; puff++) {
      const double puff_phase = wobble_clock + puff * 1.17;
      const int offset_x =
          static_cast<int>(std::cos(puff_phase) * element_size * 0.14);
      const int offset_y =
          static_cast<int>(std::sin(puff_phase * 1.23) * element_size * 0.11);
      const int puff_radius =
          base_radius + static_cast<int>((1.0 + std::sin(puff_phase * 1.4)) *
                                         element_size * 0.08);

      SDL_SetRenderDrawColor(sdl_renderer, 118, 168, 82, base_alpha);
      SDL_RenderFillCircle(sdl_renderer, center_x + offset_x,
                           center_y + offset_y, puff_radius);
      SDL_SetRenderDrawColor(sdl_renderer, 170, 190, 104,
                             std::min(255, base_alpha / 2 + 30));
      SDL_RenderFillCircle(sdl_renderer,
                           center_x + offset_x / 2 + puff_radius / 3,
                           center_y + offset_y / 2 - puff_radius / 4,
                           std::max(3, puff_radius / 2));
    }

    SDL_SetRenderDrawColor(
        sdl_renderer, 150, 214, 128,
        static_cast<Uint8>(std::clamp(80.0 * alpha_factor, 0.0, 100.0)));
    SDL_RenderDrawCircle(sdl_renderer, center_x, center_y,
                         std::max(7, static_cast<int>(element_size * 0.34)));
  }
}

void Renderer::draweffects() {
  if (game == nullptr || game->dead) {
    return;
  }

  for (const auto &effect : game->effects) {
    const PixelCoord effect_px = getPixelCoord(effect.coord, 0, 0);
    const int center_x = effect_px.x + element_size / 2;
    const int center_y = effect_px.y + element_size / 2;
    const Uint32 elapsed = SDL_GetTicks() - effect.started_ticks;

    if (effect.type == EffectType::MonsterExplosion) {
      const double progress =
          std::clamp(static_cast<double>(elapsed) / 520.0, 0.0, 1.0);
      const int outer_radius =
          std::max(5, static_cast<int>(element_size * (0.18 + 0.42 * progress)));
      const int inner_radius =
          std::max(3, static_cast<int>(element_size * (0.10 + 0.22 * progress)));
      const Uint8 alpha =
          static_cast<Uint8>(std::clamp(210.0 * (1.0 - progress), 0.0, 210.0));
      SDL_SetRenderDrawColor(sdl_renderer, 255, 88, 40, alpha / 2);
      SDL_RenderFillCircle(sdl_renderer, center_x, center_y, outer_radius);
      SDL_SetRenderDrawColor(sdl_renderer, 255, 180, 72, alpha);
      SDL_RenderFillCircle(sdl_renderer, center_x, center_y, inner_radius);

      for (int spark = 0; spark < 6; spark++) {
        const double angle =
            progress * 4.0 + spark * (2.0 * 3.1415926535 / 6.0);
        const int length =
            std::max(4, static_cast<int>(element_size * (0.20 + 0.35 * progress)));
        const int x2 = center_x + static_cast<int>(std::cos(angle) * length);
        const int y2 = center_y + static_cast<int>(std::sin(angle) * length);
        SDL_SetRenderDrawColor(sdl_renderer, 255, 220, 120, alpha);
        SDL_RenderDrawLine(sdl_renderer, center_x, center_y, x2, y2);
      }
    } else {
      const double progress =
          std::clamp(static_cast<double>(elapsed) / 180.0, 0.0, 1.0);
      const int radius =
          std::max(3, static_cast<int>(element_size * (0.14 + 0.12 * progress)));
      const Uint8 alpha =
          static_cast<Uint8>(std::clamp(180.0 * (1.0 - progress), 0.0, 180.0));
      SDL_SetRenderDrawColor(sdl_renderer, 255, 210, 150, alpha);
      SDL_RenderFillCircle(sdl_renderer, center_x, center_y, radius);
      SDL_SetRenderDrawColor(sdl_renderer, 210, 190, 180, alpha);
      SDL_RenderDrawLine(sdl_renderer, center_x - radius, center_y - radius,
                         center_x + radius, center_y + radius);
      SDL_RenderDrawLine(sdl_renderer, center_x - radius, center_y + radius,
                         center_x + radius, center_y - radius);
    }
  }
}

void Renderer::drawmap() {
  if (map == nullptr) {
    return;
  }

  SDL_Rect block;
  block.w = element_size;
  block.h = element_size;
  SDL_SetRenderDrawColor(sdl_renderer, 50, 50, 50, 0xFF);

  for (size_t i = 0; i < cols; i++) {
    for (size_t j = 0; j < rows; j++) {
      const ElementType entry = map->map_entry(j, i);
      const int x = offset_x + 1 + static_cast<int>(i) * (element_size + 1);
      const int y = offset_y + 1 + static_cast<int>(j) * (element_size + 1);

      switch (entry) {
      case ElementType::TYPE_WALL:
        sdl_wall_rect = SDL_Rect{x, y, element_size + 1, element_size + 1};
        SDL_RenderCopy(sdl_renderer, sdl_wall_texture, nullptr, &sdl_wall_rect);
        break;
      case ElementType::TYPE_PATH:
      case ElementType::TYPE_TELEPORTER:
      case ElementType::TYPE_GOODIE:
      case ElementType::TYPE_PACMAN:
      case ElementType::TYPE_MONSTER:
      default:
        SDL_SetRenderDrawColor(sdl_renderer, COLOR_PATH, 0xFF);
        block.x = x;
        block.y = y;
        SDL_RenderFillRect(sdl_renderer, &block);
        break;
      }
    }
  }
}

void Renderer::drawteleporters() {
  if (map == nullptr) {
    return;
  }

  for (size_t teleporter_index = 0;
       teleporter_index < map->get_teleporter_pairs().size(); teleporter_index++) {
    const auto &teleporter_pair = map->get_teleporter_pairs()[teleporter_index];
    const std::vector<MapCoord> positions{teleporter_pair.first,
                                          teleporter_pair.second};
    for (size_t position_index = 0; position_index < positions.size();
         position_index++) {
      const PixelCoord teleporter_px = getPixelCoord(positions[position_index], 0, 0);
      SDL_Rect teleporter_rect{teleporter_px.x + int(element_size * 0.08),
                               teleporter_px.y + int(element_size * 0.08),
                               int(element_size * 0.84),
                               int(element_size * 0.84)};
      drawTeleporterGlyph(
          teleporter_rect, teleporter_pair.digit,
          static_cast<int>(teleporter_index * 19 + position_index * 7));
    }
  }
}

void Renderer::drawLayout(const std::vector<std::string> &layout) {
  SDL_Rect block;
  block.w = element_size;
  block.h = element_size;

  for (size_t row = 0; row < layout.size(); row++) {
    for (size_t col = 0; col < layout[row].size(); col++) {
      const int x = offset_x + 1 + static_cast<int>(col) * (element_size + 1);
      const int y = offset_y + 1 + static_cast<int>(row) * (element_size + 1);
      if (layout[row][col] == BRICK) {
        sdl_wall_rect = SDL_Rect{x, y, element_size + 1, element_size + 1};
        SDL_RenderCopy(sdl_renderer, sdl_wall_texture, nullptr, &sdl_wall_rect);
      } else {
        SDL_SetRenderDrawColor(sdl_renderer, COLOR_PATH, 0xFF);
        block.x = x;
        block.y = y;
        SDL_RenderFillRect(sdl_renderer, &block);
      }
    }
  }
}

void Renderer::drawLayoutTeleporters(const std::vector<std::string> &layout) {
  for (size_t row = 0; row < layout.size(); row++) {
    for (size_t col = 0; col < layout[row].size(); col++) {
      if (!Map::IsTeleporterChar(layout[row][col])) {
        continue;
      }

      const int x = offset_x + 1 + static_cast<int>(col) * (element_size + 1);
      const int y = offset_y + 1 + static_cast<int>(row) * (element_size + 1);
      SDL_Rect teleporter_rect{x + int(element_size * 0.08),
                               y + int(element_size * 0.08),
                               int(element_size * 0.84),
                               int(element_size * 0.84)};
      drawTeleporterGlyph(
          teleporter_rect, layout[row][col],
          static_cast<int>(row * layout[row].size() + col));
    }
  }
}

void Renderer::drawTeleporterGlyph(const SDL_Rect &teleporter_rect,
                                   char teleporter_digit,
                                   int animation_seed) {
  const SDL_Color portal_color = TeleporterColor(teleporter_digit);
  const int center_x = teleporter_rect.x + teleporter_rect.w / 2;
  const int center_y = teleporter_rect.y + teleporter_rect.h / 2;
  const int max_radius = std::max(5, teleporter_rect.w / 2 - 1);
  const double time =
      static_cast<double>(SDL_GetTicks() + animation_seed * 113) / 480.0;

  SDL_SetRenderDrawColor(sdl_renderer, portal_color.r / 5, portal_color.g / 5,
                         portal_color.b / 5, 170);
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y,
                       std::max(3, max_radius - 2));

  for (int arm = 0; arm < 3; arm++) {
    for (int step = 0; step < 18; step++) {
      const double progress = static_cast<double>(step) / 17.0;
      const double angle = time + arm * (2.0 * 3.1415926535 / 3.0) +
                           progress * 4.5;
      const double radius = max_radius * (0.18 + progress * 0.72);
      const int point_x =
          center_x + static_cast<int>(std::cos(angle) * radius * 1.05);
      const int point_y =
          center_y + static_cast<int>(std::sin(angle) * radius * 0.72);
      const int point_radius =
          std::max(1, static_cast<int>((1.0 - progress) * max_radius * 0.18));
      const Uint8 alpha = static_cast<Uint8>(
          std::clamp(235.0 * (1.0 - progress * 0.55), 40.0, 235.0));

      SDL_SetRenderDrawColor(sdl_renderer, portal_color.r, portal_color.g,
                             portal_color.b, alpha);
      SDL_RenderFillCircle(sdl_renderer, point_x, point_y, point_radius);

      if (step % 4 == 0) {
        SDL_SetRenderDrawColor(sdl_renderer, 246, 244, 255, alpha / 2 + 40);
        SDL_RenderFillCircle(sdl_renderer, point_x + point_radius / 2,
                             point_y - point_radius / 2, 1);
      }
    }
  }

  SDL_SetRenderDrawColor(sdl_renderer, 245, 242, 232, 165);
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y,
                       std::max(2, max_radius / 5));
  SDL_SetRenderDrawColor(sdl_renderer, portal_color.r, portal_color.g,
                         portal_color.b, 190);
  SDL_RenderDrawCircle(sdl_renderer, center_x, center_y, max_radius);
}

void Renderer::drawStaticPacman() {
  if (map == nullptr) {
    return;
  }

  const MapCoord pacman_coord = map->get_coord_pacman();
  const PixelCoord pacman_px = getPixelCoord(pacman_coord, 0, 0);
  sdl_pacman_rect = SDL_Rect{pacman_px.x + int(element_size * 0.05),
                             pacman_px.y + int(element_size * 0.05),
                             int(element_size * 0.9), int(element_size * 0.9)};
  SDL_RenderCopy(sdl_renderer, sdl_pacman_texture, nullptr, &sdl_pacman_rect);
}

void Renderer::drawStaticMonsters() {
  if (map == nullptr) {
    return;
  }

  for (int i = 0; i < map->get_number_monsters(); i++) {
    SDL_Texture *monster_texture = getMonsterTexture(i);
    if (monster_texture == nullptr) {
      continue;
    }

    const PixelCoord monster_px = getPixelCoord(map->get_coord_monster(i), 0, 0);
    sdl_monster_rect =
        SDL_Rect{monster_px.x + int(element_size * 0.05),
                 monster_px.y + int(element_size * 0.05),
                 int(element_size * 0.9), int(element_size * 0.9)};
    drawMonsterGlow(sdl_monster_rect, map->get_char_monster(i), i);
    SDL_RenderCopy(sdl_renderer, monster_texture, nullptr, &sdl_monster_rect);
  }
}

void Renderer::drawStaticGoodies() {
  if (map == nullptr) {
    return;
  }

  for (int i = 0; i < map->get_number_goodies(); i++) {
    const PixelCoord goodie_px = getPixelCoord(map->get_coord_goodie(i), 0, 0);
    sdl_goodie_rect =
        SDL_Rect{goodie_px.x + int(element_size * 0.15),
                 goodie_px.y + int(element_size * 0.15),
                 int(element_size * 0.7), int(element_size * 0.7)};
    SDL_RenderCopy(sdl_renderer, sdl_goodie_texture, nullptr, &sdl_goodie_rect);
  }
}

void Renderer::drawLayoutEntities(const std::vector<std::string> &layout) {
  for (size_t row = 0; row < layout.size(); row++) {
    for (size_t col = 0; col < layout[row].size(); col++) {
      const char entry = layout[row][col];
      const int x = offset_x + 1 + static_cast<int>(col) * (element_size + 1);
      const int y = offset_y + 1 + static_cast<int>(row) * (element_size + 1);

      if (entry == GOODIE) {
        sdl_goodie_rect = SDL_Rect{x + int(element_size * 0.15),
                                   y + int(element_size * 0.15),
                                   int(element_size * 0.7),
                                   int(element_size * 0.7)};
        SDL_RenderCopy(sdl_renderer, sdl_goodie_texture, nullptr,
                       &sdl_goodie_rect);
      } else if (entry == PACMAN) {
        sdl_pacman_rect = SDL_Rect{x + int(element_size * 0.05),
                                   y + int(element_size * 0.05),
                                   int(element_size * 0.9),
                                   int(element_size * 0.9)};
        SDL_RenderCopy(sdl_renderer, sdl_pacman_texture, nullptr,
                       &sdl_pacman_rect);
      } else if (entry == MONSTER_FEW || entry == MONSTER_MEDIUM ||
                 entry == MONSTER_MANY) {
        const int animation_seed =
            static_cast<int>(row * layout[row].size() + col);
        SDL_Texture *monster_texture = getMonsterTexture(animation_seed);
        if (monster_texture == nullptr) {
          continue;
        }
        sdl_monster_rect = SDL_Rect{x + int(element_size * 0.05),
                                    y + int(element_size * 0.05),
                                    int(element_size * 0.9),
                                    int(element_size * 0.9)};
        drawMonsterGlow(sdl_monster_rect, entry, animation_seed);
        SDL_RenderCopy(sdl_renderer, monster_texture, nullptr,
                       &sdl_monster_rect);
      }
    }
  }
}

void Renderer::drawMonsterGlow(const SDL_Rect &monster_rect, char monster_char,
                               int shimmer_seed) {
  const SDL_Color glow_color = MonsterGlowColor(monster_char);
  const int cycle = 1800;
  int pulse = static_cast<int>((SDL_GetTicks() + shimmer_seed * 137) % cycle);
  if (pulse > cycle / 2) {
    pulse = cycle - pulse;
  }

  const int alpha_base = 26 + (pulse * 28) / std::max(1, cycle / 2);
  const int center_x = monster_rect.x + monster_rect.w / 2;
  const int center_y = monster_rect.y + monster_rect.h / 2;
  const int outer_radius = std::max(6, static_cast<int>(element_size * 0.82f));
  const int middle_radius = std::max(4, static_cast<int>(element_size * 0.66f));
  const int inner_radius = std::max(3, static_cast<int>(element_size * 0.5f));

  SDL_SetRenderDrawColor(sdl_renderer, glow_color.r, glow_color.g, glow_color.b,
                         std::min(255, alpha_base / 2));
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y, outer_radius);
  SDL_SetRenderDrawColor(sdl_renderer, glow_color.r, glow_color.g, glow_color.b,
                         std::min(255, alpha_base));
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y, middle_radius);
  SDL_SetRenderDrawColor(sdl_renderer, glow_color.r, glow_color.g, glow_color.b,
                         std::min(255, alpha_base + 18));
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y, inner_radius);
}

int Renderer::SDL_RenderDrawCircle(SDL_Renderer *renderer, int x, int y,
                                   int radius) {
  int offsetx = 0;
  int offsety = radius;
  int d = radius - 1;
  int status = 0;

  while (offsety >= offsetx) {
    status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
    status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
    status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
    status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
    status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
    status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
    status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
    status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

    if (status < 0) {
      status = -1;
      break;
    }

    if (d >= 2 * offsetx) {
      d -= 2 * offsetx + 1;
      offsetx += 1;
    } else if (d < 2 * (radius - offsety)) {
      d += 2 * offsety - 1;
      offsety -= 1;
    } else {
      d += 2 * (offsety - offsetx - 1);
      offsety -= 1;
      offsetx += 1;
    }
  }

  return status;
}

int Renderer::SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y,
                                   int radius) {
  int offsetx = 0;
  int offsety = radius;
  int d = radius - 1;
  int status = 0;

  while (offsety >= offsetx) {
    status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
                                 x + offsety, y + offsetx);
    status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
                                 x + offsetx, y + offsety);
    status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
                                 x + offsetx, y - offsety);
    status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
                                 x + offsety, y - offsetx);

    if (status < 0) {
      status = -1;
      break;
    }

    if (d >= 2 * offsetx) {
      d -= 2 * offsetx + 1;
      offsetx += 1;
    } else if (d < 2 * (radius - offsety)) {
      d += 2 * offsety - 1;
      offsety -= 1;
    } else {
      d += 2 * (offsety - offsetx - 1);
      offsety -= 1;
      offsetx += 1;
    }
  }

  return status;
}

PixelCoord Renderer::getPixelCoord(MapCoord in_coord, int delta_x,
                                   int delta_y) {
  PixelCoord out;
  out.x = offset_x + 1 + in_coord.v * (element_size + 1) + delta_x;
  out.y = offset_y + 1 + in_coord.u * (element_size + 1) + delta_y;
  return out;
}
