/*
 * PROJECT COMMENT (renderer.cpp)
 * ---------------------------------------------------------------------------
 * This file is part of "BobMan", an SDL2 game inspired by Pac-Man.
 * The code in this unit has a clear responsibility so newcomers can quickly
 * understand the architecture: data model (map, elements), runtime logic
 * (game, events), rendering (renderer), and optional audio output.
 *
 * Important notes for newcomers:
 * - Header files declare classes, methods, and data types.
 * - CPP files contain the concrete implementation of the logic.
 * - Multiple threads move game entities in parallel, so shared data is read
 *   and written in a controlled way.
 * - Macros in definitions.h control resource paths, colors, and features.
 */

#include "renderer.h"
#include "audio.h"
#include "paths.h"

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
const SDL_Color kStartMenuPanelFillColor{10, 6, 18, 118};
const SDL_Color kStartMenuPanelBorderColor{196, 130, 92, 180};
const SDL_Color kBrickOutlineColor{78, 20, 14, 255};
const SDL_Color kShieldTextColor{116, 220, 255, 255};
const SDL_Color kFewMonsterGlowColor{120, 210, 255, 255};
const SDL_Color kMediumMonsterGlowColor{255, 170, 72, 255};
const SDL_Color kManyMonsterGlowColor{255, 82, 82, 255};
const SDL_Color kStartLogoHighlightColor{238, 252, 255, 255};
const SDL_Color kStartLogoLightColor{88, 234, 255, 255};
const SDL_Color kStartLogoMidColor{44, 156, 255, 255};
const SDL_Color kStartLogoDeepColor{18, 70, 222, 255};
const SDL_Color kStartLogoOutlineColor{6, 18, 102, 255};
const SDL_Color kTeleporterBlue{78, 160, 255, 255};
const SDL_Color kTeleporterRed{255, 104, 104, 255};
const SDL_Color kTeleporterGreen{110, 205, 118, 255};
const SDL_Color kTeleporterGrey{172, 176, 186, 255};
const SDL_Color kTeleporterYellow{244, 214, 88, 255};
constexpr Uint8 kStartMenuLedFillAlpha = 148;
constexpr Uint8 kStartMenuLedBorderAlpha = 132;
constexpr double kLogoPi = 3.14159265358979323846;
constexpr int kPacmanFramesPerDirection = 4;
constexpr Uint32 kPacmanWalkFrameMs = 140;

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

SDL_Color LerpColor(const SDL_Color &from, const SDL_Color &to, float factor) {
  factor = std::clamp(factor, 0.0f, 1.0f);
  const auto mix_channel = [factor](Uint8 start, Uint8 end) -> Uint8 {
    const float blended =
        static_cast<float>(start) +
        (static_cast<float>(end) - static_cast<float>(start)) * factor;
    return static_cast<Uint8>(std::clamp(static_cast<int>(std::lround(blended)),
                                         0, 255));
  };

  return SDL_Color{mix_channel(from.r, to.r), mix_channel(from.g, to.g),
                   mix_channel(from.b, to.b), 255};
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
      sdl_goodie_texture(nullptr), sdl_logo_brick_surface(nullptr),
      sdl_dynamite_texture(nullptr), sdl_dynamite_size{0, 0},
      sdl_start_menu_monster_texture(nullptr), sdl_start_menu_monster_size{0, 0},
      sdl_start_menu_hero_texture(nullptr), sdl_start_menu_hero_size{0, 0},
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
      sdl_goodie_texture(nullptr), sdl_logo_brick_surface(nullptr),
      sdl_dynamite_texture(nullptr), sdl_dynamite_size{0, 0},
      sdl_start_menu_monster_texture(nullptr), sdl_start_menu_monster_size{0, 0},
      sdl_start_menu_hero_texture(nullptr), sdl_start_menu_hero_size{0, 0},
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

  std::string app_icon_path = Paths::GetDataFilePath("app_icon_rgba.png");
  SDL_Surface *app_icon_surface = IMG_Load(app_icon_path.c_str());
  if (app_icon_surface == nullptr) {
    app_icon_path = Paths::GetDataFilePath("app_icon.png");
    app_icon_surface = IMG_Load(app_icon_path.c_str());
  }
  if (app_icon_surface != nullptr) {
    SDL_SetWindowIcon(sdl_window, app_icon_surface);
    SDL_FreeSurface(app_icon_surface);
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
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

  const std::string font_path = Paths::GetDataFilePath("font.ttf");
  sdl_font_hud = TTF_OpenFont(font_path.c_str(), hud_fontsize);
  sdl_font_menu =
      TTF_OpenFont(font_path.c_str(), std::max(30, screen_res_y / 22));
  sdl_font_logo =
      TTF_OpenFont(font_path.c_str(), std::max(84, screen_res_y / 5));
  sdl_font_display =
      TTF_OpenFont(font_path.c_str(), std::max(72, screen_res_y / 7));

  if (sdl_font_hud == nullptr || sdl_font_menu == nullptr ||
      sdl_font_logo == nullptr || sdl_font_display == nullptr) {
    std::cerr << "Could not open font: " << TTF_GetError() << "\n";
    exit(1);
  }

  sdl_font_color = kHudTextColor;

  const std::string wall_path = Paths::GetDataFilePath("brick.bmp");
  const std::string goodie_path = Paths::GetDataFilePath("goodie.bmp");

  sdl_wall_surface = SDL_LoadBMP(wall_path.c_str());
  sdl_goodie_surface = SDL_LoadBMP(goodie_path.c_str());

  const std::vector<std::string> monster_frame_paths{
      Paths::GetDataFilePath("monster.bmp"),
      Paths::GetDataFilePath("monster_1.bmp"),
      Paths::GetDataFilePath("monster_2.bmp"),
      Paths::GetDataFilePath("monster_3.bmp")};
  for (const std::string &monster_frame_path : monster_frame_paths) {
    SDL_Surface *monster_surface = SDL_LoadBMP(monster_frame_path.c_str());
    if (monster_surface == nullptr) {
      std::cerr << "Could not open monster sprite asset "
                << monster_frame_path << ": " << SDL_GetError() << "\n";
      exit(1);
    }
    sdl_monster_surfaces.push_back(monster_surface);
  }

  const std::vector<std::string> pacman_frame_paths{
      Paths::GetDataFilePath("pacman_frames/down_0.png"),
      Paths::GetDataFilePath("pacman_frames/down_1.png"),
      Paths::GetDataFilePath("pacman_frames/down_2.png"),
      Paths::GetDataFilePath("pacman_frames/down_3.png"),
      Paths::GetDataFilePath("pacman_frames/up_0.png"),
      Paths::GetDataFilePath("pacman_frames/up_1.png"),
      Paths::GetDataFilePath("pacman_frames/up_2.png"),
      Paths::GetDataFilePath("pacman_frames/up_3.png"),
      Paths::GetDataFilePath("pacman_frames/left_0.png"),
      Paths::GetDataFilePath("pacman_frames/left_1.png"),
      Paths::GetDataFilePath("pacman_frames/left_2.png"),
      Paths::GetDataFilePath("pacman_frames/left_3.png"),
      Paths::GetDataFilePath("pacman_frames/right_0.png"),
      Paths::GetDataFilePath("pacman_frames/right_1.png"),
      Paths::GetDataFilePath("pacman_frames/right_2.png"),
      Paths::GetDataFilePath("pacman_frames/right_3.png")};
  for (const std::string &pacman_frame_path : pacman_frame_paths) {
    SDL_Surface *pacman_surface = IMG_Load(pacman_frame_path.c_str());
    if (pacman_surface == nullptr) {
      std::cerr << "Could not open pacman sprite asset "
                << pacman_frame_path << ": " << IMG_GetError() << "\n";
      exit(1);
    }
    sdl_pacman_surfaces.push_back(pacman_surface);
  }

  if (sdl_goodie_surface == nullptr || sdl_wall_surface == nullptr) {
    std::cerr << "Could not open sprite assets: " << SDL_GetError() << "\n";
    exit(1);
  }

  sdl_wall_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_wall_surface);
  sdl_goodie_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_goodie_surface);
  for (SDL_Surface *pacman_surface : sdl_pacman_surfaces) {
    SDL_Texture *pacman_texture =
        SDL_CreateTextureFromSurface(sdl_renderer, pacman_surface);
    if (pacman_texture == nullptr) {
      std::cerr << "Could not create pacman texture: " << SDL_GetError()
                << "\n";
      exit(1);
    }
    sdl_pacman_textures.push_back(pacman_texture);
  }
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

  const std::string logo_brick_path = Paths::GetDataFilePath("brick.png");
  SDL_Surface *brick_surface = IMG_Load(logo_brick_path.c_str());
  if (brick_surface == nullptr) {
    brick_surface = SDL_LoadBMP(wall_path.c_str());
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

  sdl_start_menu_monster_texture = loadTrimmedTexture(
      Paths::GetDataFilePath("start_menu_monster.png"),
      sdl_start_menu_monster_size);
  sdl_start_menu_hero_texture = loadTrimmedTexture(
      Paths::GetDataFilePath("start_menu_spielfigur.png"),
      sdl_start_menu_hero_size);
  sdl_dynamite_texture = loadTrimmedTexture(Paths::GetDataFilePath("dynamite.png"),
                                            sdl_dynamite_size);
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
  for (SDL_Texture *pacman_texture : sdl_pacman_textures) {
    if (pacman_texture != nullptr) {
      SDL_DestroyTexture(pacman_texture);
    }
  }
  if (sdl_start_menu_monster_texture != nullptr) {
    SDL_DestroyTexture(sdl_start_menu_monster_texture);
  }
  if (sdl_start_menu_hero_texture != nullptr) {
    SDL_DestroyTexture(sdl_start_menu_hero_texture);
  }
  if (sdl_dynamite_texture != nullptr) {
    SDL_DestroyTexture(sdl_dynamite_texture);
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
  for (SDL_Surface *pacman_surface : sdl_pacman_surfaces) {
    if (pacman_surface != nullptr) {
      SDL_FreeSurface(pacman_surface);
    }
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

bool Renderer::TryGetLayoutCoordFromScreen(int screen_x, int screen_y,
                                           MapCoord &coord) const {
  const int grid_origin_x = offset_x + 1;
  const int grid_origin_y = offset_y + 1;
  const int cell_stride = element_size + 1;
  const int relative_x = screen_x - grid_origin_x;
  const int relative_y = screen_y - grid_origin_y;

  if (relative_x < 0 || relative_y < 0) {
    return false;
  }

  const int col = relative_x / cell_stride;
  const int row = relative_y / cell_stride;
  if (row < 0 || col < 0 || row >= static_cast<int>(rows) ||
      col >= static_cast<int>(cols)) {
    return false;
  }

  if (relative_x % cell_stride >= element_size ||
      relative_y % cell_stride >= element_size) {
    return false;
  }

  coord = {row, col};
  return true;
}

void Renderer::renderFrame(bool show_hud) {
  SDL_SetRenderDrawColor(sdl_renderer, COLOR_BACK, 255);
  SDL_RenderClear(sdl_renderer);
  drawmap();
  drawteleporters();
  if (game != nullptr) {
    drawgoodies();
    drawbonusflask();
    drawdynamitepickup();
    drawplasticexplosivepickup();
    drawpacman();
    drawplaceddynamites();
    drawplacedplasticexplosive();
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

  const std::string title_text = "BOBMAN";
  const std::string score_text = "Score: " + std::to_string(game->score);
  const std::string dynamite_text = std::to_string(game->dynamite_inventory) + "x";
  const std::string plastic_explosive_text =
      std::to_string(game->plastic_explosive_inventory) + "x";
  int title_width = 0;
  int score_width = 0;
  int dynamite_text_width = 0;
  int plastic_explosive_text_width = 0;
  TTF_SizeUTF8(sdl_font_hud, title_text.c_str(), &title_width, nullptr);
  TTF_SizeUTF8(sdl_font_hud, score_text.c_str(), &score_width, nullptr);
  if (game->dynamite_inventory > 0) {
    TTF_SizeUTF8(sdl_font_hud, dynamite_text.c_str(), &dynamite_text_width,
                 nullptr);
  }
  if (game->plastic_explosive_inventory > 0) {
    TTF_SizeUTF8(sdl_font_hud, plastic_explosive_text.c_str(),
                 &plastic_explosive_text_width, nullptr);
  }

  const int line_top = 30;
  const int hud_gap = std::max(14, element_size / 2);
  const int icon_size = std::max(18, TTF_FontHeight(sdl_font_hud) - 2);
  int line_width = title_width + hud_gap + score_width;
  if (game->dynamite_inventory > 0) {
    line_width += hud_gap + icon_size + 10 + dynamite_text_width;
  }
  if (game->plastic_explosive_inventory > 0) {
    line_width += hud_gap + icon_size + 10 + plastic_explosive_text_width;
  }

  int cursor_x = screen_res_x / 2 - line_width / 2;
  renderTextLeft(sdl_font_hud, title_text, sdl_font_color, cursor_x, line_top);
  cursor_x += title_width + hud_gap;
  renderTextLeft(sdl_font_hud, score_text, sdl_font_color, cursor_x, line_top);
  cursor_x += score_width;

  if (game->dynamite_inventory > 0) {
    cursor_x += hud_gap;
    const SDL_Rect icon_rect{
        cursor_x, line_top + std::max(0, (TTF_FontHeight(sdl_font_hud) - icon_size) / 2),
        icon_size, icon_size};
    drawDynamiteIcon(icon_rect, false, static_cast<double>(SDL_GetTicks()) / 180.0,
                     255, 0.0);
    cursor_x += icon_rect.w + 10;
    renderTextLeft(sdl_font_hud, dynamite_text, sdl_font_color, cursor_x,
                   line_top);
    cursor_x += dynamite_text_width;
  }

  if (game->plastic_explosive_inventory > 0) {
    cursor_x += hud_gap;
    const SDL_Rect icon_rect{
        cursor_x, line_top + std::max(0, (TTF_FontHeight(sdl_font_hud) - icon_size) / 2),
        icon_size, icon_size};
    drawPlasticExplosiveIcon(icon_rect, static_cast<double>(SDL_GetTicks()) / 170.0,
                             255, false);
    cursor_x += icon_rect.w + 10;
    renderTextLeft(sdl_font_hud, plastic_explosive_text, sdl_font_color, cursor_x,
                   line_top);
  }

  const Uint32 now = SDL_GetTicks();
  if (game->pacman != nullptr && game->pacman->invulnerable_until_ticks > now) {
    const Uint32 remaining_ms = game->pacman->invulnerable_until_ticks - now;
    const int remaining_seconds =
        static_cast<int>((remaining_ms + 999) / 1000);
    renderSimpleText(sdl_font_hud,
                     "Schild: " + std::to_string(remaining_seconds) + "s",
                     kShieldTextColor, screen_res_x / 2,
                     30 + TTF_FontHeight(sdl_font_hud) + 6);
  }

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
                         border_color.b, std::min<Uint8>(100, border_color.a));
  SDL_RenderDrawRect(sdl_renderer, &inner_panel);
}

void Renderer::drawStartMenuSpectrum(const SDL_Rect &panel) {
  const int available_band_count = Audio::kMenuSpectrumBandCount;
  const int available_half_band_count = std::max(1, available_band_count / 2);
  const int configured_half_band_count =
      std::max(1, START_MENU_LED_HALF_BAR_COUNT);
  const int band_count = configured_half_band_count * 2;
  constexpr int kSegmentCount = 8;
  const int segment_gap = std::max(0, START_MENU_LED_SEGMENT_GAP_X);
  const int band_gap = std::max(0, START_MENU_LED_SEGMENT_GAP_Y);
  const int menu_gap = std::max(0, START_MENU_LED_MENU_MARGIN);
  const int outer_margin = std::max(0, START_MENU_LED_OUTER_MARGIN);
  const int left_inner = panel.x - menu_gap;
  const int left_outer = outer_margin;
  const int right_inner = panel.x + panel.w + menu_gap;
  const int right_outer = screen_res_x - outer_margin;
  const int left_max_extent = left_inner - left_outer;
  const int right_max_extent = right_outer - right_inner;
  const int minimum_bar_width =
      kSegmentCount + (kSegmentCount - 1) * segment_gap;
  if (left_max_extent < minimum_bar_width ||
      right_max_extent < minimum_bar_width) {
    return;
  }

  const int axis_gap = 0;
  const int usable_height = panel.h - outer_margin * 2 - axis_gap -
                            band_gap * std::max(0, band_count - 1);
  if (usable_height < band_count) {
    return;
  }

  const int safe_band_count = std::max(1, band_count);
  const int base_band_height = usable_height / safe_band_count;
  if (base_band_height <= 0) {
    return;
  }
  const int band_height_remainder = usable_height % safe_band_count;
  const int top_start_y = panel.y + outer_margin;
  const Uint32 now = SDL_GetTicks();
  std::array<float, Audio::kMenuSpectrumBandCount> levels =
      Audio::GetMenuSpectrumLevels();

  float peak_level = 0.0f;
  for (float level : levels) {
    peak_level = std::max(peak_level, level);
  }

  if (peak_level < 0.02f) {
    const double clock = static_cast<double>(now);
    for (int band = 0; band < available_band_count; ++band) {
      const double progress =
          static_cast<double>(band) / std::max(1, available_band_count - 1);
      const double wave_a =
          std::sin(clock / 410.0 + progress * 7.4) * 0.28 + 0.34;
      const double wave_b =
          std::sin(clock / 930.0 - progress * 11.8) * 0.18 + 0.18;
      levels[static_cast<size_t>(band)] = static_cast<float>(
          std::clamp(wave_a + wave_b, 0.06, 0.72));
    }
  }

  auto draw_gradient_bar = [&](const SDL_Rect &rect,
                               const SDL_Color &start_color,
                               const SDL_Color &end_color, Uint8 alpha,
                               int inset = 0) {
    if (rect.w <= 0 || rect.h <= 0) {
      return;
    }

    const int y0 = rect.y + inset;
    const int y1 = rect.y + rect.h - 1 - inset;
    if (y1 < y0) {
      return;
    }

    for (int offset = 0; offset < rect.w; ++offset) {
      const float factor =
          (rect.w > 1)
              ? static_cast<float>(offset) / static_cast<float>(rect.w - 1)
              : 0.0f;
      const SDL_Color color = LerpColor(start_color, end_color, factor);
      SDL_SetRenderDrawColor(sdl_renderer, color.r, color.g, color.b, alpha);
      SDL_RenderDrawLine(sdl_renderer, rect.x + offset, y0, rect.x + offset, y1);
    }
  };

  auto draw_rounded_gradient_segment = [&](const SDL_Rect &rect,
                                           const SDL_Color &start_color,
                                           const SDL_Color &end_color,
                                           const SDL_Color &border_start_color,
                                           const SDL_Color &border_end_color,
                                           Uint8 fill_alpha,
                                           Uint8 border_alpha) {
    if (rect.w <= 1 || rect.h <= 1) {
      return;
    }
    const int border_thickness = std::clamp(std::min(rect.w, rect.h) / 5, 1, 2);
    draw_gradient_bar(rect, start_color, end_color, fill_alpha);

    const SDL_Color highlight_start{255, 255, 255, 60};
    const SDL_Color highlight_end{255, 255, 255, 12};
    SDL_Rect highlight_rect{
        rect.x + border_thickness,
        rect.y + border_thickness,
        std::max(1, rect.w - border_thickness * 2),
        std::max(1, rect.h / 2 - std::max(0, border_thickness - 1))};
    draw_gradient_bar(
        highlight_rect, highlight_start, highlight_end,
        std::min<Uint8>(static_cast<Uint8>(fill_alpha / 2 + 30), 118));

    const SDL_Rect top_border{rect.x, rect.y, rect.w, border_thickness};
    const SDL_Rect bottom_border{rect.x, rect.y + rect.h - border_thickness,
                                 rect.w, border_thickness};
    draw_gradient_bar(top_border, border_start_color, border_end_color,
                      border_alpha);
    draw_gradient_bar(
        bottom_border,
        LerpColor(border_start_color, SDL_Color{18, 16, 26, 255}, 0.12f),
        LerpColor(border_end_color, SDL_Color{18, 16, 26, 255}, 0.18f),
        static_cast<Uint8>(std::max(36, border_alpha - 12)));

    SDL_Rect left_border{rect.x, rect.y, border_thickness, rect.h};
    SDL_SetRenderDrawColor(sdl_renderer, border_start_color.r,
                           border_start_color.g, border_start_color.b,
                           border_alpha);
    SDL_RenderFillRect(sdl_renderer, &left_border);

    SDL_Rect right_border{rect.x + rect.w - border_thickness, rect.y,
                          border_thickness, rect.h};
    SDL_SetRenderDrawColor(sdl_renderer, border_end_color.r,
                           border_end_color.g, border_end_color.b, border_alpha);
    SDL_RenderFillRect(sdl_renderer, &right_border);
  };

  auto spectrum_color = [&](float factor) {
    factor = std::clamp(factor, 0.0f, 1.0f);
    struct ColorStop {
      float position;
      SDL_Color color;
    };
    constexpr std::array<ColorStop, 6> kStops{{
        {0.00f, SDL_Color{24, 132, 255, 255}},
        {0.20f, SDL_Color{0, 232, 255, 255}},
        {0.40f, SDL_Color{0, 255, 154, 255}},
        {0.60f, SDL_Color{255, 238, 0, 255}},
        {0.80f, SDL_Color{255, 136, 0, 255}},
        {1.00f, SDL_Color{255, 24, 24, 255}},
    }};

    for (size_t index = 1; index < kStops.size(); ++index) {
      if (factor <= kStops[index].position) {
        const float start = kStops[index - 1].position;
        const float range = std::max(0.0001f, kStops[index].position - start);
        const float local = (factor - start) / range;
        return LerpColor(kStops[index - 1].color, kStops[index].color, local);
      }
    }
    return kStops.back().color;
  };

  auto draw_segmented_bar = [&](const SDL_Rect &bar_rect, float level,
                                bool anchor_from_right) {
    if (bar_rect.w <= 0 || bar_rect.h <= 0) {
      return;
    }
    const int total_gap = (kSegmentCount - 1) * segment_gap;
    const int total_segment_width = bar_rect.w - total_gap;
    const int raw_segment_w = total_segment_width / kSegmentCount;
    if (raw_segment_w <= 0) {
      return;
    }
    const int remainder = total_segment_width % kSegmentCount;
    const float clamped_level = std::clamp(level, 0.0f, 1.0f);

    int cursor_x = bar_rect.x;
    for (int segment = 0; segment < kSegmentCount; ++segment) {
      int segment_w = raw_segment_w + (segment < remainder ? 1 : 0);
      if (segment_w <= 0) {
        cursor_x += segment_gap;
        continue;
      }

      const int distance_from_menu =
          anchor_from_right ? (kSegmentCount - 1 - segment) : segment;
      const float threshold = static_cast<float>(distance_from_menu + 1) /
                              static_cast<float>(kSegmentCount);
      const bool active = clamped_level >= threshold;
      SDL_Rect segment_rect{cursor_x, bar_rect.y, segment_w, bar_rect.h};
      float gradient = static_cast<float>(distance_from_menu) /
                       static_cast<float>(kSegmentCount - 1);
      const SDL_Color core = spectrum_color(gradient);
      if (active) {
        const SDL_Color edge = LerpColor(core, SDL_Color{255, 255, 255, 255}, 0.24f);
        const SDL_Color border_core =
            LerpColor(core, SDL_Color{18, 14, 24, 255}, 0.16f);
        const SDL_Color border_edge =
            LerpColor(edge, SDL_Color{22, 16, 26, 255}, 0.22f);
        draw_rounded_gradient_segment(segment_rect, core, edge, border_core,
                                      border_edge, kStartMenuLedFillAlpha,
                                      kStartMenuLedBorderAlpha);
      }
      cursor_x += segment_w + segment_gap;
    }
  };

  auto draw_spectrum_row = [&](int band_index, int y, int band_height) {
    const float level =
        std::clamp(levels[static_cast<size_t>(band_index)], 0.0f, 1.0f);
    const float eased_level = std::pow(level, 0.82f);
    const float extended_level = std::clamp(eased_level * 1.30f, 0.0f, 1.0f);
    const SDL_Rect left_bar{left_outer, y, left_max_extent, band_height};
    const SDL_Rect right_bar{right_inner, y, right_max_extent, band_height};
    draw_segmented_bar(left_bar, extended_level, true);
    draw_segmented_bar(right_bar, extended_level, false);
  };

  int current_y = top_start_y;
  for (int row = 0; row < band_count; ++row) {
    const int mirrored_index =
        (row < configured_half_band_count)
            ? (configured_half_band_count - 1 - row)
            : (row - configured_half_band_count);
    const float row_progress =
        (configured_half_band_count > 1)
            ? static_cast<float>(mirrored_index) /
                  static_cast<float>(configured_half_band_count - 1)
            : 0.0f;
    const int band_index = std::clamp(
        static_cast<int>(std::lround(row_progress *
                                     static_cast<float>(available_half_band_count - 1))),
        0, available_half_band_count - 1);
    const int band_height =
        base_band_height + (row < band_height_remainder ? 1 : 0);
    draw_spectrum_row(band_index, current_y, band_height);
    current_y += band_height + band_gap;
    if (row == configured_half_band_count - 1) {
      current_y += axis_gap;
    }
  }
}

void Renderer::drawStartMenuOverlay(int selected_item,
                                    const std::string &map_name,
                                    const std::string &status_message) {
  const int logo_top = screen_res_y / 18;
  const int logo_center_x = screen_res_x / 2;
  int logo_width = 0;
  int logo_height = TTF_FontHeight(sdl_font_logo);
  if (TTF_SizeUTF8(sdl_font_logo, "BobMan", &logo_width, &logo_height) != 0) {
    logo_width = screen_res_x / 3;
    logo_height = TTF_FontHeight(sdl_font_logo);
  }
  const int logo_left = logo_center_x - logo_width / 2;
  const int logo_right = logo_center_x + logo_width / 2;

  const std::vector<std::string> menu_items{
      "Start Spiel", "Karte: " + map_name, "Map Editor", "Konfiguration",
      "Ende"};
  const int item_height = std::max(52, TTF_FontHeight(sdl_font_menu) + 18);
  const int panel_width = std::min(780, screen_res_x * 46 / 100);
  const int panel_height =
      std::max(360, 88 + static_cast<int>(menu_items.size()) * item_height);
  const int header_side_margin = std::max(18, screen_res_x / 34);
  const int mascot_gap = std::max(10, screen_res_x / 130);
  const int available_mascot_width =
      std::max(0, (screen_res_x - logo_width - header_side_margin * 2 -
                   mascot_gap * 2) /
                      2);
  const int mascot_target_height =
      std::clamp(static_cast<int>(logo_height * 1.42), 120, screen_res_y / 4);

  auto scaled_mascot_size = [&](const SDL_Point &source_size) {
    SDL_Point scaled_size{0, 0};
    if (source_size.x <= 0 || source_size.y <= 0 || available_mascot_width < 96) {
      return scaled_size;
    }

    const double scale_by_width =
        static_cast<double>(available_mascot_width) /
        static_cast<double>(source_size.x);
    const double scale_by_height =
        static_cast<double>(mascot_target_height) /
        static_cast<double>(source_size.y);
    const double scale = std::min(scale_by_width, scale_by_height);
    if (scale <= 0.0) {
      return scaled_size;
    }

    scaled_size.x =
        std::max(1, static_cast<int>(std::lround(source_size.x * scale)));
    scaled_size.y =
        std::max(1, static_cast<int>(std::lround(source_size.y * scale)));
    return scaled_size;
  };

  const SDL_Point monster_size = scaled_mascot_size(sdl_start_menu_monster_size);
  const SDL_Point hero_size = scaled_mascot_size(sdl_start_menu_hero_size);
  SDL_Rect monster_rect{0, 0, 0, 0};
  SDL_Rect hero_rect{0, 0, 0, 0};
  if (monster_size.x > 0 && monster_size.y > 0) {
    monster_rect = SDL_Rect{
        std::max(header_side_margin, logo_left - mascot_gap - monster_size.x),
        std::max(8, logo_top + (logo_height - monster_size.y) / 2 + logo_height / 16),
        monster_size.x, monster_size.y};
  }
  if (hero_size.x > 0 && hero_size.y > 0) {
    hero_rect = SDL_Rect{
        std::min(screen_res_x - header_side_margin - hero_size.x,
                 logo_right + mascot_gap),
        std::max(8, logo_top + (logo_height - hero_size.y) / 2 + logo_height / 10),
        hero_size.x, hero_size.y};
  }

  int header_bottom = logo_top + logo_height;
  if (monster_rect.w > 0 && monster_rect.h > 0) {
    header_bottom = std::max(header_bottom, monster_rect.y + monster_rect.h);
  }
  if (hero_rect.w > 0 && hero_rect.h > 0) {
    header_bottom = std::max(header_bottom, hero_rect.y + hero_rect.h);
  }
  const int panel_top =
      std::min(screen_res_y - panel_height - 70,
               header_bottom + screen_res_y / 28);
  SDL_Rect panel{(screen_res_x - panel_width) / 2, panel_top, panel_width,
                 panel_height};

  renderDecorativeTexture(sdl_start_menu_monster_texture, monster_rect,
                          SDL_Color{180, 96, 255, 255}, 0.25, 0.055, 0.07, 0.62);
  renderDecorativeTexture(sdl_start_menu_hero_texture, hero_rect,
                          SDL_Color{88, 196, 255, 255}, 1.35, 0.0, 0.0, 0.0);
  renderStartLogo(sdl_font_logo, "BobMan", logo_center_x, logo_top);

  drawStartMenuSpectrum(panel);
  drawPanel(panel, kStartMenuPanelFillColor, kStartMenuPanelBorderColor);

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
  renderBrickText(sdl_font_logo, "BobMan", screen_res_x / 2, logo_top,
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
  renderBrickText(sdl_font_logo, "BobMan", screen_res_x / 2, logo_top,
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
  renderBrickText(sdl_font_logo, "BobMan", screen_res_x / 2, logo_top,
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
  renderBrickText(sdl_font_logo, "BobMan", screen_res_x / 2, logo_top,
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
  const int header_top = 12;
  renderBrickText(sdl_font_menu, "Map Editor", screen_res_x / 2, header_top,
                  kBrickOutlineColor);
  renderSimpleText(sdl_font_hud, map_name, kHudTextColor, screen_res_x / 2,
                   header_top + TTF_FontHeight(sdl_font_menu) + 4);

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
  const int info_height = std::max(170, screen_res_y / 4);
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
      "Pfeile oder Linksklick | Klick schaltet Mauer an/aus";
  const std::string line_two =
      "X/W=Mauer | Leer/Entf=Weg | G=Goodie | P=Spielfigur";
  const std::string line_three =
      "M/N/O=Monster | 1-5=Teleportpaare";
  const std::string line_four =
      "Belegte Felder blockieren den Klick | Esc oeffnet Dialog";
  renderSimpleText(sdl_font_hud, line_one, kHudTextColor, screen_res_x / 2,
                   info_panel.y + 16);
  renderSimpleText(sdl_font_hud, line_two, kHudTextColor, screen_res_x / 2,
                   info_panel.y + 16 + TTF_FontHeight(sdl_font_hud) + 6);
  renderSimpleText(sdl_font_hud, line_three, kHudTextColor, screen_res_x / 2,
                   info_panel.y + 16 + 2 * (TTF_FontHeight(sdl_font_hud) + 6));
  renderSimpleText(sdl_font_hud, line_four, kHudTextColor, screen_res_x / 2,
                   info_panel.y + 16 + 3 * (TTF_FontHeight(sdl_font_hud) + 6));

  const SDL_Color status_color =
      warning_message.empty() ? kStatusTextColor : kWarningTextColor;
  const std::string status_text =
      warning_message.empty() ? "Karte ist gueltig" : warning_message;
  renderSimpleText(sdl_font_hud, status_text, status_color, screen_res_x / 2,
                   info_panel.y + info_panel.h - TTF_FontHeight(sdl_font_hud) - 14);

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

void Renderer::renderTextLeft(TTF_Font *font, const std::string &text,
                              const SDL_Color &color, int left_x, int top_y) {
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
    SDL_Rect destination{left_x, top_y, text_surface->w, text_surface->h};
    SDL_RenderCopy(sdl_renderer, text_texture, nullptr, &destination);
    SDL_DestroyTexture(text_texture);
  }

  SDL_FreeSurface(text_surface);
}

void Renderer::renderStartLogo(TTF_Font *font, const std::string &text,
                               int center_x, int top_y) {
  if (text.empty()) {
    return;
  }

  SDL_Surface *logo_surface = createStartLogoSurface(font, text);
  if (logo_surface == nullptr) {
    return;
  }

  SDL_Texture *logo_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, logo_surface);
  if (logo_texture == nullptr) {
    SDL_FreeSurface(logo_surface);
    return;
  }

  const int logo_width = logo_surface->w;
  const int logo_height = logo_surface->h;
  const Uint32 now = SDL_GetTicks();
  const double clock = static_cast<double>(now);
  const double base_sway_x =
      std::sin(clock / 620.0) * std::max(4.0, logo_width * 0.010);
  const double base_sway_y =
      std::sin(clock / 890.0 + 0.7) * std::max(2.0, logo_height * 0.040);

  SDL_SetTextureBlendMode(logo_texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureColorMod(logo_texture, 6, 18, 86);
  SDL_SetTextureAlphaMod(logo_texture, 176);
  const int shadow_offset = std::max(5, TTF_FontHeight(font) / 20);
  SDL_Rect shadow_rect{
      center_x - logo_width / 2 + static_cast<int>(std::lround(base_sway_x)) +
          shadow_offset,
      top_y + static_cast<int>(std::lround(base_sway_y)) + shadow_offset,
      logo_width, logo_height};
  SDL_RenderCopy(sdl_renderer, logo_texture, nullptr, &shadow_rect);

  SDL_SetTextureBlendMode(logo_texture, SDL_BLENDMODE_ADD);
  for (int pass = 0; pass < 3; ++pass) {
    const int padding = std::max(6, (pass + 1) * TTF_FontHeight(font) / 18);
    const double glow_clock = clock / (680.0 + pass * 110.0) + pass * 0.9;
    SDL_SetTextureColorMod(
        logo_texture, static_cast<Uint8>(72 + pass * 32),
        static_cast<Uint8>(156 + pass * 24), 255);
    SDL_SetTextureAlphaMod(logo_texture, static_cast<Uint8>(58 - pass * 14));
    SDL_Rect glow_rect{
        center_x - logo_width / 2 - padding +
            static_cast<int>(std::lround(base_sway_x * 0.7 +
                                         std::sin(glow_clock) * (pass + 1))),
        top_y - padding / 2 +
            static_cast<int>(std::lround(base_sway_y * 0.5 +
                                         std::cos(glow_clock * 0.9) * pass)),
        logo_width + padding * 2, logo_height + padding};
    SDL_RenderCopy(sdl_renderer, logo_texture, nullptr, &glow_rect);
  }

  SDL_SetTextureBlendMode(logo_texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureColorMod(logo_texture, 255, 255, 255);
  SDL_SetTextureAlphaMod(logo_texture, 255);

  const int slice_height = std::max(4, logo_height / 18);
  const int slice_count =
      std::max(1, (logo_height + slice_height - 1) / slice_height);
  struct SliceTransform {
    int src_y;
    int src_h;
    int dest_x;
    int dest_y;
    int dest_width;
  };
  std::vector<SliceTransform> slice_transforms;
  slice_transforms.reserve(static_cast<size_t>(slice_count));

  // Draw the logo in thin horizontal slices so each band can lag behind the
  // main sway and feel like stacked retro layers.
  for (int slice = 0; slice < slice_count; ++slice) {
    const int src_y = slice * slice_height;
    const int src_h = std::min(slice_height + 1, logo_height - src_y);
    const double progress =
        (slice_count > 1)
            ? static_cast<double>(slice) / static_cast<double>(slice_count - 1)
            : 0.0;
    const double trailing_phase = progress * 2.6;
    const double layer_drag =
        std::sin(clock / 340.0 - trailing_phase * 1.7) *
        std::max(2.0, logo_width * 0.006);
    const double layer_pull =
        std::sin(clock / 760.0 - trailing_phase) *
        std::max(3.0, logo_width * 0.008);
    const double stretch =
        1.0 + 0.026 * std::sin(clock / 470.0 + progress * 5.5) +
        0.010 * std::sin(clock / 230.0 - progress * 6.3);
    const int dest_width =
        std::max(1, static_cast<int>(std::lround(logo_width * stretch)));
    const int dest_x = center_x - dest_width / 2 +
                       static_cast<int>(std::lround(base_sway_x + layer_drag +
                                                    layer_pull));
    const int dest_y =
        top_y + src_y +
        static_cast<int>(std::lround(base_sway_y +
                                     std::sin(clock / 720.0 + progress * 4.1)));
    const SDL_Rect src_rect{0, src_y, logo_width, src_h};
    const SDL_Rect slice_shadow_rect{dest_x + 2, dest_y + 2, dest_width,
                                     src_h + 1};
    SDL_SetTextureColorMod(logo_texture, 8, 26, 100);
    SDL_SetTextureAlphaMod(logo_texture, 96);
    SDL_RenderCopy(sdl_renderer, logo_texture, &src_rect, &slice_shadow_rect);

    SDL_SetTextureColorMod(logo_texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(logo_texture, 255);
    const SDL_Rect dest_rect{dest_x, dest_y, dest_width, src_h + 1};
    SDL_RenderCopy(sdl_renderer, logo_texture, &src_rect, &dest_rect);
    slice_transforms.push_back(
        SliceTransform{src_y, src_h, dest_x, dest_y, dest_width});
  }

  const bool lock_logo_surface = SDL_MUSTLOCK(logo_surface);
  if (lock_logo_surface) {
    SDL_LockSurface(logo_surface);
  }

  auto logo_alpha = [&](int px, int py) -> Uint8 {
    if (px < 0 || py < 0 || px >= logo_width || py >= logo_height) {
      return 0;
    }

    Uint8 red = 0;
    Uint8 green = 0;
    Uint8 blue = 0;
    Uint8 alpha = 0;
    SDL_GetRGBA(readPixel(logo_surface, px, py), logo_surface->format, &red,
                &green, &blue, &alpha);
    return alpha;
  };

  auto find_logo_anchor = [&](float norm_x, float norm_y) -> SDL_Point {
    const int candidate_x = std::clamp(
        static_cast<int>(std::lround(norm_x * static_cast<float>(logo_width - 1))),
        0, std::max(0, logo_width - 1));
    const int candidate_y = std::clamp(
        static_cast<int>(std::lround(norm_y * static_cast<float>(logo_height - 1))),
        0, std::max(0, logo_height - 1));
    if (logo_alpha(candidate_x, candidate_y) > 24) {
      return SDL_Point{candidate_x, candidate_y};
    }

    constexpr int kSearchRadius = 26;
    for (int radius = 1; radius <= kSearchRadius; ++radius) {
      for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
          const int probe_x = candidate_x + dx;
          const int probe_y = candidate_y + dy;
          if (logo_alpha(probe_x, probe_y) > 24) {
            return SDL_Point{probe_x, probe_y};
          }
        }
      }
    }

    return SDL_Point{-1, -1};
  };

  struct SparkleLane {
    double phase_seconds;
    double cycle_seconds;
    double visible_fraction;
    float min_scale;
    float max_scale;
  };
  constexpr std::array<SparkleLane, 8> kSparkleLanes{{
      {0.20, 2.6, 0.26, 0.84f, 1.18f},
      {0.58, 3.1, 0.22, 0.78f, 1.06f},
      {1.02, 2.8, 0.28, 0.88f, 1.24f},
      {1.46, 3.4, 0.24, 0.74f, 1.08f},
      {1.88, 2.9, 0.25, 0.82f, 1.14f},
      {2.32, 3.7, 0.21, 0.76f, 1.02f},
      {2.74, 3.0, 0.27, 0.86f, 1.20f},
      {3.18, 4.1, 0.23, 0.80f, 1.12f},
  }};

  for (size_t sparkle_index = 0; sparkle_index < kSparkleLanes.size();
       ++sparkle_index) {
    const SparkleLane &sparkle = kSparkleLanes[sparkle_index];
    const double cycle_clock = clock / 1000.0 + sparkle.phase_seconds;
    const double cycle_index = std::floor(cycle_clock / sparkle.cycle_seconds);
    const double cycle_position =
        std::fmod(cycle_clock, sparkle.cycle_seconds) / sparkle.cycle_seconds;
    if (cycle_position < 0.0 || cycle_position > sparkle.visible_fraction) {
      continue;
    }

    const double local_progress = cycle_position / sparkle.visible_fraction;
    const double pulse = std::sin(local_progress * kLogoPi);
    if (pulse <= 0.01) {
      continue;
    }

    const auto random_fraction = [&](double salt) {
      const double raw_random =
          std::sin((cycle_index + 1.0) * (117.1 + salt * 0.9) +
                   sparkle.phase_seconds * (39.7 + salt * 0.3) +
                   static_cast<double>(sparkle_index + 1) * (61.3 + salt * 0.7)) *
          43758.5453123;
      double fraction = raw_random - std::floor(raw_random);
      if (fraction < 0.0) {
        fraction += 1.0;
      }
      return fraction;
    };

    const float norm_x =
        static_cast<float>(0.08 + random_fraction(11.0) * 0.84);
    const float norm_y =
        static_cast<float>(0.10 + random_fraction(23.0) * 0.72);
    const double intensity = std::pow(pulse, 0.62);

    const SDL_Point anchor = find_logo_anchor(norm_x, norm_y);
    if (anchor.x < 0 || anchor.y < 0) {
      continue;
    }

    const auto slice_it = std::find_if(
        slice_transforms.begin(), slice_transforms.end(),
        [&](const SliceTransform &slice) {
          return anchor.y >= slice.src_y && anchor.y < slice.src_y + slice.src_h;
        });
    if (slice_it == slice_transforms.end()) {
      continue;
    }

    const double x_ratio =
        (logo_width > 1)
            ? static_cast<double>(anchor.x) / static_cast<double>(logo_width - 1)
            : 0.0;
    const int sparkle_center_x = slice_it->dest_x +
                                 static_cast<int>(std::lround(
                                     x_ratio * static_cast<double>(slice_it->dest_width)));
    const int sparkle_center_y = slice_it->dest_y + (anchor.y - slice_it->src_y);
    const double randomized_scale =
        sparkle.min_scale +
        random_fraction(31.0) * (sparkle.max_scale - sparkle.min_scale);
    const int halo_arm = std::max(
        5, static_cast<int>(std::lround(logo_height * 0.126 * randomized_scale *
                                        (0.40 + intensity * 0.92))));
    const int major_arm = std::max(
        4, static_cast<int>(std::lround(logo_height * 0.104 * randomized_scale *
                                        (0.50 + intensity * 0.86))));
    const int inner_arm = std::max(
        2, static_cast<int>(std::lround(major_arm *
                                        (0.46 + random_fraction(41.0) * 0.16))));
    const int outer_thickness = std::max(
        1, static_cast<int>(std::lround(
               1.0 + intensity * (1.8 + random_fraction(53.0) * 1.4))));
    const int inner_thickness = std::max(1, outer_thickness - 1);
    const Uint8 halo_alpha = static_cast<Uint8>(
        std::clamp(static_cast<int>(std::lround(150 * intensity + 24)), 0, 255));
    const Uint8 outer_alpha = static_cast<Uint8>(
        std::clamp(static_cast<int>(std::lround(228 * intensity + 16)), 0, 255));
    const Uint8 inner_alpha = static_cast<Uint8>(
        std::clamp(static_cast<int>(std::lround(255 * std::pow(intensity, 0.86))),
                   0, 255));

    auto draw_four_point_star = [&](const SDL_Color &color, Uint8 alpha,
                                    int arm_length, int thickness) {
      SDL_SetRenderDrawColor(sdl_renderer, color.r, color.g, color.b, alpha);
      for (int offset = -thickness; offset <= thickness; ++offset) {
        const int taper = std::abs(offset) * 2;
        const int horizontal_length = std::max(1, arm_length - taper);
        const int vertical_length = std::max(1, arm_length - taper);
        SDL_RenderDrawLine(sdl_renderer, sparkle_center_x - horizontal_length,
                           sparkle_center_y + offset,
                           sparkle_center_x + horizontal_length,
                           sparkle_center_y + offset);
        SDL_RenderDrawLine(sdl_renderer, sparkle_center_x + offset,
                           sparkle_center_y - vertical_length,
                           sparkle_center_x + offset,
                           sparkle_center_y + vertical_length);
      }
    };

    draw_four_point_star(SDL_Color{88, 214, 255, 255}, halo_alpha, halo_arm,
                         outer_thickness + 1);
    draw_four_point_star(SDL_Color{184, 240, 255, 255}, outer_alpha, major_arm,
                         outer_thickness);
    draw_four_point_star(SDL_Color{255, 255, 255, 255}, inner_alpha, inner_arm,
                         inner_thickness);
  }

  if (lock_logo_surface) {
    SDL_UnlockSurface(logo_surface);
  }

  SDL_DestroyTexture(logo_texture);
  SDL_FreeSurface(logo_surface);
}

SDL_Texture *Renderer::loadTrimmedTexture(const std::string &path,
                                          SDL_Point &trimmed_size) {
  trimmed_size = SDL_Point{0, 0};
  SDL_Surface *raw_surface = IMG_Load(path.c_str());
  if (raw_surface == nullptr) {
    std::cerr << "Could not open decorative start menu art " << path << ": "
              << IMG_GetError() << "\n";
    return nullptr;
  }

  SDL_Surface *rgba_surface =
      SDL_ConvertSurfaceFormat(raw_surface, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(raw_surface);
  if (rgba_surface == nullptr) {
    std::cerr << "Could not convert decorative art " << path << ": "
              << SDL_GetError() << "\n";
    return nullptr;
  }

  const bool lock_surface = SDL_MUSTLOCK(rgba_surface);
  if (lock_surface) {
    SDL_LockSurface(rgba_surface);
  }

  int min_x = rgba_surface->w;
  int min_y = rgba_surface->h;
  int max_x = -1;
  int max_y = -1;
  for (int y = 0; y < rgba_surface->h; ++y) {
    for (int x = 0; x < rgba_surface->w; ++x) {
      Uint8 red = 0;
      Uint8 green = 0;
      Uint8 blue = 0;
      Uint8 alpha = 0;
      SDL_GetRGBA(readPixel(rgba_surface, x, y), rgba_surface->format, &red,
                  &green, &blue, &alpha);
      if (alpha <= 8) {
        continue;
      }
      min_x = std::min(min_x, x);
      min_y = std::min(min_y, y);
      max_x = std::max(max_x, x);
      max_y = std::max(max_y, y);
    }
  }

  if (lock_surface) {
    SDL_UnlockSurface(rgba_surface);
  }

  SDL_Rect trim_rect{0, 0, rgba_surface->w, rgba_surface->h};
  if (max_x >= min_x && max_y >= min_y) {
    trim_rect.x = min_x;
    trim_rect.y = min_y;
    trim_rect.w = max_x - min_x + 1;
    trim_rect.h = max_y - min_y + 1;
  }

  SDL_Surface *trimmed_surface = SDL_CreateRGBSurfaceWithFormat(
      0, trim_rect.w, trim_rect.h, 32, SDL_PIXELFORMAT_RGBA32);
  if (trimmed_surface == nullptr) {
    SDL_FreeSurface(rgba_surface);
    std::cerr << "Could not allocate decorative art buffer for " << path
              << ": " << SDL_GetError() << "\n";
    return nullptr;
  }

  SDL_FillRect(trimmed_surface, nullptr,
               SDL_MapRGBA(trimmed_surface->format, 0, 0, 0, 0));
  if (SDL_BlitSurface(rgba_surface, &trim_rect, trimmed_surface, nullptr) != 0) {
    std::cerr << "Could not trim decorative art " << path << ": "
              << SDL_GetError() << "\n";
    SDL_FreeSurface(trimmed_surface);
    SDL_FreeSurface(rgba_surface);
    return nullptr;
  }

  SDL_Texture *texture =
      SDL_CreateTextureFromSurface(sdl_renderer, trimmed_surface);
  if (texture == nullptr) {
    std::cerr << "Could not create decorative texture " << path << ": "
              << SDL_GetError() << "\n";
  } else {
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    trimmed_size = SDL_Point{trim_rect.w, trim_rect.h};
  }

  SDL_FreeSurface(trimmed_surface);
  SDL_FreeSurface(rgba_surface);
  return texture;
}

void Renderer::renderDecorativeTexture(SDL_Texture *texture,
                                       const SDL_Rect &destination,
                                       const SDL_Color &glow_color,
                                       double sway_phase,
                                       double sway_strength_x,
                                       double sway_strength_y,
                                       double sway_speed) {
  if (texture == nullptr || destination.w <= 0 || destination.h <= 0) {
    return;
  }

  SDL_FRect animated_rect{static_cast<float>(destination.x),
                          static_cast<float>(destination.y),
                          static_cast<float>(destination.w),
                          static_cast<float>(destination.h)};
  if (sway_strength_x > 0.0 || sway_strength_y > 0.0) {
    const double clock = static_cast<double>(SDL_GetTicks());
    const double speed = std::max(0.05, sway_speed);
    const double horizontal_phase = clock * speed / 2100.0 + sway_phase;
    const double vertical_phase = clock * speed / 2950.0 + sway_phase * 1.15;
    const double drift_phase = clock * speed / 5200.0 + sway_phase * 0.75;
    const double horizontal_amplitude =
        std::max(6.0, destination.w * std::max(0.0, sway_strength_x));
    const double vertical_amplitude =
        std::max(5.0, destination.h * std::max(0.0, sway_strength_y));
    const double sway_x =
        std::sin(horizontal_phase) * horizontal_amplitude +
        std::sin(drift_phase) * horizontal_amplitude * 0.18;
    const double sway_y =
        std::sin(vertical_phase) * vertical_amplitude +
        std::cos(drift_phase * 0.82) * vertical_amplitude * 0.12;
    animated_rect.x += static_cast<float>(sway_x);
    animated_rect.y += static_cast<float>(sway_y);
  }

  SDL_FRect shadow_rect{
      animated_rect.x + static_cast<float>(std::max(5, destination.w / 34)),
      animated_rect.y + static_cast<float>(std::max(8, destination.h / 28)),
      animated_rect.w, animated_rect.h};
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureColorMod(texture, 8, 8, 18);
  SDL_SetTextureAlphaMod(texture, 116);
  SDL_RenderCopyF(sdl_renderer, texture, nullptr, &shadow_rect);

  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
  for (int pass = 0; pass < 2; ++pass) {
    const int padding = std::max(6, destination.w / (18 - pass * 4));
    SDL_FRect glow_rect{animated_rect.x - static_cast<float>(padding),
                        animated_rect.y - static_cast<float>(padding) / 2.0f,
                        animated_rect.w + static_cast<float>(padding * 2),
                        animated_rect.h + static_cast<float>(padding)};
    SDL_SetTextureColorMod(texture, glow_color.r, glow_color.g, glow_color.b);
    SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(44 - pass * 18));
    SDL_RenderCopyF(sdl_renderer, texture, nullptr, &glow_rect);
  }

  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureColorMod(texture, 255, 255, 255);
  SDL_SetTextureAlphaMod(texture, 255);
  SDL_RenderCopyF(sdl_renderer, texture, nullptr, &animated_rect);
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

int Renderer::getPacmanAnimationFrame(bool walking) const {
  if (!walking) {
    return 0;
  }

  const Uint32 frame_clock = SDL_GetTicks() / kPacmanWalkFrameMs;
  return static_cast<int>(frame_clock % kPacmanFramesPerDirection);
}

SDL_Texture *Renderer::getPacmanTexture(Directions facing_direction,
                                        bool walking) const {
  if (sdl_pacman_textures.size() <
      static_cast<size_t>(kPacmanFramesPerDirection * 4)) {
    return nullptr;
  }

  int direction_index = 0;
  switch (facing_direction) {
  case Directions::Up:
    direction_index = 1;
    break;
  case Directions::Left:
    direction_index = 2;
    break;
  case Directions::Right:
    direction_index = 3;
    break;
  case Directions::Down:
  case Directions::None:
  default:
    direction_index = 0;
    break;
  }

  const int frame_index =
      direction_index * kPacmanFramesPerDirection +
      std::clamp(getPacmanAnimationFrame(walking), 0,
                 kPacmanFramesPerDirection - 1);
  return sdl_pacman_textures[static_cast<size_t>(frame_index)];
}

SDL_Surface *Renderer::createStartLogoSurface(TTF_Font *font,
                                              const std::string &text) {
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
  if (lock_glyph) {
    SDL_LockSurface(glyph_surface);
  }
  if (lock_output) {
    SDL_LockSurface(output_surface);
  }

  const int glyph_height = std::max(1, glyph_surface->h - 1);
  const int band_height = std::max(3, glyph_surface->h / 12);
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

  auto gradient_color = [](float progress) -> SDL_Color {
    if (progress < 0.18f) {
      return LerpColor(kStartLogoHighlightColor, kStartLogoLightColor,
                       progress / 0.18f);
    }
    if (progress < 0.52f) {
      return LerpColor(kStartLogoLightColor, kStartLogoMidColor,
                       (progress - 0.18f) / 0.34f);
    }
    return LerpColor(kStartLogoMidColor, kStartLogoDeepColor,
                     (progress - 0.52f) / 0.48f);
  };

  for (int y = 0; y < glyph_surface->h; ++y) {
    for (int x = 0; x < glyph_surface->w; ++x) {
      const Uint8 alpha = glyph_alpha(x, y);
      if (alpha == 0) {
        continue;
      }

      const float progress = static_cast<float>(y) / glyph_height;
      const SDL_Color base_color = gradient_color(progress);
      float raster_boost = 1.0f;
      switch ((y / band_height) % 4) {
      case 0:
        raster_boost = 1.26f;
        break;
      case 1:
        raster_boost = 1.14f;
        break;
      case 2:
        raster_boost = 1.00f;
        break;
      default:
        raster_boost = 0.90f;
        break;
      }
      const float scanline_boost =
          ((y + x / 5) % 2 == 0) ? 1.06f : 0.95f;
      const float shine_boost =
          (progress < 0.34f && ((x + y / 2) % 9) < 3) ? 1.16f : 1.0f;

      int red =
          static_cast<int>(base_color.r * raster_boost * scanline_boost *
                           shine_boost);
      int green =
          static_cast<int>(base_color.g * raster_boost * scanline_boost *
                           shine_boost);
      int blue =
          static_cast<int>(base_color.b * raster_boost * scanline_boost *
                           shine_boost);

      const Uint8 left = glyph_alpha(x - 1, y);
      const Uint8 right = glyph_alpha(x + 1, y);
      const Uint8 up = glyph_alpha(x, y - 1);
      const Uint8 down = glyph_alpha(x, y + 1);
      const Uint8 diagonal = glyph_alpha(x - 1, y - 1);
      const bool edge = left == 0 || right == 0 || up == 0 || down == 0;
      const bool highlight_edge = left == 0 || up == 0 || diagonal == 0;
      const bool shadow_edge = right == 0 || down == 0;

      if (highlight_edge) {
        red = (red * 2 + kStartLogoHighlightColor.r * 3) / 5 + 8;
        green = (green * 2 + kStartLogoHighlightColor.g * 3) / 5 + 10;
        blue = (blue * 2 + kStartLogoHighlightColor.b * 3) / 5 + 10;
      }
      if (shadow_edge) {
        red = (red * 2 + kStartLogoOutlineColor.r * 3) / 5;
        green = (green * 2 + kStartLogoOutlineColor.g * 3) / 5;
        blue = (blue * 2 + kStartLogoOutlineColor.b * 3) / 5;
      } else if (edge) {
        red = (red * 3 + kStartLogoOutlineColor.r * 2) / 5;
        green = (green * 3 + kStartLogoOutlineColor.g * 2) / 5;
        blue = (blue * 3 + kStartLogoOutlineColor.b * 2) / 5;
      }

      red = std::clamp(red, 0, 255);
      green = std::clamp(green, 0, 255);
      blue = std::clamp(blue, 0, 255);

      writePixel(output_surface, x, y,
                 SDL_MapRGBA(output_surface->format, red, green, blue, alpha));
    }
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

  const Uint32 now = SDL_GetTicks();
  const bool invulnerable = game->pacman->invulnerable_until_ticks > now;
  const bool walking =
      game->pacman->px_delta.x != 0 || game->pacman->px_delta.y != 0;
  const Directions facing_direction =
      (game->pacman->facing_direction == Directions::None)
          ? Directions::Down
          : game->pacman->facing_direction;
  SDL_Texture *pacman_texture = getPacmanTexture(facing_direction, walking);

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

    if (invulnerable) {
      drawPacmanShield(animated_rect.x + animated_rect.w / 2,
                       animated_rect.y + animated_rect.h / 2,
                       std::max(8, animated_rect.w / 2 + 5),
                       static_cast<double>(now) / 180.0);
    }

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

    if (size > 1 && pacman_texture != nullptr) {
      SDL_RenderCopyEx(sdl_renderer, pacman_texture, nullptr, &animated_rect,
                       rotation, nullptr, SDL_FLIP_NONE);
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
  if (invulnerable) {
    drawPacmanShield(sdl_pacman_rect.x + sdl_pacman_rect.w / 2,
                     sdl_pacman_rect.y + sdl_pacman_rect.h / 2,
                     std::max(8, sdl_pacman_rect.w / 2 + 5),
                     static_cast<double>(now) / 180.0);
  }
  if (pacman_texture != nullptr) {
    SDL_RenderCopy(sdl_renderer, pacman_texture, nullptr, &sdl_pacman_rect);
  }
}

void Renderer::drawbonusflask() {
  if (game == nullptr || game->dead) {
    return;
  }

  const auto &potion = game->invulnerability_potion;
  if (!potion.is_visible) {
    return;
  }

  const Uint32 now = SDL_GetTicks();
  double alpha_factor = 1.0;
  if (potion.is_fading) {
    alpha_factor =
        1.0 - std::clamp(static_cast<double>(now - potion.fade_started_ticks) /
                             static_cast<double>(BLUE_POTION_FADE_MS),
                         0.0, 1.0);
  }

  if (alpha_factor <= 0.0) {
    return;
  }

  const PixelCoord potion_px = getPixelCoord(potion.coord, 0, 0);
  const int center_x = potion_px.x + element_size / 2;
  const int center_y = potion_px.y + element_size / 2;
  const double wobble_clock =
      static_cast<double>(now + potion.animation_seed * 53) / 260.0;
  const double pulse = 0.76 + 0.24 * std::sin(wobble_clock);
  const Uint8 glow_alpha =
      static_cast<Uint8>(std::clamp(120.0 * alpha_factor, 0.0, 140.0));
  const Uint8 glass_alpha =
      static_cast<Uint8>(std::clamp(175.0 * alpha_factor, 0.0, 200.0));
  const Uint8 fluid_alpha =
      static_cast<Uint8>(std::clamp(210.0 * alpha_factor, 0.0, 230.0));

  SDL_SetRenderDrawColor(sdl_renderer, 72, 164, 255, glow_alpha / 2);
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y,
                       std::max(6, static_cast<int>(element_size * 0.30 * pulse)));
  SDL_SetRenderDrawColor(sdl_renderer, 124, 214, 255, glow_alpha);
  SDL_RenderDrawCircle(sdl_renderer, center_x, center_y,
                       std::max(8, static_cast<int>(element_size * 0.38 * pulse)));

  const int flask_width = std::max(10, static_cast<int>(element_size * 0.34));
  const int flask_body_height =
      std::max(12, static_cast<int>(element_size * 0.36));
  const int flask_neck_width =
      std::max(5, static_cast<int>(flask_width * 0.42));
  const int flask_neck_height =
      std::max(5, static_cast<int>(element_size * 0.16));
  const int flask_top = center_y - flask_body_height / 2;

  SDL_Rect neck_rect{center_x - flask_neck_width / 2,
                     flask_top - flask_neck_height / 2, flask_neck_width,
                     flask_neck_height};
  SDL_Rect body_rect{center_x - flask_width / 2, flask_top, flask_width,
                     flask_body_height};

  SDL_SetRenderDrawColor(sdl_renderer, 225, 245, 255, glass_alpha / 4);
  SDL_RenderFillRect(sdl_renderer, &neck_rect);
  SDL_RenderFillRect(sdl_renderer, &body_rect);
  SDL_RenderFillCircle(sdl_renderer, center_x, body_rect.y + body_rect.h,
                       std::max(5, flask_width / 2));

  const int liquid_height = std::max(
      5, static_cast<int>(flask_body_height * (0.42 + 0.10 * std::sin(wobble_clock))));
  SDL_Rect liquid_rect{body_rect.x + 2, body_rect.y + body_rect.h - liquid_height,
                       std::max(2, body_rect.w - 4), liquid_height};
  SDL_SetRenderDrawColor(sdl_renderer, 44, 112, 255, fluid_alpha);
  SDL_RenderFillRect(sdl_renderer, &liquid_rect);
  SDL_SetRenderDrawColor(sdl_renderer, 118, 204, 255, fluid_alpha);
  SDL_RenderFillCircle(
      sdl_renderer, center_x,
      body_rect.y + body_rect.h - std::max(2, liquid_height / 3),
      std::max(4, flask_width / 2 - 2));

  for (int bubble = 0; bubble < 3; bubble++) {
    const double bubble_phase = wobble_clock * 1.15 + bubble * 1.7;
    const double bubble_progress = std::fmod(now / 900.0 + bubble * 0.31, 1.0);
    const int bubble_x = center_x +
                         static_cast<int>(std::sin(bubble_phase) * flask_width * 0.16);
    const int bubble_y =
        liquid_rect.y + liquid_rect.h -
        static_cast<int>(bubble_progress * std::max(6, liquid_rect.h - 2));
    const int bubble_radius = std::max(2, element_size / 14 + bubble % 2);
    SDL_SetRenderDrawColor(sdl_renderer, 220, 245, 255, glow_alpha);
    SDL_RenderFillCircle(sdl_renderer, bubble_x, bubble_y, bubble_radius);
  }

  SDL_SetRenderDrawColor(sdl_renderer, 224, 246, 255, glass_alpha);
  SDL_RenderDrawRect(sdl_renderer, &neck_rect);
  SDL_RenderDrawRect(sdl_renderer, &body_rect);
  SDL_RenderDrawLine(sdl_renderer, body_rect.x, body_rect.y + body_rect.h,
                     center_x - flask_width / 4, body_rect.y + body_rect.h + flask_width / 3);
  SDL_RenderDrawLine(sdl_renderer, body_rect.x + body_rect.w,
                     body_rect.y + body_rect.h,
                     center_x + flask_width / 4,
                     body_rect.y + body_rect.h + flask_width / 3);
  SDL_RenderDrawCircle(sdl_renderer, center_x, body_rect.y + body_rect.h,
                       std::max(5, flask_width / 2));
}

void Renderer::drawDynamiteIcon(const SDL_Rect &icon_rect, bool lit_fuse,
                                double animation_clock, Uint8 alpha,
                                double fuse_burn_progress) {
  if (icon_rect.w <= 0 || icon_rect.h <= 0) {
    return;
  }

  SDL_Rect draw_rect{icon_rect.x, icon_rect.y, icon_rect.w, icon_rect.h};
  if (sdl_dynamite_texture != nullptr && sdl_dynamite_size.x > 0 &&
      sdl_dynamite_size.y > 0) {
    const double scale_by_width =
        static_cast<double>(icon_rect.w) / static_cast<double>(sdl_dynamite_size.x);
    const double scale_by_height =
        static_cast<double>(icon_rect.h) / static_cast<double>(sdl_dynamite_size.y);
    const double scale = std::min(scale_by_width, scale_by_height);
    draw_rect.w = std::max(
        1, static_cast<int>(std::lround(sdl_dynamite_size.x * scale)));
    draw_rect.h = std::max(
        1, static_cast<int>(std::lround(sdl_dynamite_size.y * scale)));
    draw_rect.x = icon_rect.x + (icon_rect.w - draw_rect.w) / 2;
    draw_rect.y = icon_rect.y + (icon_rect.h - draw_rect.h) / 2;

    SDL_SetTextureBlendMode(sdl_dynamite_texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(sdl_dynamite_texture, alpha);
    SDL_RenderCopy(sdl_renderer, sdl_dynamite_texture, nullptr, &draw_rect);
    SDL_SetTextureAlphaMod(sdl_dynamite_texture, 255);
  } else {
    const int body_height = std::max(5, icon_rect.h / 3);
    const int body_y = icon_rect.y + icon_rect.h / 2 - body_height / 2;
    const SDL_Rect body_rect{
        icon_rect.x + std::max(1, icon_rect.w / 10), body_y,
        std::max(6, icon_rect.w - std::max(2, icon_rect.w / 5)), body_height};
    const int cap_radius = std::max(2, body_rect.h / 2);
    const int center_y = body_rect.y + body_rect.h / 2;

    SDL_SetRenderDrawColor(sdl_renderer, 122, 18, 12, alpha);
    SDL_RenderFillCircle(sdl_renderer, body_rect.x, center_y, cap_radius);
    SDL_RenderFillCircle(sdl_renderer, body_rect.x + body_rect.w, center_y,
                         cap_radius);
    SDL_SetRenderDrawColor(sdl_renderer, 186, 40, 28, alpha);
    SDL_RenderFillRect(sdl_renderer, &body_rect);
    SDL_SetRenderDrawColor(sdl_renderer, 242, 130, 76, alpha);
    SDL_RenderDrawLine(sdl_renderer, body_rect.x + 1, body_rect.y + 1,
                       body_rect.x + body_rect.w - 1, body_rect.y + 1);
    SDL_SetRenderDrawColor(sdl_renderer, 88, 10, 8, alpha);
    SDL_RenderDrawRect(sdl_renderer, &body_rect);

    const int band_width = std::max(2, body_rect.w / 8);
    for (int band = 0; band < 2; band++) {
      const int band_x = body_rect.x + body_rect.w / 3 + band * body_rect.w / 4;
      SDL_Rect band_rect{band_x, body_rect.y - 1, band_width, body_rect.h + 2};
      SDL_SetRenderDrawColor(sdl_renderer, 236, 220, 174, alpha);
      SDL_RenderFillRect(sdl_renderer, &band_rect);
    }
  }

  if (!lit_fuse) {
    return;
  }

  fuse_burn_progress = std::clamp(fuse_burn_progress, 0.0, 1.0);
  constexpr std::array<SDL_FPoint, 6> kFusePath{{
      {0.94f, 0.15f}, {0.89f, 0.13f}, {0.82f, 0.16f},
      {0.76f, 0.09f}, {0.68f, 0.07f}, {0.60f, 0.24f},
  }};

  auto interpolate_fuse_point = [&](double path_progress) {
    path_progress = std::clamp(path_progress, 0.0, 1.0);
    const double scaled =
        path_progress * static_cast<double>(kFusePath.size() - 1);
    const size_t index = std::min(
        static_cast<size_t>(std::floor(scaled)), kFusePath.size() - 2);
    const double local = scaled - static_cast<double>(index);
    const SDL_FPoint from = kFusePath[index];
    const SDL_FPoint to = kFusePath[index + 1];
    return SDL_FPoint{
        static_cast<float>(from.x + (to.x - from.x) * local),
        static_cast<float>(from.y + (to.y - from.y) * local)};
  };

  const SDL_FPoint burn_norm = interpolate_fuse_point(fuse_burn_progress);
  const SDL_FPoint trail_norm =
      interpolate_fuse_point(std::max(0.0, fuse_burn_progress - 0.10));
  const int burn_x = draw_rect.x +
                     static_cast<int>(std::lround(burn_norm.x * draw_rect.w));
  const int burn_y = draw_rect.y +
                     static_cast<int>(std::lround(burn_norm.y * draw_rect.h));
  const int trail_x = draw_rect.x +
                      static_cast<int>(std::lround(trail_norm.x * draw_rect.w));
  const int trail_y = draw_rect.y +
                      static_cast<int>(std::lround(trail_norm.y * draw_rect.h));

  const double spark_pulse = 0.55 + 0.45 * std::sin(animation_clock * 2.4);
  const int spark_radius =
      std::max(2, static_cast<int>(std::max(draw_rect.w, draw_rect.h) * 0.06));
  const Uint8 spark_alpha = static_cast<Uint8>(
      std::clamp(static_cast<double>(alpha) * (0.70 + 0.30 * spark_pulse), 0.0,
                 255.0));

  SDL_SetRenderDrawColor(sdl_renderer, 255, 156, 48, spark_alpha / 2);
  SDL_RenderFillCircle(sdl_renderer, burn_x, burn_y, spark_radius + 2);
  SDL_SetRenderDrawColor(sdl_renderer, 255, 112, 32, spark_alpha);
  SDL_RenderFillCircle(sdl_renderer, burn_x, burn_y, spark_radius + 1);
  SDL_SetRenderDrawColor(sdl_renderer, 255, 232, 146, spark_alpha);
  SDL_RenderFillCircle(sdl_renderer, burn_x, burn_y, spark_radius);

  SDL_SetRenderDrawColor(sdl_renderer, 255, 148, 40,
                         static_cast<Uint8>(std::clamp(spark_alpha * 0.7, 0.0, 255.0)));
  SDL_RenderDrawLine(sdl_renderer, trail_x, trail_y, burn_x, burn_y);

  for (int spark = 0; spark < 5; spark++) {
    const double angle =
        animation_clock * 1.8 + spark * (2.0 * 3.1415926535 / 5.0);
    const int length =
        std::max(3, static_cast<int>(draw_rect.w * (0.10 + 0.06 * spark_pulse)));
    const int x2 = burn_x + static_cast<int>(std::cos(angle) * length);
    const int y2 = burn_y + static_cast<int>(std::sin(angle) * length);
    SDL_SetRenderDrawColor(sdl_renderer, 255, 236, 158, spark_alpha);
    SDL_RenderDrawLine(sdl_renderer, burn_x, burn_y, x2, y2);
  }
}

void Renderer::drawPlasticExplosiveIcon(const SDL_Rect &icon_rect,
                                        double animation_clock, Uint8 alpha,
                                        bool armed) {
  if (icon_rect.w <= 0 || icon_rect.h <= 0) {
    return;
  }

  const SDL_Color body_color =
      armed ? SDL_Color{204, 56, 44, alpha} : SDL_Color{178, 64, 44, alpha};
  const SDL_Color band_color =
      armed ? SDL_Color{46, 42, 40, alpha} : SDL_Color{56, 52, 48, alpha};
  const SDL_Color cap_color =
      armed ? SDL_Color{255, 238, 194, alpha} : SDL_Color{230, 222, 184, alpha};
  const SDL_Color led_color =
      armed ? SDL_Color{255, 86, 66, alpha} : SDL_Color{84, 220, 255, alpha};
  const double pulse = 0.64 + 0.36 * std::sin(animation_clock * (armed ? 2.6 : 1.7));

  SDL_Rect body_rect{icon_rect.x + std::max(1, icon_rect.w / 8),
                     icon_rect.y + std::max(2, icon_rect.h / 4),
                     std::max(6, icon_rect.w * 3 / 4),
                     std::max(6, icon_rect.h / 2)};
  SDL_SetRenderDrawColor(sdl_renderer, body_color.r, body_color.g, body_color.b,
                         static_cast<Uint8>(alpha));
  SDL_RenderFillRect(sdl_renderer, &body_rect);

  SDL_SetRenderDrawColor(sdl_renderer, cap_color.r, cap_color.g, cap_color.b,
                         alpha);
  SDL_RenderDrawRect(sdl_renderer, &body_rect);

  const int strap_width = std::max(2, body_rect.w / 6);
  SDL_Rect left_strap{body_rect.x + strap_width, body_rect.y, strap_width,
                      body_rect.h};
  SDL_Rect right_strap{body_rect.x + body_rect.w - strap_width * 2, body_rect.y,
                       strap_width, body_rect.h};
  SDL_SetRenderDrawColor(sdl_renderer, band_color.r, band_color.g, band_color.b,
                         static_cast<Uint8>(alpha));
  SDL_RenderFillRect(sdl_renderer, &left_strap);
  SDL_RenderFillRect(sdl_renderer, &right_strap);

  const int wire_start_x = body_rect.x + body_rect.w - 1;
  const int wire_start_y = body_rect.y + std::max(2, body_rect.h / 4);
  const int wire_mid_x = icon_rect.x + icon_rect.w - std::max(2, icon_rect.w / 7);
  const int wire_mid_y = icon_rect.y + std::max(2, icon_rect.h / 5);
  SDL_SetRenderDrawColor(sdl_renderer, 226, 216, 184, alpha);
  SDL_RenderDrawLine(sdl_renderer, wire_start_x, wire_start_y, wire_mid_x,
                     wire_mid_y);
  SDL_RenderDrawLine(sdl_renderer, wire_mid_x, wire_mid_y,
                     wire_mid_x + std::max(1, icon_rect.w / 10),
                     wire_mid_y - std::max(2, icon_rect.h / 7));

  const int led_radius = std::max(2, icon_rect.w / 8);
  const int led_center_x = body_rect.x + body_rect.w / 2;
  const int led_center_y = body_rect.y + body_rect.h / 2;
  SDL_SetRenderDrawColor(
      sdl_renderer, led_color.r, led_color.g, led_color.b,
      static_cast<Uint8>(std::clamp(static_cast<double>(alpha) *
                                        (armed ? (0.72 + pulse * 0.28) : 0.84),
                                    0.0, 255.0)));
  SDL_RenderFillCircle(sdl_renderer, led_center_x, led_center_y, led_radius);
}

void Renderer::drawdynamitepickup() {
  if (game == nullptr || game->dead) {
    return;
  }

  const auto &dynamite = game->dynamite_pickup;
  if (!dynamite.is_visible) {
    return;
  }

  const Uint32 now = SDL_GetTicks();
  double alpha_factor = 1.0;
  if (dynamite.is_fading) {
    alpha_factor =
        1.0 - std::clamp(static_cast<double>(now - dynamite.fade_started_ticks) /
                             static_cast<double>(DYNAMITE_FADE_MS),
                         0.0, 1.0);
  }
  if (alpha_factor <= 0.0) {
    return;
  }

  const PixelCoord pickup_px = getPixelCoord(dynamite.coord, 0, 0);
  const int center_x = pickup_px.x + element_size / 2;
  const int center_y = pickup_px.y + element_size / 2;
  const double animation_clock =
      static_cast<double>(now + dynamite.animation_seed * 41) / 180.0;
  const double pulse = 0.80 + 0.20 * std::sin(animation_clock);
  const Uint8 glow_alpha = static_cast<Uint8>(
      std::clamp(96.0 * alpha_factor, 0.0, 140.0));
  const Uint8 body_alpha = static_cast<Uint8>(
      std::clamp(255.0 * alpha_factor, 0.0, 255.0));

  SDL_SetRenderDrawColor(sdl_renderer, 255, 118, 24, glow_alpha / 2);
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y,
                       std::max(7, static_cast<int>(element_size * 0.28 * pulse)));
  SDL_SetRenderDrawColor(sdl_renderer, 255, 202, 110, glow_alpha);
  SDL_RenderDrawCircle(sdl_renderer, center_x, center_y,
                       std::max(9, static_cast<int>(element_size * 0.38 * pulse)));

  const int icon_size = std::max(12, static_cast<int>(element_size * 0.74));
  const SDL_Rect icon_rect{center_x - icon_size / 2, center_y - icon_size / 2,
                           icon_size, icon_size};
  drawDynamiteIcon(icon_rect, false, animation_clock, body_alpha, 0.0);
}

void Renderer::drawplasticexplosivepickup() {
  if (game == nullptr || game->dead) {
    return;
  }

  const auto &plastic_explosive = game->plastic_explosive_pickup;
  if (!plastic_explosive.is_visible) {
    return;
  }

  const Uint32 now = SDL_GetTicks();
  double alpha_factor = 1.0;
  if (plastic_explosive.is_fading) {
    alpha_factor =
        1.0 - std::clamp(static_cast<double>(now - plastic_explosive.fade_started_ticks) /
                             static_cast<double>(PLASTIC_EXPLOSIVE_FADE_MS),
                         0.0, 1.0);
  }
  if (alpha_factor <= 0.0) {
    return;
  }

  const PixelCoord pickup_px = getPixelCoord(plastic_explosive.coord, 0, 0);
  const int center_x = pickup_px.x + element_size / 2;
  const int center_y = pickup_px.y + element_size / 2;
  const double animation_clock =
      static_cast<double>(now + plastic_explosive.animation_seed * 37) / 170.0;
  const double pulse = 0.78 + 0.22 * std::sin(animation_clock);
  const Uint8 glow_alpha = static_cast<Uint8>(
      std::clamp(88.0 * alpha_factor, 0.0, 136.0));
  const Uint8 body_alpha = static_cast<Uint8>(
      std::clamp(255.0 * alpha_factor, 0.0, 255.0));

  SDL_SetRenderDrawColor(sdl_renderer, 84, 214, 255, glow_alpha / 2);
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y,
                       std::max(7, static_cast<int>(element_size * 0.26 * pulse)));
  SDL_SetRenderDrawColor(sdl_renderer, 190, 244, 255, glow_alpha);
  SDL_RenderDrawCircle(sdl_renderer, center_x, center_y,
                       std::max(9, static_cast<int>(element_size * 0.36 * pulse)));

  const int icon_size = std::max(12, static_cast<int>(element_size * 0.74));
  const SDL_Rect icon_rect{center_x - icon_size / 2, center_y - icon_size / 2,
                           icon_size, icon_size};
  drawPlasticExplosiveIcon(icon_rect, animation_clock, body_alpha, false);
}

void Renderer::drawplaceddynamites() {
  if (game == nullptr) {
    return;
  }

  const Uint32 now = SDL_GetTicks();
  for (const auto &placed_dynamite : game->placed_dynamites) {
    const PixelCoord placed_px = getPixelCoord(placed_dynamite.coord, 0, 0);
    const int center_x = placed_px.x + element_size / 2;
    const int center_y = placed_px.y + element_size / 2;
    const Uint32 remaining_ms =
        (placed_dynamite.explode_at_ticks > now)
            ? (placed_dynamite.explode_at_ticks - now)
            : 0;
    const double countdown_progress =
        std::clamp(static_cast<double>(remaining_ms) /
                       static_cast<double>(std::max(1, DYNAMITE_COUNTDOWN_MS)),
                   0.0, 1.0);
    const double urgency = 1.0 - countdown_progress;
    const double pulse_clock =
        static_cast<double>(now + placed_dynamite.animation_seed * 29) / 130.0;
    const double pulse =
        0.88 + (0.12 + urgency * 0.22) * std::sin(pulse_clock);

    SDL_SetRenderDrawColor(
        sdl_renderer, 255, 88, 32,
        static_cast<Uint8>(std::clamp(72.0 + urgency * 90.0, 0.0, 190.0)));
    SDL_RenderFillCircle(sdl_renderer, center_x, center_y + element_size / 6,
                         std::max(6, static_cast<int>(element_size * 0.24 * pulse)));

    const int icon_size = std::max(12, static_cast<int>(element_size * 0.78));
    const SDL_Rect icon_rect{center_x - icon_size / 2,
                             center_y - icon_size / 2 - element_size / 10,
                             icon_size, icon_size};
    drawDynamiteIcon(icon_rect, true, pulse_clock, 255, urgency);

    const int remaining_seconds =
        static_cast<int>((std::max<Uint32>(1, remaining_ms) + 999) / 1000);
    renderSimpleText(sdl_font_hud, std::to_string(remaining_seconds),
                     SDL_Color{255, 238, 206, 255}, center_x,
                     placed_px.y - TTF_FontHeight(sdl_font_hud) / 3);
  }
}

void Renderer::drawplacedplasticexplosive() {
  if (game == nullptr || !game->plastic_explosive_is_armed) {
    return;
  }

  const Uint32 now = SDL_GetTicks();
  const auto &placed_plastic_explosive = game->placed_plastic_explosive;
  const PixelCoord placed_px = getPixelCoord(placed_plastic_explosive.coord, 0, 0);
  const int center_x = placed_px.x + element_size / 2;
  const int center_y = placed_px.y + element_size / 2;
  const double pulse_clock =
      static_cast<double>(now + placed_plastic_explosive.animation_seed * 31) /
      130.0;
  const double pulse = 0.84 + 0.28 * std::sin(pulse_clock);

  SDL_SetRenderDrawColor(
      sdl_renderer, 255, 72, 52,
      static_cast<Uint8>(std::clamp(92.0 + pulse * 72.0, 0.0, 188.0)));
  SDL_RenderFillCircle(
      sdl_renderer, center_x, center_y + element_size / 8,
      std::max(6, static_cast<int>(element_size * 0.22 * pulse)));

  const int icon_size = std::max(12, static_cast<int>(element_size * 0.76));
  const SDL_Rect icon_rect{center_x - icon_size / 2,
                           center_y - icon_size / 2 - element_size / 12,
                           icon_size, icon_size};
  drawPlasticExplosiveIcon(icon_rect, pulse_clock, 255, true);
}

void Renderer::drawPacmanShield(int center_x, int center_y, int base_radius,
                                double pulse_clock) {
  const double pulse = 0.72 + 0.28 * std::sin(pulse_clock);
  const int outer_radius =
      std::max(base_radius, static_cast<int>(base_radius * (1.08 + 0.16 * pulse)));
  const int inner_radius =
      std::max(base_radius - 3, static_cast<int>(base_radius * (0.82 + 0.08 * pulse)));

  SDL_SetRenderDrawColor(sdl_renderer, 32, 126, 255, 58);
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y, outer_radius);
  SDL_SetRenderDrawColor(sdl_renderer, 86, 196, 255, 78);
  SDL_RenderFillCircle(sdl_renderer, center_x, center_y, inner_radius);
  SDL_SetRenderDrawColor(sdl_renderer, 200, 245, 255, 165);
  SDL_RenderDrawCircle(sdl_renderer, center_x, center_y, outer_radius);

  for (int orb = 0; orb < 4; orb++) {
    const double angle = pulse_clock * 0.85 + orb * (2.0 * 3.1415926535 / 4.0);
    const int orb_radius =
        std::max(2, static_cast<int>(element_size * (0.05 + 0.02 * pulse)));
    const int orbit_radius = std::max(outer_radius - 2, inner_radius);
    const int orb_x = center_x + static_cast<int>(std::cos(angle) * orbit_radius);
    const int orb_y = center_y + static_cast<int>(std::sin(angle) * orbit_radius);
    SDL_SetRenderDrawColor(sdl_renderer, 214, 250, 255, 180);
    SDL_RenderFillCircle(sdl_renderer, orb_x, orb_y, orb_radius);
  }
}

void Renderer::drawDwarf(const SDL_Rect &dwarf_rect,
                         Directions facing_direction, double gait_phase,
                         bool walking) {
  const double pi = 3.1415926535;
  const Directions resolved_facing =
      (facing_direction == Directions::None) ? Directions::Down
                                             : facing_direction;
  const bool facing_front = resolved_facing == Directions::Down;
  const bool facing_back = resolved_facing == Directions::Up;
  const int mirror = resolved_facing == Directions::Left ? -1 : 1;
  const int center_x = dwarf_rect.x + dwarf_rect.w / 2;
  const int center_y = dwarf_rect.y + dwarf_rect.h / 2;
  const double stride =
      walking ? std::sin(gait_phase) * dwarf_rect.w * 0.08 : 0.0;
  const double counter_stride =
      walking ? std::sin(gait_phase + pi) * dwarf_rect.w * 0.08 : 0.0;
  const double bob =
      walking ? std::sin(gait_phase * 2.0) * dwarf_rect.h * 0.018 : 0.0;
  const double arm_swing =
      walking ? std::sin(gait_phase + pi * 0.5) * dwarf_rect.h * 0.05 : 0.0;
  const double braid_sway =
      walking ? std::sin(gait_phase * 0.75) * dwarf_rect.w * 0.035 : 0.0;

  const SDL_Color outline{34, 24, 18, 255};
  const SDL_Color outline_soft{58, 45, 34, 255};
  const SDL_Color rim_light{246, 232, 196, 110};
  const SDL_Color gold_dark{104, 72, 24, 255};
  const SDL_Color gold{170, 126, 50, 255};
  const SDL_Color gold_light{224, 190, 104, 255};
  const SDL_Color steel_dark{76, 68, 64, 255};
  const SDL_Color steel{126, 114, 102, 255};
  const SDL_Color steel_light{210, 198, 180, 255};
  const SDL_Color leather_dark{68, 42, 20, 255};
  const SDL_Color leather{106, 66, 30, 255};
  const SDL_Color beard_dark{108, 52, 16, 255};
  const SDL_Color beard{164, 82, 28, 255};
  const SDL_Color beard_light{220, 140, 68, 255};
  const SDL_Color skin{214, 170, 120, 255};
  const SDL_Color eye{236, 238, 244, 255};
  const SDL_Color eye_glow{255, 216, 158, 255};
  const SDL_Color shadow{0, 0, 0, 255};

  auto setColor = [&](const SDL_Color &color, Uint8 alpha_override = 255) {
    SDL_SetRenderDrawColor(sdl_renderer, color.r, color.g, color.b,
                           alpha_override);
  };
  auto point = [&](double x, double y) {
    return SDL_Point{center_x + static_cast<int>(std::lround(x)),
                     center_y + static_cast<int>(std::lround(y + bob))};
  };
  auto sidePoint = [&](double x, double y) {
    return SDL_Point{center_x + static_cast<int>(std::lround(x * mirror)),
                     center_y + static_cast<int>(std::lround(y + bob))};
  };
  auto fillPoint = [&](const SDL_Point &point_value, int radius,
                       const SDL_Color &color,
                       Uint8 alpha_override = 255) {
    setColor(color, alpha_override);
    SDL_RenderFillCircle(sdl_renderer, point_value.x, point_value.y, radius);
  };
  auto outlinePoint = [&](const SDL_Point &point_value, int radius,
                          const SDL_Color &color,
                          Uint8 alpha_override = 255) {
    setColor(color, alpha_override);
    SDL_RenderDrawCircle(sdl_renderer, point_value.x, point_value.y, radius);
  };
  auto drawThickLine = [&](const SDL_Point &from, const SDL_Point &to,
                           int thickness, const SDL_Color &color,
                           Uint8 alpha_override = 255) {
    setColor(color, alpha_override);
    const int radius = std::max(0, thickness / 2);
    for (int dx = -radius; dx <= radius; dx++) {
      for (int dy = -radius; dy <= radius; dy++) {
        if (dx * dx + dy * dy > radius * radius + 1) {
          continue;
        }
        SDL_RenderDrawLine(sdl_renderer, from.x + dx, from.y + dy, to.x + dx,
                           to.y + dy);
      }
    }
  };
  auto fillTriangle = [&](const SDL_Point &base_one, const SDL_Point &base_two,
                          const SDL_Point &tip, const SDL_Color &color,
                          Uint8 alpha_override = 255) {
    setColor(color, alpha_override);
    const int steps =
        std::max(10, std::max(std::abs(tip.y - base_one.y),
                              std::abs(tip.y - base_two.y)));
    for (int step = 0; step <= steps; step++) {
      const double t = static_cast<double>(step) / steps;
      const int x1 = base_one.x +
                     static_cast<int>(std::lround((tip.x - base_one.x) * t));
      const int y1 = base_one.y +
                     static_cast<int>(std::lround((tip.y - base_one.y) * t));
      const int x2 = base_two.x +
                     static_cast<int>(std::lround((tip.x - base_two.x) * t));
      const int y2 = base_two.y +
                     static_cast<int>(std::lround((tip.y - base_two.y) * t));
      SDL_RenderDrawLine(sdl_renderer, x1, y1, x2, y2);
    }
  };
  auto drawInsetRect = [&](const SDL_Rect &rect, const SDL_Color &fill_color,
                           const SDL_Color &border_color,
                           Uint8 fill_alpha = 255,
                           Uint8 border_alpha = 255) {
    if (rect.w <= 0 || rect.h <= 0) {
      return;
    }
    setColor(border_color, border_alpha);
    SDL_RenderFillRect(sdl_renderer, &rect);
    if (rect.w > 2 && rect.h > 2) {
      SDL_Rect inner{rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2};
      setColor(fill_color, fill_alpha);
      SDL_RenderFillRect(sdl_renderer, &inner);
    }
  };
  auto drawBoot = [&](const SDL_Point &ankle, int toe_direction, bool large,
                      bool shaded) {
    const int boot_width = large ? std::max(8, dwarf_rect.w / 9)
                                 : std::max(7, dwarf_rect.w / 10);
    const int boot_height = large ? std::max(6, dwarf_rect.h / 12)
                                  : std::max(5, dwarf_rect.h / 13);
    SDL_Rect boot_rect{ankle.x - boot_width / 2, ankle.y - boot_height / 2,
                       boot_width, boot_height};
    drawInsetRect(boot_rect, shaded ? leather_dark : leather, outline);
    fillPoint({ankle.x + toe_direction * std::max(2, boot_width / 3),
               ankle.y + boot_height / 6},
              std::max(2, boot_height / 2), shaded ? leather_dark : leather);
    outlinePoint({ankle.x + toe_direction * std::max(2, boot_width / 3),
                  ankle.y + boot_height / 6},
                 std::max(2, boot_height / 2), outline_soft);
    setColor(gold_light, 145);
    SDL_RenderDrawLine(sdl_renderer, boot_rect.x + 1, boot_rect.y + 1,
                       boot_rect.x + boot_rect.w - 2, boot_rect.y + 1);
  };
  auto drawGauntlet = [&](const SDL_Point &hand, bool highlighted) {
    fillPoint(hand, std::max(3, dwarf_rect.w / 16), outline);
    fillPoint(hand, std::max(2, dwarf_rect.w / 18),
              highlighted ? gold_light : gold);
    outlinePoint(hand, std::max(3, dwarf_rect.w / 16), outline_soft);
  };

  fillPoint({center_x, dwarf_rect.y + dwarf_rect.h - std::max(4, dwarf_rect.h / 12)},
            std::max(6, dwarf_rect.w / 4), shadow, 74);

  const int head_radius = std::max(6, dwarf_rect.w / 7);
  const int shoulder_radius = std::max(6, dwarf_rect.w / 7);
  const int limb_width = std::max(4, dwarf_rect.w / 12);
  const int body_width = std::max(18, static_cast<int>(dwarf_rect.w * 0.35));
  const int body_height = std::max(18, static_cast<int>(dwarf_rect.h * 0.30));

  if (facing_front) {
    const SDL_Point head = point(0, -dwarf_rect.h * 0.22);
    const SDL_Point left_shoulder = point(-dwarf_rect.w * 0.18, -dwarf_rect.h * 0.05);
    const SDL_Point right_shoulder = point(dwarf_rect.w * 0.18, -dwarf_rect.h * 0.05);
    const SDL_Point left_hip = point(-dwarf_rect.w * 0.08, dwarf_rect.h * 0.12);
    const SDL_Point right_hip = point(dwarf_rect.w * 0.08, dwarf_rect.h * 0.12);
    const SDL_Point left_knee = point(-dwarf_rect.w * 0.10 + stride * 0.35,
                                      dwarf_rect.h * 0.23);
    const SDL_Point right_knee = point(dwarf_rect.w * 0.10 + counter_stride * 0.35,
                                       dwarf_rect.h * 0.23);
    const SDL_Point left_foot = point(-dwarf_rect.w * 0.11 + stride * 0.55,
                                      dwarf_rect.h * 0.35);
    const SDL_Point right_foot = point(dwarf_rect.w * 0.11 + counter_stride * 0.55,
                                       dwarf_rect.h * 0.35);
    const SDL_Point left_elbow = point(-dwarf_rect.w * 0.23,
                                       dwarf_rect.h * (0.07 + arm_swing / dwarf_rect.h));
    const SDL_Point right_elbow = point(dwarf_rect.w * 0.23,
                                        dwarf_rect.h * (0.07 - arm_swing / dwarf_rect.h));
    const SDL_Point left_hand = point(-dwarf_rect.w * 0.17,
                                      dwarf_rect.h * (0.21 + arm_swing / dwarf_rect.h * 0.6));
    const SDL_Point right_hand =
        point(dwarf_rect.w * 0.17,
              dwarf_rect.h * (0.21 - arm_swing / dwarf_rect.h * 0.6));

    fillPoint(head, head_radius + 4, rim_light, 72);
    fillPoint(point(0, 0), std::max(10, body_width / 2 + 5), rim_light, 58);

    drawThickLine(left_hip, left_knee, limb_width + 2, outline);
    drawThickLine(left_knee, left_foot, limb_width + 2, outline);
    drawThickLine(right_hip, right_knee, limb_width + 2, outline);
    drawThickLine(right_knee, right_foot, limb_width + 2, outline);
    drawThickLine(left_hip, left_knee, limb_width, steel_dark);
    drawThickLine(left_knee, left_foot, limb_width, leather);
    drawThickLine(right_hip, right_knee, limb_width, steel_dark);
    drawThickLine(right_knee, right_foot, limb_width, leather);
    drawBoot(left_foot, -1, true, false);
    drawBoot(right_foot, 1, true, false);

    drawThickLine(left_shoulder, left_elbow, limb_width + 2, outline);
    drawThickLine(left_elbow, left_hand, limb_width + 2, outline);
    drawThickLine(right_shoulder, right_elbow, limb_width + 2, outline);
    drawThickLine(right_elbow, right_hand, limb_width + 2, outline);
    drawThickLine(left_shoulder, left_elbow, limb_width, leather_dark);
    drawThickLine(left_elbow, left_hand, limb_width, gold_dark);
    drawThickLine(right_shoulder, right_elbow, limb_width, leather_dark);
    drawThickLine(right_elbow, right_hand, limb_width, gold_dark);
    drawGauntlet(left_hand, false);
    drawGauntlet(right_hand, true);

    fillPoint(left_shoulder, shoulder_radius + 2, outline);
    fillPoint(right_shoulder, shoulder_radius + 2, outline);
    fillPoint(left_shoulder, shoulder_radius, gold_dark);
    fillPoint(right_shoulder, shoulder_radius, gold_dark);
    fillPoint(left_shoulder, std::max(4, shoulder_radius - 3), gold);
    fillPoint(right_shoulder, std::max(4, shoulder_radius - 3), gold);
    outlinePoint(left_shoulder, shoulder_radius, gold_light, 210);
    outlinePoint(right_shoulder, shoulder_radius, gold_light, 210);

    SDL_Rect torso{center_x - body_width / 2,
                   center_y - body_height / 3 + static_cast<int>(std::lround(bob)),
                   body_width, body_height};
    drawInsetRect(torso, steel, outline);
    SDL_Rect torso_highlight{torso.x + 3, torso.y + 2, torso.w - 6,
                             std::max(3, torso.h / 3)};
    drawInsetRect(torso_highlight, steel_light, gold_dark, 230, 170);
    SDL_Rect belt{center_x - body_width / 2 + 2,
                  center_y + static_cast<int>(dwarf_rect.h * 0.12 + bob),
                  body_width - 4, std::max(6, dwarf_rect.h / 13)};
    drawInsetRect(belt, leather_dark, outline);
    SDL_Rect buckle{center_x - std::max(4, dwarf_rect.w / 12),
                    belt.y - 1, std::max(8, dwarf_rect.w / 6),
                    belt.h + 2};
    drawInsetRect(buckle, gold_light, outline);
    SDL_Rect buckle_inner{buckle.x + 2, buckle.y + 2, std::max(2, buckle.w - 4),
                          std::max(2, buckle.h - 4)};
    drawInsetRect(buckle_inner, gold_dark, leather_dark);

    const SDL_Point beard_left =
        point(-dwarf_rect.w * 0.12, -dwarf_rect.h * 0.06);
    const SDL_Point beard_right =
        point(dwarf_rect.w * 0.12, -dwarf_rect.h * 0.06);
    const SDL_Point beard_tip =
        point(braid_sway, dwarf_rect.h * 0.25);
    fillTriangle(beard_left, beard_right, beard_tip, outline);
    fillTriangle(point(-dwarf_rect.w * 0.11, -dwarf_rect.h * 0.05),
                 point(dwarf_rect.w * 0.11, -dwarf_rect.h * 0.05),
                 point(braid_sway, dwarf_rect.h * 0.24), beard_dark);
    fillTriangle(point(-dwarf_rect.w * 0.08, -dwarf_rect.h * 0.04),
                 point(dwarf_rect.w * 0.08, -dwarf_rect.h * 0.04),
                 point(braid_sway * 0.8, dwarf_rect.h * 0.22), beard);
    drawThickLine(point(-dwarf_rect.w * 0.02, -dwarf_rect.h * 0.03),
                  point(braid_sway * 0.6, dwarf_rect.h * 0.21),
                  std::max(2, dwarf_rect.w / 18), beard_light, 210);
    drawThickLine(point(dwarf_rect.w * 0.03, -dwarf_rect.h * 0.02),
                  point(dwarf_rect.w * 0.07 + braid_sway * 0.2,
                        dwarf_rect.h * 0.17),
                  std::max(2, dwarf_rect.w / 20), beard_light, 180);
    drawThickLine(point(-dwarf_rect.w * 0.07, -dwarf_rect.h * 0.02),
                  point(-dwarf_rect.w * 0.08 + braid_sway * 0.2,
                        dwarf_rect.h * 0.17),
                  std::max(2, dwarf_rect.w / 20), beard_light, 180);

    fillPoint(head, head_radius + 2, outline);
    fillPoint(point(0, -dwarf_rect.h * 0.14), std::max(4, head_radius / 3), skin);
    fillPoint(head, head_radius, steel_dark);
    fillPoint(point(0, -dwarf_rect.h * 0.25), std::max(5, head_radius - 2),
              gold_dark);
    fillPoint(point(0, -dwarf_rect.h * 0.26), std::max(4, head_radius - 4),
              gold);
    SDL_Rect brow_band{center_x - head_radius - 2,
                       head.y - std::max(2, head_radius / 6),
                       head_radius * 2 + 4, std::max(7, head_radius / 2)};
    drawInsetRect(brow_band, gold, outline);
    SDL_Rect nose_guard{center_x - std::max(2, dwarf_rect.w / 30), head.y - 1,
                        std::max(4, dwarf_rect.w / 15),
                        std::max(12, dwarf_rect.h / 8)};
    drawInsetRect(nose_guard, gold, outline);
    SDL_Rect left_cheek{head.x - head_radius - 2, head.y - 1,
                        std::max(5, dwarf_rect.w / 11),
                        std::max(14, dwarf_rect.h / 7)};
    SDL_Rect right_cheek{head.x + head_radius - std::max(3, dwarf_rect.w / 18),
                         head.y - 1, std::max(5, dwarf_rect.w / 11),
                         std::max(14, dwarf_rect.h / 7)};
    drawInsetRect(left_cheek, steel, outline);
    drawInsetRect(right_cheek, steel, outline);
    fillPoint(point(-dwarf_rect.w * 0.05, -dwarf_rect.h * 0.17),
              std::max(2, dwarf_rect.w / 25), eye);
    fillPoint(point(dwarf_rect.w * 0.05, -dwarf_rect.h * 0.17),
              std::max(2, dwarf_rect.w / 25), eye);
    fillPoint(point(-dwarf_rect.w * 0.05, -dwarf_rect.h * 0.17), 1, eye_glow);
    fillPoint(point(dwarf_rect.w * 0.05, -dwarf_rect.h * 0.17), 1, eye_glow);

    outlinePoint(head, head_radius + 1, rim_light, 130);
    outlinePoint(point(0, 0), std::max(8, body_width / 2 + 1), rim_light, 86);
  } else if (facing_back) {
    const SDL_Point head = point(0, -dwarf_rect.h * 0.21);
    const SDL_Point left_shoulder = point(-dwarf_rect.w * 0.17, -dwarf_rect.h * 0.05);
    const SDL_Point right_shoulder = point(dwarf_rect.w * 0.17, -dwarf_rect.h * 0.05);
    const SDL_Point left_hip = point(-dwarf_rect.w * 0.08, dwarf_rect.h * 0.11);
    const SDL_Point right_hip = point(dwarf_rect.w * 0.08, dwarf_rect.h * 0.11);
    const SDL_Point left_knee = point(-dwarf_rect.w * 0.10 + stride * 0.32,
                                      dwarf_rect.h * 0.23);
    const SDL_Point right_knee = point(dwarf_rect.w * 0.10 + counter_stride * 0.32,
                                       dwarf_rect.h * 0.23);
    const SDL_Point left_foot = point(-dwarf_rect.w * 0.11 + stride * 0.48,
                                      dwarf_rect.h * 0.35);
    const SDL_Point right_foot = point(dwarf_rect.w * 0.11 + counter_stride * 0.48,
                                       dwarf_rect.h * 0.35);
    const SDL_Point left_hand =
        point(-dwarf_rect.w * 0.17, dwarf_rect.h * (0.18 + arm_swing / dwarf_rect.h));
    const SDL_Point right_hand =
        point(dwarf_rect.w * 0.17,
              dwarf_rect.h * (0.18 - arm_swing / dwarf_rect.h));

    fillPoint(head, head_radius + 4, rim_light, 68);
    fillPoint(point(0, 0), std::max(10, body_width / 2 + 4), rim_light, 52);

    drawThickLine(left_hip, left_knee, limb_width + 2, outline);
    drawThickLine(left_knee, left_foot, limb_width + 2, outline);
    drawThickLine(right_hip, right_knee, limb_width + 2, outline);
    drawThickLine(right_knee, right_foot, limb_width + 2, outline);
    drawThickLine(left_hip, left_knee, limb_width, steel_dark);
    drawThickLine(left_knee, left_foot, limb_width, leather);
    drawThickLine(right_hip, right_knee, limb_width, steel_dark);
    drawThickLine(right_knee, right_foot, limb_width, leather);
    drawBoot(left_foot, -1, false, true);
    drawBoot(right_foot, 1, false, true);

    drawThickLine(left_shoulder, left_hand, limb_width + 2, outline);
    drawThickLine(right_shoulder, right_hand, limb_width + 2, outline);
    drawThickLine(left_shoulder, left_hand, limb_width, leather_dark);
    drawThickLine(right_shoulder, right_hand, limb_width, leather_dark);
    drawGauntlet(left_hand, false);
    drawGauntlet(right_hand, false);

    fillPoint(left_shoulder, shoulder_radius + 2, outline);
    fillPoint(right_shoulder, shoulder_radius + 2, outline);
    fillPoint(left_shoulder, shoulder_radius, gold_dark);
    fillPoint(right_shoulder, shoulder_radius, gold_dark);
    fillPoint(left_shoulder, std::max(4, shoulder_radius - 4), gold);
    fillPoint(right_shoulder, std::max(4, shoulder_radius - 4), gold);

    SDL_Rect backplate{center_x - body_width / 2,
                       center_y - body_height / 3 + static_cast<int>(std::lround(bob)),
                       body_width, body_height};
    drawInsetRect(backplate, steel_dark, outline);
    SDL_Rect cloak_band{backplate.x + 2, backplate.y + 2, backplate.w - 4,
                        std::max(6, backplate.h / 4)};
    drawInsetRect(cloak_band, gold, outline);
    SDL_Rect belt{center_x - body_width / 2 + 2,
                  center_y + static_cast<int>(dwarf_rect.h * 0.12 + bob),
                  body_width - 4, std::max(6, dwarf_rect.h / 13)};
    drawInsetRect(belt, leather_dark, outline);
    SDL_Rect pouch{center_x - std::max(5, dwarf_rect.w / 12),
                   belt.y + std::max(1, belt.h / 4), std::max(10, dwarf_rect.w / 7),
                   std::max(8, dwarf_rect.h / 10)};
    drawInsetRect(pouch, leather, outline);

    fillPoint(head, head_radius + 2, outline);
    fillPoint(head, head_radius, steel_dark);
    fillPoint(point(0, -dwarf_rect.h * 0.25), std::max(5, head_radius - 2),
              gold_dark);
    fillPoint(point(0, -dwarf_rect.h * 0.26), std::max(4, head_radius - 4),
              gold);
    SDL_Rect back_rim{center_x - head_radius - 1, head.y + head_radius / 4,
                      head_radius * 2 + 2, std::max(6, dwarf_rect.h / 14)};
    drawInsetRect(back_rim, steel, outline);
    drawThickLine(point(0, -dwarf_rect.h * 0.12),
                  point(braid_sway * 0.35, dwarf_rect.h * 0.20),
                  std::max(6, dwarf_rect.w / 10), outline);
    drawThickLine(point(0, -dwarf_rect.h * 0.12),
                  point(braid_sway * 0.30, dwarf_rect.h * 0.20),
                  std::max(5, dwarf_rect.w / 11), beard_dark);
    drawThickLine(point(0, -dwarf_rect.h * 0.11),
                  point(braid_sway * 0.28, dwarf_rect.h * 0.18),
                  std::max(3, dwarf_rect.w / 14), beard);
    drawThickLine(point(0, -dwarf_rect.h * 0.10),
                  point(braid_sway * 0.24, dwarf_rect.h * 0.14),
                  std::max(2, dwarf_rect.w / 18), beard_light, 200);
    outlinePoint(head, head_radius + 1, rim_light, 118);
  } else {
    const SDL_Point head = sidePoint(dwarf_rect.w * 0.01, -dwarf_rect.h * 0.21);
    const SDL_Point front_shoulder =
        sidePoint(dwarf_rect.w * 0.04, -dwarf_rect.h * 0.06);
    const SDL_Point back_shoulder =
        sidePoint(-dwarf_rect.w * 0.08, -dwarf_rect.h * 0.04);
    const SDL_Point front_hip = sidePoint(dwarf_rect.w * 0.03, dwarf_rect.h * 0.11);
    const SDL_Point back_hip = sidePoint(-dwarf_rect.w * 0.06, dwarf_rect.h * 0.11);
    const SDL_Point front_knee =
        sidePoint(dwarf_rect.w * 0.05 + stride * 0.35, dwarf_rect.h * 0.23);
    const SDL_Point back_knee =
        sidePoint(-dwarf_rect.w * 0.03 + counter_stride * 0.28,
                  dwarf_rect.h * 0.23);
    const SDL_Point front_foot =
        sidePoint(dwarf_rect.w * 0.10 + stride * 0.5, dwarf_rect.h * 0.35);
    const SDL_Point back_foot =
        sidePoint(-dwarf_rect.w * 0.02 + counter_stride * 0.38,
                  dwarf_rect.h * 0.34);
    const SDL_Point front_elbow =
        sidePoint(dwarf_rect.w * 0.15, dwarf_rect.h * (0.05 - arm_swing / dwarf_rect.h));
    const SDL_Point front_hand =
        sidePoint(dwarf_rect.w * 0.12, dwarf_rect.h * (0.18 - arm_swing / dwarf_rect.h * 0.7));
    const SDL_Point back_elbow =
        sidePoint(-dwarf_rect.w * 0.12, dwarf_rect.h * (0.07 + arm_swing / dwarf_rect.h * 0.6));
    const SDL_Point back_hand =
        sidePoint(-dwarf_rect.w * 0.08, dwarf_rect.h * (0.18 + arm_swing / dwarf_rect.h * 0.45));
    const SDL_Point torso = sidePoint(-dwarf_rect.w * 0.02, 0.01 * dwarf_rect.h);

    fillPoint(head, head_radius + 4, rim_light, 74);
    fillPoint(torso, std::max(10, body_width / 2 + 4), rim_light, 56);

    drawThickLine(back_hip, back_knee, limb_width + 1, outline);
    drawThickLine(back_knee, back_foot, limb_width + 1, outline);
    drawThickLine(back_hip, back_knee, std::max(3, limb_width - 1), steel_dark);
    drawThickLine(back_knee, back_foot, std::max(3, limb_width - 1), leather_dark);
    drawBoot(back_foot, mirror, false, true);

    drawThickLine(back_shoulder, back_elbow, limb_width + 1, outline);
    drawThickLine(back_elbow, back_hand, limb_width + 1, outline);
    drawThickLine(back_shoulder, back_elbow, std::max(3, limb_width - 1),
                  leather_dark);
    drawThickLine(back_elbow, back_hand, std::max(3, limb_width - 1), gold_dark);
    drawGauntlet(back_hand, false);

    fillPoint(back_shoulder, shoulder_radius, outline);
    fillPoint(back_shoulder, std::max(5, shoulder_radius - 1), gold_dark);
    fillPoint(back_shoulder, std::max(3, shoulder_radius - 4), gold);

    fillPoint(sidePoint(-dwarf_rect.w * 0.03, dwarf_rect.h * 0.02),
              std::max(10, body_width / 2), outline);
    fillPoint(sidePoint(-dwarf_rect.w * 0.03, dwarf_rect.h * 0.02),
              std::max(9, body_width / 2 - 1), steel_dark);
    fillPoint(torso, std::max(8, body_width / 2 - 2), steel);
    fillPoint(front_shoulder, shoulder_radius + 2, outline);
    fillPoint(front_shoulder, shoulder_radius, gold_dark);
    fillPoint(front_shoulder, std::max(4, shoulder_radius - 3), gold);
    outlinePoint(front_shoulder, shoulder_radius, gold_light, 210);

    SDL_Rect belt{center_x - std::max(10, dwarf_rect.w / 9),
                  center_y + static_cast<int>(dwarf_rect.h * 0.12 + bob),
                  std::max(16, dwarf_rect.w / 4), std::max(6, dwarf_rect.h / 13)};
    drawInsetRect(belt, leather_dark, outline);
    SDL_Rect buckle{belt.x + belt.w - std::max(8, dwarf_rect.w / 9),
                    belt.y - 1, std::max(8, dwarf_rect.w / 8), belt.h + 2};
    drawInsetRect(buckle, gold_light, outline);

    drawThickLine(front_hip, front_knee, limb_width + 2, outline);
    drawThickLine(front_knee, front_foot, limb_width + 2, outline);
    drawThickLine(front_hip, front_knee, limb_width, steel_dark);
    drawThickLine(front_knee, front_foot, limb_width, leather);
    drawBoot(front_foot, mirror, true, false);

    drawThickLine(front_shoulder, front_elbow, limb_width + 2, outline);
    drawThickLine(front_elbow, front_hand, limb_width + 2, outline);
    drawThickLine(front_shoulder, front_elbow, limb_width, leather_dark);
    drawThickLine(front_elbow, front_hand, limb_width, gold_dark);
    drawGauntlet(front_hand, true);

    fillPoint(head, head_radius + 2, outline);
    fillPoint(head, head_radius, steel_dark);
    fillPoint(sidePoint(dwarf_rect.w * 0.03, -dwarf_rect.h * 0.25),
              std::max(5, head_radius - 2), gold_dark);
    fillPoint(sidePoint(dwarf_rect.w * 0.03, -dwarf_rect.h * 0.26),
              std::max(4, head_radius - 4), gold);
    SDL_Rect brow_band{head.x - head_radius / 2,
                       head.y - std::max(2, head_radius / 6),
                       head_radius + std::max(6, dwarf_rect.w / 11),
                       std::max(7, head_radius / 2)};
    if (mirror < 0) {
      brow_band.x = head.x - brow_band.w + head_radius / 2;
    }
    drawInsetRect(brow_band, gold, outline);
    SDL_Rect cheek_guard{head.x + mirror * (head_radius / 3),
                         head.y + std::max(1, head_radius / 8),
                         std::max(5, dwarf_rect.w / 10),
                         std::max(13, dwarf_rect.h / 7)};
    if (mirror < 0) {
      cheek_guard.x -= cheek_guard.w;
    }
    drawInsetRect(cheek_guard, steel, outline);
    fillPoint(sidePoint(dwarf_rect.w * 0.07, -dwarf_rect.h * 0.17),
              std::max(2, dwarf_rect.w / 25), eye);
    fillPoint(sidePoint(dwarf_rect.w * 0.07, -dwarf_rect.h * 0.17), 1, eye_glow);
    fillPoint(sidePoint(dwarf_rect.w * 0.10, -dwarf_rect.h * 0.14),
              std::max(3, dwarf_rect.w / 22), skin);
    const SDL_Point beard_root =
        sidePoint(dwarf_rect.w * 0.05, -dwarf_rect.h * 0.08);
    const SDL_Point beard_tip = sidePoint(dwarf_rect.w * 0.14 + braid_sway * 0.15,
                                          dwarf_rect.h * 0.24);
    const SDL_Point beard_back =
        sidePoint(-dwarf_rect.w * 0.02, dwarf_rect.h * 0.09);
    fillTriangle(beard_root, beard_back, beard_tip, outline);
    fillTriangle(sidePoint(dwarf_rect.w * 0.05, -dwarf_rect.h * 0.07),
                 sidePoint(-dwarf_rect.w * 0.01, dwarf_rect.h * 0.08), beard_tip,
                 beard_dark);
    fillTriangle(sidePoint(dwarf_rect.w * 0.06, -dwarf_rect.h * 0.06),
                 sidePoint(0, dwarf_rect.h * 0.06),
                 sidePoint(dwarf_rect.w * 0.13 + braid_sway * 0.12,
                           dwarf_rect.h * 0.22),
                 beard);
    drawThickLine(sidePoint(dwarf_rect.w * 0.06, -dwarf_rect.h * 0.05),
                  sidePoint(dwarf_rect.w * 0.11 + braid_sway * 0.08,
                            dwarf_rect.h * 0.18),
                  std::max(2, dwarf_rect.w / 18), beard_light, 210);

    outlinePoint(head, head_radius + 1, rim_light, 124);
    outlinePoint(torso, std::max(8, body_width / 2 - 2), rim_light, 86);
  }
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

  const Uint32 now = SDL_GetTicks();
  for (auto monster : game->monsters) {
    const bool waiting_for_blast =
        monster->scheduled_dynamite_blast_ticks != 0 &&
        now < monster->scheduled_dynamite_blast_ticks;
    if (!monster->is_alive && !waiting_for_blast) {
      continue;
    }

    SDL_Texture *monster_texture = getMonsterTexture(monster->id);
    if (monster_texture == nullptr) {
      continue;
    }

    if (waiting_for_blast) {
      monster->px_coord = getPixelCoord(monster->scheduled_dynamite_blast_coord, 0, 0);
    } else {
      monster->px_coord = getPixelCoord(
          monster->map_coord, element_size * monster->px_delta.x / 100.0,
          element_size * monster->px_delta.y / 100.0);
    }
    sdl_monster_rect =
        SDL_Rect{monster->px_coord.x + int(element_size * 0.05),
                 monster->px_coord.y + int(element_size * 0.05),
                 int(element_size * 0.9), int(element_size * 0.9)};
    drawMonsterGlow(sdl_monster_rect, monster->monster_char, monster->id);
    if (waiting_for_blast) {
      const Uint32 remaining_ticks = monster->scheduled_dynamite_blast_ticks - now;
      const double blink = std::sin(static_cast<double>(now) / 60.0);
      const Uint8 warning_alpha = static_cast<Uint8>(
          std::clamp(120.0 + 110.0 * std::max(0.0, blink), 0.0, 255.0));
      SDL_SetRenderDrawColor(sdl_renderer, 255, 120, 48, warning_alpha / 2);
      SDL_RenderFillCircle(sdl_renderer, sdl_monster_rect.x + sdl_monster_rect.w / 2,
                           sdl_monster_rect.y + sdl_monster_rect.h / 2,
                           std::max(7, static_cast<int>(element_size * 0.44)));
      const int alpha_mod = std::min(255, 170 + static_cast<int>(remaining_ticks % 80));
      SDL_SetTextureAlphaMod(
          monster_texture, static_cast<Uint8>(alpha_mod));
    } else {
      SDL_SetTextureAlphaMod(monster_texture, 255);
    }
    SDL_RenderCopy(sdl_renderer, monster_texture, nullptr, &sdl_monster_rect);
    SDL_SetTextureAlphaMod(monster_texture, 255);
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
  if (game == nullptr) {
    return;
  }

  const Uint32 now = SDL_GetTicks();
  for (const auto &effect : game->effects) {
    if (now < effect.started_ticks) {
      continue;
    }

    const PixelCoord effect_px = getPixelCoord(effect.coord, 0, 0);
    const int center_x = effect_px.x + element_size / 2;
    const int center_y = effect_px.y + element_size / 2;
    const Uint32 elapsed = now - effect.started_ticks;

    if (effect.type == EffectType::DynamiteExplosion) {
      const double progress =
          std::clamp(static_cast<double>(elapsed) /
                         static_cast<double>(DYNAMITE_EXPLOSION_DURATION_MS),
                     0.0, 1.0);
      const double bloom = std::sin(progress * 3.1415926535);
      const int max_radius =
          std::max(element_size,
                   static_cast<int>(element_size * effect.radius_cells * 1.02));
      const int outer_radius =
          std::max(10, static_cast<int>(max_radius * (0.28 + 0.92 * progress)));
      const int mid_radius =
          std::max(7, static_cast<int>(max_radius * (0.18 + 0.58 * bloom)));
      const int core_radius =
          std::max(4, static_cast<int>(max_radius * (0.08 + 0.28 * bloom)));
      const Uint8 outer_alpha =
          static_cast<Uint8>(std::clamp(168.0 * (1.0 - progress * 0.72), 0.0, 180.0));
      const Uint8 mid_alpha =
          static_cast<Uint8>(std::clamp(210.0 * (1.0 - progress * 0.46), 0.0, 220.0));
      const Uint8 core_alpha =
          static_cast<Uint8>(std::clamp(240.0 * (1.0 - progress * 0.30), 0.0, 255.0));

      SDL_SetRenderDrawColor(sdl_renderer, 255, 78, 20, outer_alpha);
      SDL_RenderFillCircle(sdl_renderer, center_x, center_y, outer_radius);
      SDL_SetRenderDrawColor(sdl_renderer, 255, 154, 52, mid_alpha);
      SDL_RenderFillCircle(sdl_renderer, center_x, center_y, mid_radius);
      SDL_SetRenderDrawColor(sdl_renderer, 255, 232, 138, core_alpha);
      SDL_RenderFillCircle(sdl_renderer, center_x, center_y, core_radius);

      const int spark_count = 12;
      for (int spark = 0; spark < spark_count; spark++) {
        const double angle =
            progress * 5.8 + spark * (2.0 * 3.1415926535 / spark_count);
        const int spark_start =
            std::max(6, static_cast<int>(max_radius * (0.10 + 0.20 * bloom)));
        const int spark_end =
            std::max(spark_start + 4,
                     static_cast<int>(max_radius * (0.56 + 0.46 * progress)));
        const int x1 = center_x + static_cast<int>(std::cos(angle) * spark_start);
        const int y1 = center_y + static_cast<int>(std::sin(angle) * spark_start);
        const int x2 = center_x + static_cast<int>(std::cos(angle) * spark_end);
        const int y2 = center_y + static_cast<int>(std::sin(angle) * spark_end);
        SDL_SetRenderDrawColor(
            sdl_renderer, 255, 224 - spark * 5, 112,
            static_cast<Uint8>(std::clamp(220.0 * (1.0 - progress), 0.0, 220.0)));
        SDL_RenderDrawLine(sdl_renderer, x1, y1, x2, y2);
      }

      SDL_SetRenderDrawColor(
          sdl_renderer, 255, 248, 180,
          static_cast<Uint8>(std::clamp(120.0 * (1.0 - progress), 0.0, 120.0)));
      SDL_RenderDrawCircle(sdl_renderer, center_x, center_y,
                           std::max(outer_radius - element_size / 4, mid_radius + 1));
    } else if (effect.type == EffectType::MonsterExplosion) {
      const double progress =
          std::clamp(static_cast<double>(elapsed) /
                         static_cast<double>(900.0),
                     0.0, 1.0);
      const int effect_radius = std::max(1, effect.radius_cells);
      const int outer_radius = std::max(
          5, static_cast<int>(element_size * effect_radius * (0.18 + 0.42 * progress)));
      const int inner_radius = std::max(
          3, static_cast<int>(element_size * effect_radius * (0.10 + 0.22 * progress)));
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
        drawWallTile(
            sdl_wall_rect, j > 0 && map->map_entry(j - 1, i) == ElementType::TYPE_WALL,
            i + 1 < cols &&
                map->map_entry(j, i + 1) == ElementType::TYPE_WALL,
            j + 1 < rows &&
                map->map_entry(j + 1, i) == ElementType::TYPE_WALL,
            i > 0 && map->map_entry(j, i - 1) == ElementType::TYPE_WALL);
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

void Renderer::carveRoundedWallCorner(const SDL_Rect &wall_rect, int radius,
                                      bool align_left, bool align_top) {
  if (radius <= 0) {
    return;
  }

  SDL_SetRenderDrawColor(sdl_renderer, COLOR_PATH, 0xFF);
  const double radius_value = static_cast<double>(radius);
  for (int row = 0; row < radius; ++row) {
    const double dy = radius_value - (static_cast<double>(row) + 0.5);
    const double inside_x = std::sqrt(std::max(
        0.0, radius_value * radius_value - dy * dy));
    const int cut_width = std::clamp(
        static_cast<int>(std::ceil(radius_value - inside_x)), 0, radius);
    if (cut_width <= 0) {
      continue;
    }

    SDL_Rect cut_rect{
        align_left ? wall_rect.x : wall_rect.x + wall_rect.w - cut_width,
        align_top ? wall_rect.y + row : wall_rect.y + wall_rect.h - 1 - row,
        cut_width, 1};
    SDL_RenderFillRect(sdl_renderer, &cut_rect);
  }
}

void Renderer::drawWallTile(const SDL_Rect &wall_rect, bool has_wall_up,
                            bool has_wall_right, bool has_wall_down,
                            bool has_wall_left) {
  if (sdl_wall_texture != nullptr) {
    SDL_RenderCopy(sdl_renderer, sdl_wall_texture, nullptr, &wall_rect);
  } else {
    SDL_SetRenderDrawColor(sdl_renderer, 118, 84, 68, 0xFF);
    SDL_RenderFillRect(sdl_renderer, &wall_rect);
  }

  const int max_radius = std::min(wall_rect.w, wall_rect.h) / 2;
  const int corner_radius =
      std::clamp(std::max(3, std::min(wall_rect.w, wall_rect.h) / 4), 0,
                 max_radius);
  if (corner_radius <= 1) {
    return;
  }

  if (!has_wall_up && !has_wall_left) {
    carveRoundedWallCorner(wall_rect, corner_radius, true, true);
  }
  if (!has_wall_up && !has_wall_right) {
    carveRoundedWallCorner(wall_rect, corner_radius, false, true);
  }
  if (!has_wall_down && !has_wall_left) {
    carveRoundedWallCorner(wall_rect, corner_radius, true, false);
  }
  if (!has_wall_down && !has_wall_right) {
    carveRoundedWallCorner(wall_rect, corner_radius, false, false);
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
        const auto has_layout_wall = [&](int probe_row, int probe_col) {
          return probe_row >= 0 && probe_col >= 0 &&
                 static_cast<size_t>(probe_row) < layout.size() &&
                 static_cast<size_t>(probe_col) < layout[probe_row].size() &&
                 layout[probe_row][probe_col] == BRICK;
        };
        sdl_wall_rect = SDL_Rect{x, y, element_size + 1, element_size + 1};
        drawWallTile(sdl_wall_rect, has_layout_wall(static_cast<int>(row) - 1,
                                                    static_cast<int>(col)),
                     has_layout_wall(static_cast<int>(row),
                                     static_cast<int>(col) + 1),
                     has_layout_wall(static_cast<int>(row) + 1,
                                     static_cast<int>(col)),
                     has_layout_wall(static_cast<int>(row),
                                     static_cast<int>(col) - 1));
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
  SDL_Texture *pacman_texture = getPacmanTexture(Directions::Down, false);
  if (pacman_texture != nullptr) {
    SDL_RenderCopy(sdl_renderer, pacman_texture, nullptr, &sdl_pacman_rect);
  }
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
        SDL_Texture *pacman_texture = getPacmanTexture(Directions::Down, false);
        if (pacman_texture != nullptr) {
          SDL_RenderCopy(sdl_renderer, pacman_texture, nullptr, &sdl_pacman_rect);
        }
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
