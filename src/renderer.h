/*
 * PROJEKTKOMMENTAR (renderer.h)
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

#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "definitions.h"
#include "globaltypes.h"

class Map;
class Game;

class Renderer {
public:
  Renderer(Map *map, Game *game);
  ~Renderer();

  void Render();
  void RenderStartMenu(int selected_item, const std::string &map_name,
                       const std::string &status_message);
  void RenderConfigMenu(int selected_item, MonsterAmount monster_amount);
  void RenderMapSelectionMenu(const std::vector<std::string> &map_names,
                              int selected_index);
  void RenderCountdown(int seconds_left);

private:
  void renderFrame(bool show_hud);
  void drawmap();
  void drawpacman();
  void drawmonsters();
  void drawgoodies();
  void drawhud();
  void drawDimmer(Uint8 alpha);
  void drawPanel(const SDL_Rect &panel, const SDL_Color &fill_color,
                 const SDL_Color &border_color);
  void drawStartMenuOverlay(int selected_item,
                            const std::string &map_name,
                            const std::string &status_message);
  void drawConfigMenuOverlay(int selected_item, MonsterAmount monster_amount);
  void drawMapSelectionOverlay(const std::vector<std::string> &map_names,
                               int selected_index);
  void drawCountdownOverlay(int seconds_left);
  void renderSimpleText(TTF_Font *font, const std::string &text,
                        const SDL_Color &color, int center_x, int top_y);
  void renderBrickText(TTF_Font *font, const std::string &text, int center_x,
                       int top_y, const SDL_Color &outline_color);
  SDL_Surface *createBrickTextSurface(TTF_Font *font, const std::string &text,
                                      const SDL_Color &outline_color);
  Uint32 readPixel(SDL_Surface *surface, int x, int y);
  void writePixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

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

  SDL_Surface *sdl_logo_brick_surface;

  TTF_Font *sdl_font_hud;
  TTF_Font *sdl_font_menu;
  TTF_Font *sdl_font_logo;
  TTF_Font *sdl_font_display;

  SDL_Color sdl_font_color;
  SDL_Color sdl_font_back_color;

  int texW;
  int texH;
};

#endif
