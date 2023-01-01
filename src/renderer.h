#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <math.h>
#include <string>
#include <thread>
#include <vector>
#include <memory>

#include "definitions.h"
#include "game.h"
#include "globaltypes.h"
#include "map.h"

class Map;
class Game;

class Renderer {
public:
  Renderer(Map *map, Game *game);
  ~Renderer();

  void Render();
  // void UpdateWindowTitle(int score, int fps);

private:
  void drawmap();
  void drawpacman();
  void drawmonsters();
  void drawgoodies();
  void drawtext();

  int SDL_RenderDrawCircle(SDL_Renderer *, int, int, int);
  int SDL_RenderFillCircle(SDL_Renderer *, int, int, int);
  PixelCoord getPixelCoord(MapCoord, int, int);
  int screen_res_x;
  int screen_res_y;
  int element_size;
  int offset_x;
  int offset_y;
  size_t rows;
  size_t cols;
  Map *map;
  Game *game;
  SDL_Window *sdl_window;
  SDL_Renderer *sdl_renderer;

  SDL_Surface *sdl_wall_surface;
  SDL_Texture *sdl_wall_texture;
  SDL_Rect sdl_wall_rect;

  SDL_Surface *sdl_goodie_surface;
  SDL_Texture *sdl_goodie_texture;
  SDL_Rect sdl_goodie_rect;

  SDL_Surface *sdl_monster_surface;
  SDL_Texture *sdl_monster_texture;
  SDL_Rect sdl_monster_rect;

  SDL_Surface *sdl_pacman_surface;
  SDL_Texture *sdl_pacman_texture;
  SDL_Rect sdl_pacman_rect;

  SDL_Surface *sdl_text_surface;
  SDL_Surface *sdl_text_surface_winlose;
  TTF_Font *sdl_font;
  TTF_Font *sdl_font_winlose;
  SDL_Color sdl_font_color;
  SDL_Color sdl_font_back_color;
  SDL_Texture *sdl_font_texture;
  SDL_Texture *sdl_font_texture_winlose;
  SDL_Rect sdl_rect;

  int texW, texH;
  SDL_Rect sdl_text_rect;
  SDL_Rect sdl_text_winlose;

  int renderstyle;
  enum { RENDER_LATIN1, RENDER_UTF8, RENDER_UNICODE } rendertype;

  SDL_Texture *el_wall; // for future use
};

#endif