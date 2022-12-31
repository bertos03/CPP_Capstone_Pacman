#include "renderer.h"

/**
 * @brief Destroy the Renderer:: Renderer object
 *
 */
Renderer::~Renderer() {

  // TODO: clean up images etc.

  SDL_DestroyWindow(sdl_window);
  sdl_window = NULL;
  TTF_Quit();
  SDL_Quit();
}

/**
 * @brief Construct a new Renderer:: Renderer object
 *
 * @param _map: pointer to the _map object
 */
Renderer::Renderer(Map *_map, Game *_game) : map(_map), game(_game) {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize.\n";
    std::cerr << "SDL_Error: " << SDL_GetError() << "\n";
    exit(1);
  }
  // calculate size of map elements
  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(0, &DM);
  std::cout << "Screen size is " << DM.w << "x" << DM.h << "px\n";
  screen_res_x = DM.w;
  screen_res_y = DM.h;
  rows = map->get_map_rows();
  cols = map->get_map_cols();
  int fontsize = screen_res_y / 35; // for top text - store it in a separate
                                    // variable for better readability

  int element_size_x = (screen_res_y - rows - 1) / rows;
  int element_size_y = (screen_res_x - cols - 1 - fontsize) / cols;
  // make sure elements have square, not rectangular shape
  if (element_size_x > element_size_y) {
    element_size = element_size_y;
  } else {
    element_size = element_size_x;
  }

  offset_x = (screen_res_x - (cols + 1) * element_size) / 2;
  offset_y = (screen_res_y - (rows + 1) * element_size) / 2 + fontsize;

  // Create Window
  sdl_window = SDL_CreateWindow(
      "Pacman", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DM.w, DM.h,
      SDL_WINDOW_BORDERLESS); // SDL_WINDOW_BORDERLESS //SDL_WINDOW_FULLSCREEN

  if (nullptr == sdl_window) {
    std::cerr << "Window could not be created.\n";
    std::cerr << " SDL_Error: " << SDL_GetError() << "\n";
    exit(1);
  }

  // Create renderer
  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
  if (nullptr == sdl_renderer) {
    std::cerr << "Renderer could not be created.\n";
    std::cerr << "SDL_Error: " << SDL_GetError() << "\n";
  }

  SDL_SetRenderDrawColor(sdl_renderer, COLOR_BACK, 255);
  SDL_RenderClear(sdl_renderer);
  std::cout << "Block size x:" << element_size << " (" << rows
            << " rows) y:" << element_size << " (" << cols << " cols)\n";
  std::cout << "Offset x:" << offset_x << " y:" << offset_y << "\n";

  // Define Font Stuff
  TTF_Init();

  sdl_font = TTF_OpenFont("../data/font.ttf", fontsize);
  fontsize = screen_res_y / 8;
  sdl_font_winlose = TTF_OpenFont("../data/font.ttf", fontsize);

  if (sdl_font == NULL || sdl_font_winlose == NULL) {
    std::cerr << "could not open font .- " << SDL_GetError() << "\n";
    exit(1);
  }
  sdl_font_color = SDL_Color{255, 255, 255};
  sdl_font_back_color = SDL_Color{COLOR_BACK};

  // set up icons
  sdl_wall_surface = SDL_LoadBMP(WALL_PATH);
  sdl_wall_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_wall_surface);

  sdl_goodie_surface = SDL_LoadBMP(GOODIE_PATH);
  sdl_goodie_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_goodie_surface);

  sdl_monster_surface = SDL_LoadBMP(MONSTER_PATH);
  sdl_monster_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_monster_surface);

  sdl_pacman_surface = SDL_LoadBMP(PACMAN_PATH);
  sdl_pacman_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_pacman_surface);

  if (sdl_goodie_surface == NULL || sdl_wall_surface == NULL ||
      sdl_monster_surface == NULL || sdl_pacman_surface == NULL) {
    std::cerr << "could not open icons .- " << SDL_GetError() << "\n";
    exit(1);
  }
}

void Renderer::Render() {
  SDL_SetRenderDrawColor(sdl_renderer, COLOR_BACK, 255);
  SDL_RenderClear(sdl_renderer);
  drawmap();
  drawgoodies();
  drawpacman();
  drawmonsters();
  drawtext();
  // Update Screen
  SDL_RenderPresent(sdl_renderer);
}

/**
 * @brief Draws the elements of the game
 *
 */
void Renderer::drawpacman() {
  game->pacman->px_coord = getPixelCoord(
      game->pacman->map_coord, element_size * game->pacman->px_delta.x / 100.,
      element_size * game->pacman->px_delta.y / 100.);
  // SDL_SetRenderDrawColor(sdl_renderer, COLOR_PACMAN, 0xFF);
  // SDL_RenderFillCircle(sdl_renderer,
  //                      game->pacman->px_coord.x + element_size / 2,
  //                      game->pacman->px_coord.y + element_size / 2,
  //                      (element_size - SIZE_PACMAN) / 2);
  sdl_pacman_rect =
      SDL_Rect{game->pacman->px_coord.x + int(element_size * 0.05),
               game->pacman->px_coord.y + int(element_size * 0.05),
               int(element_size * 0.9), int(element_size * 0.9)};
  SDL_RenderCopy(sdl_renderer, sdl_pacman_texture, NULL, &sdl_pacman_rect);
}
void Renderer::drawgoodies() {
  for (auto goodie : game->goodies) {
    if (goodie->is_active) {
      goodie->px_coord = getPixelCoord(goodie->map_coord, 0, 0);
      // SDL_SetRenderDrawColor(sdl_renderer, COLOR_GOODIE, 0xFF);
      // SDL_RenderFillCircle(sdl_renderer, goodie->px_coord.x + element_size /
      // 2,
      //                      goodie->px_coord.y + element_size / 2,
      //                      sin(std::chrono::system_clock::now()) *
      //                          (element_size - 2) / 4);
      sdl_goodie_rect =
          SDL_Rect{goodie->px_coord.x + int(element_size * 0.15),
                   goodie->px_coord.y + int(element_size * 0.15),
                   int(element_size * 0.7), int(element_size * 0.7)};
      SDL_RenderCopy(sdl_renderer, sdl_goodie_texture, NULL, &sdl_goodie_rect);
    }
  }
}

void Renderer::drawmonsters() {
  for (auto monster : game->monsters) {
    monster->px_coord = getPixelCoord(
        monster->map_coord, element_size * monster->px_delta.x / 100.,
        element_size * monster->px_delta.y / 100.);
    // SDL_SetRenderDrawColor(sdl_renderer, COLOR_MONSTER, 0xFF);
    // SDL_RenderFillCircle(sdl_renderer, monster->px_coord.x + element_size /
    // 2,
    //                      monster->px_coord.y + element_size / 2,
    //                      (element_size - SIZE_MONSTER) / 2);
    sdl_monster_rect =
        SDL_Rect{monster->px_coord.x + int(element_size * 0.05),
                 monster->px_coord.y + int(element_size * 0.05),
                 int(element_size * 0.9), int(element_size * 0.9)};
    SDL_RenderCopy(sdl_renderer, sdl_monster_texture, NULL, &sdl_monster_rect);
  }
}

void Renderer::drawtext() {
  string pactext =
      "\\\\\\\\ PACMAN ////         Score: " + std::to_string(game->score);
  if (game->win) {
    sdl_text_surface_winlose = TTF_RenderText_Solid(
        sdl_font_winlose, "+++ You won +++", sdl_font_color);
  }
  if (game->dead) {
    sdl_text_surface_winlose = TTF_RenderText_Solid(
        sdl_font_winlose, "+++ Game over +++", sdl_font_color);
  }

  if (game->dead || game->win) {
    sdl_font_texture_winlose =
        SDL_CreateTextureFromSurface(sdl_renderer, sdl_text_surface_winlose);
    SDL_QueryTexture(sdl_font_texture_winlose, NULL, NULL, &texW, &texH);
    sdl_text_rect = SDL_Rect{(screen_res_x - texW) / 2,
                             (screen_res_y - texH) / 2, texW, texH};
    SDL_RenderCopy(sdl_renderer, sdl_font_texture_winlose, NULL,
                   &sdl_text_rect);
  }
#ifdef FONT_FINE
  sdl_text_surface = TTF_RenderUTF8_LCD(sdl_font, pactext.c_str(),
                                        sdl_font_color, sdl_font_back_color);
#endif
#ifndef FONT_FINE
  sdl_text_surface = TTF_RenderText_Solid(sdl_font, pactext.c_str(),
                                        sdl_font_color);
#endif
  sdl_font_texture =
      SDL_CreateTextureFromSurface(sdl_renderer, sdl_text_surface);
  SDL_QueryTexture(sdl_font_texture, NULL, NULL, &texW, &texH);
  sdl_text_rect = SDL_Rect{(screen_res_x - texW) / 2, 30, texW, texH};
  SDL_RenderCopy(sdl_renderer, sdl_font_texture, NULL, &sdl_text_rect);
  /////////
}

/**
 * @brief draws the map
 *
 */
void Renderer::drawmap() {
  // SDL_RenderClear(sdl_renderer);
  SDL_Rect block;
  block.w = element_size; // screen_width / grid_width;
  block.h = element_size; // screen_height / grid_height;
  SDL_SetRenderDrawColor(sdl_renderer, 50, 50, 50, 0xFF);
  int x;
  int y;
  for (size_t i = 0; i < cols; i++) {
    for (size_t j = 0; j < rows; j++) {
      ElementType temp = map->map_entry(j, i); // for debugging reasons
      x = offset_x + 1 + i * (element_size + 1);
      y = offset_y + 1 + j * (element_size + 1);
      switch (temp) {
      case ElementType::TYPE_WALL:
        // SDL_SetRenderDrawColor(sdl_renderer, 50, 50, 50, 0xFF);
        // block.x = x;
        // block.y = y;
        // SDL_RenderFillRect(sdl_renderer, &block);
        sdl_wall_rect = SDL_Rect{x, y, element_size + 1, element_size + 1};
        SDL_RenderCopy(sdl_renderer, sdl_wall_texture, NULL, &sdl_wall_rect);
        break;
      case ElementType::TYPE_PATH:
        SDL_SetRenderDrawColor(sdl_renderer, COLOR_PATH, 0xFF);
        block.x = x;
        block.y = y;
        SDL_RenderFillRect(sdl_renderer, &block);
        break;
      default:
        SDL_SetRenderDrawColor(sdl_renderer, COLOR_PATH, 0xFF);
        block.x = x;
        block.y = y;
        SDL_RenderFillRect(sdl_renderer, &block);
      };
    }
  }
}

/**
 * @brief Draw circle primitive on SDL2 renderer
 * taken from
 * https://gist.github.com/Gumichan01/332c26f6197a432db91cc4327fcabb1c
 * @param renderer Pointer to renderer
 * @param x
 * @param y
 * @param radius
 * @return int
 */
int Renderer::SDL_RenderDrawCircle(SDL_Renderer *renderer, int x, int y,
                                   int radius) {
  int offsetx, offsety, d;
  int status;

  // CHECK_RENDERER_MAGIC(renderer, -1);

  offsetx = 0;
  offsety = radius;
  d = radius - 1;
  status = 0;

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

/**
 * @brief Draw filled circle primitive on SDL2 renderer
 * taken from
 * https://gist.github.com/Gumichan01/332c26f6197a432db91cc4327fcabb1c
 * @param renderer Pointer to renderer
 * @param x
 * @param y
 * @param radius
 * @return int
 */
int Renderer::SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y,
                                   int radius) {
  int offsetx, offsety, d;
  int status;

  // CHECK_RENDERER_MAGIC(renderer, -1);

  offsetx = 0;
  offsety = radius;
  d = radius - 1;
  status = 0;

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

PixelCoord Renderer::getPixelCoord(MapCoord _in, int delta_x, int delta_y) {
  PixelCoord out;
  out.x = offset_x + 1 + _in.v * (element_size + 1) + delta_x;
  out.y = offset_y + 1 + _in.u * (element_size + 1) + delta_y;
  return out;
}