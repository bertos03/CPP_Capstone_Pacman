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
const SDL_Color kPanelFillColor{10, 6, 18, 205};
const SDL_Color kPanelBorderColor{196, 130, 92, 255};
const SDL_Color kBrickOutlineColor{78, 20, 14, 255};

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

} // namespace

Renderer::Renderer(Map *_map, Game *_game)
    : screen_res_x(0), screen_res_y(0), element_size(0), offset_x(0),
      offset_y(0), rows(0), cols(0), map(_map), game(_game),
      sdl_window(nullptr), sdl_renderer(nullptr), sdl_wall_surface(nullptr),
      sdl_wall_texture(nullptr), sdl_goodie_surface(nullptr),
      sdl_goodie_texture(nullptr), sdl_monster_surface(nullptr),
      sdl_monster_texture(nullptr), sdl_pacman_surface(nullptr),
      sdl_pacman_texture(nullptr), sdl_logo_brick_surface(nullptr),
      sdl_font_hud(nullptr), sdl_font_menu(nullptr), sdl_font_logo(nullptr),
      sdl_font_display(nullptr), sdl_font_color{255, 255, 255, 255},
      sdl_font_back_color{COLOR_BACK, 255}, texW(0), texH(0) {
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
  rows = map->get_map_rows();
  cols = map->get_map_cols();
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
  sdl_monster_surface = SDL_LoadBMP(MONSTER_PATH);
  sdl_pacman_surface = SDL_LoadBMP(PACMAN_PATH);

  if (sdl_goodie_surface == nullptr || sdl_wall_surface == nullptr ||
      sdl_monster_surface == nullptr || sdl_pacman_surface == nullptr) {
    std::cerr << "Could not open sprite assets: " << SDL_GetError() << "\n";
    exit(1);
  }

  sdl_wall_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_wall_surface);
  sdl_goodie_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_goodie_surface);
  sdl_monster_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_monster_surface);
  sdl_pacman_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_pacman_surface);

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
  if (sdl_monster_texture != nullptr) {
    SDL_DestroyTexture(sdl_monster_texture);
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
  if (sdl_monster_surface != nullptr) {
    SDL_FreeSurface(sdl_monster_surface);
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
  drawgoodies();
  drawpacman();
  drawmonsters();
  if (show_hud) {
    drawhud();
  }
}

void Renderer::drawhud() {
  const std::string title_text =
      "BOBMAN        Score: " + std::to_string(game->score);
  renderSimpleText(sdl_font_hud, title_text, sdl_font_color, screen_res_x / 2,
                   30);

  if (game->win) {
    renderBrickText(sdl_font_display, "You Won", screen_res_x / 2,
                    (screen_res_y - TTF_FontHeight(sdl_font_display)) / 2,
                    kBrickOutlineColor);
  }
  if (game->dead) {
    renderBrickText(sdl_font_display, "Game Over", screen_res_x / 2,
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

  const int panel_width = std::min(720, screen_res_x * 42 / 100);
  const int panel_height = std::max(260, screen_res_y * 28 / 100);
  const int panel_top =
      std::min(screen_res_y - panel_height - 70,
               logo_top + TTF_FontHeight(sdl_font_logo) + screen_res_y / 20);
  SDL_Rect panel{(screen_res_x - panel_width) / 2, panel_top, panel_width,
                 panel_height};

  drawPanel(panel, kPanelFillColor, kPanelBorderColor);

  const std::vector<std::string> menu_items{
      "Start Spiel", "Karte: " + map_name, "Konfiguration", "Ende"};
  const int item_height = std::max(52, TTF_FontHeight(sdl_font_menu) + 18);
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

  const std::string controls_hint =
      "Pfeiltasten hoch/runter, Enter bestaetigt";
  renderSimpleText(sdl_font_hud, controls_hint, kHudTextColor, screen_res_x / 2,
                   panel.y + panel.h + 18);

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

  const int panel_width = std::min(820, screen_res_x * 48 / 100);
  const int panel_height = std::max(290, screen_res_y * 32 / 100);
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

  renderSimpleText(sdl_font_hud,
                   "Enter aendert den Wert oder bestaetigt, Esc geht zurueck",
                   kHudTextColor, screen_res_x / 2, panel.y + panel.h + 18);
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
      110 + static_cast<int>(map_names.size()) * item_height;
  const int panel_height = std::min(std::max(260, desired_panel_height),
                                    screen_res_y * 62 / 100);
  const int panel_top =
      std::min(screen_res_y - panel_height - 70,
               logo_top + TTF_FontHeight(sdl_font_logo) + screen_res_y / 14);
  SDL_Rect panel{(screen_res_x - panel_width) / 2, panel_top, panel_width,
                 panel_height};

  drawPanel(panel, kPanelFillColor, kPanelBorderColor);

  const int highlight_width = panel.w - 56;
  const int item_start_y =
      panel.y + 28 +
      std::max(0, (panel.h - 72 - static_cast<int>(map_names.size()) * item_height) /
                       2);

  for (int i = 0; i < static_cast<int>(map_names.size()); i++) {
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
    renderSimpleText(sdl_font_menu, map_names[i], item_color, screen_res_x / 2,
                     text_top);
  }

  renderSimpleText(
      sdl_font_hud,
      "Pfeiltasten waehlen, Enter bestaetigt, Esc verwirft die Vorschau",
      kHudTextColor, screen_res_x / 2, panel.y + panel.h + 18);
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
  game->pacman->px_coord = getPixelCoord(
      game->pacman->map_coord, element_size * game->pacman->px_delta.x / 100.0,
      element_size * game->pacman->px_delta.y / 100.0);
  sdl_pacman_rect =
      SDL_Rect{game->pacman->px_coord.x + int(element_size * 0.05),
               game->pacman->px_coord.y + int(element_size * 0.05),
               int(element_size * 0.9), int(element_size * 0.9)};
  SDL_RenderCopy(sdl_renderer, sdl_pacman_texture, nullptr, &sdl_pacman_rect);
}

void Renderer::drawgoodies() {
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
  for (auto monster : game->monsters) {
    monster->px_coord = getPixelCoord(
        monster->map_coord, element_size * monster->px_delta.x / 100.0,
        element_size * monster->px_delta.y / 100.0);
    sdl_monster_rect =
        SDL_Rect{monster->px_coord.x + int(element_size * 0.05),
                 monster->px_coord.y + int(element_size * 0.05),
                 int(element_size * 0.9), int(element_size * 0.9)};
    SDL_RenderCopy(sdl_renderer, sdl_monster_texture, nullptr,
                   &sdl_monster_rect);
  }
}

void Renderer::drawmap() {
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
