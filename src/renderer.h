/*
 * PROJECT COMMENT (renderer.h)
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
  Renderer(size_t rows, size_t cols);
  ~Renderer();

  // Main render path for the running game.
  void Render();
  // Menu screens for the startup flow.
  void RenderStartMenu(int selected_item, const std::string &map_name,
                       const std::string &status_message);
  void RenderConfigMenu(int selected_item, MonsterAmount monster_amount);
  void RenderMapSelectionMenu(const std::vector<std::string> &map_names,
                              int selected_index);
  void RenderEditorSelectionMenu(const std::vector<std::string> &items,
                                 int selected_index);
  void RenderEditorSizeSelectionMenu(int selected_index);
  void RenderEditor(const std::vector<std::string> &layout,
                    const std::string &map_name, MapCoord cursor,
                    const std::string &warning_message,
                    bool show_exit_dialog, int exit_dialog_selected,
                    bool show_name_dialog, const std::string &name_input,
                    const std::string &name_dialog_message);
  // Countdown before the game starts.
  void RenderCountdown(int seconds_left);
  // Helper for the editor: screen coordinate => map cell.
  bool TryGetLayoutCoordFromScreen(int screen_x, int screen_y,
                                   MapCoord &coord) const;

private:
  void initializeRenderer(size_t row_count, size_t col_count);
  void renderFrame(bool show_hud);
  void renderLayoutFrame(const std::vector<std::string> &layout);
  void drawWallTile(const SDL_Rect &wall_rect, bool has_wall_up,
                    bool has_wall_right, bool has_wall_down,
                    bool has_wall_left);
  void carveRoundedWallCorner(const SDL_Rect &wall_rect, int radius,
                              bool align_left, bool align_top);
  void drawmap();
  void drawLayout(const std::vector<std::string> &layout);
  void drawteleporters();
  void drawLayoutTeleporters(const std::vector<std::string> &layout);
  void drawTeleporterGlyph(const SDL_Rect &teleporter_rect, char teleporter_digit,
                           int animation_seed);
  void drawbonusflask();
  void drawdynamitepickup();
  void drawplasticexplosivepickup();
  void drawwalkietalkiepickup();
  void drawpacman();
  void drawmonsters();
  void drawgoodies();
  void drawplaceddynamites();
  void drawplacedplasticexplosive();
  void drawactiveairstrike();
  void drawStaticPacman();
  void drawStaticMonsters();
  void drawStaticGoodies();
  void drawLayoutEntities(const std::vector<std::string> &layout);
  void drawgasclouds();
  void drawfireballs();
  void draweffects();
  void drawDefeatEffect();
  void drawVictoryOverlay();
  void drawDwarf(const SDL_Rect &dwarf_rect, Directions facing_direction,
                 double gait_phase, bool walking);
  void drawPacmanShield(int center_x, int center_y, int base_radius,
                        double pulse_clock);
  void drawWalkieTalkieIcon(const SDL_Rect &icon_rect, Uint8 alpha,
                            double animation_clock);
  void drawAirstrikePlane(const SDL_FPoint &center, double angle_degrees,
                          int max_dimension, double wobble_phase);
  void drawDynamiteIcon(const SDL_Rect &icon_rect, bool lit_fuse,
                        double animation_clock, Uint8 alpha,
                        double fuse_burn_progress);
  void drawPlasticExplosiveIcon(const SDL_Rect &icon_rect, double animation_clock,
                                Uint8 alpha, bool armed);
  void drawhud();
  void drawDimmer(Uint8 alpha);
  void drawPanel(const SDL_Rect &panel, const SDL_Color &fill_color,
                 const SDL_Color &border_color);
  void drawStartMenuSpectrum(const SDL_Rect &panel);
  void drawStartMenuOverlay(int selected_item,
                            const std::string &map_name,
                            const std::string &status_message);
  void drawConfigMenuOverlay(int selected_item, MonsterAmount monster_amount);
  void drawMapSelectionOverlay(const std::vector<std::string> &map_names,
                               int selected_index);
  void drawEditorSelectionOverlay(const std::vector<std::string> &items,
                                  int selected_index);
  void drawEditorSizeSelectionOverlay(int selected_index);
  void drawEditorOverlay(const std::string &map_name, MapCoord cursor,
                         const std::string &warning_message,
                         bool show_exit_dialog, int exit_dialog_selected,
                         bool show_name_dialog, const std::string &name_input,
                         const std::string &name_dialog_message);
  void drawCountdownOverlay(int seconds_left);
  void renderSimpleText(TTF_Font *font, const std::string &text,
                        const SDL_Color &color, int center_x, int top_y);
  void renderTextLeft(TTF_Font *font, const std::string &text,
                      const SDL_Color &color, int left_x, int top_y);
  void renderStartLogo(TTF_Font *font, const std::string &text, int center_x,
                       int top_y);
  void renderBrickText(TTF_Font *font, const std::string &text, int center_x,
                       int top_y, const SDL_Color &outline_color);
  SDL_Texture *loadTrimmedTexture(const std::string &path,
                                  SDL_Point &trimmed_size);
  SDL_Texture *loadTrimmedChromaKeyTexture(const std::string &path,
                                           SDL_Point &trimmed_size,
                                           Uint8 tolerance);
  void renderDecorativeTexture(SDL_Texture *texture,
                               const SDL_Rect &destination,
                               const SDL_Color &glow_color,
                               double sway_phase,
                               double sway_strength_x,
                               double sway_strength_y,
                               double sway_speed);
  SDL_Surface *createStartLogoSurface(TTF_Font *font, const std::string &text);
  SDL_Texture *getPacmanTexture(Directions facing_direction,
                                bool walking) const;
  int getPacmanAnimationFrame(bool walking) const;
  SDL_Texture *getMonsterTexture(char monster_char,
                                 Directions facing_direction,
                                 int animation_seed);
  int getMonsterAnimationFrame(int animation_seed) const;
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

  std::vector<SDL_Surface *> sdl_monster_surfaces;
  std::vector<SDL_Texture *> sdl_monster_textures;
  SDL_Rect sdl_monster_rect;

  std::vector<SDL_Surface *> sdl_pacman_surfaces;
  std::vector<SDL_Texture *> sdl_pacman_textures;
  SDL_Rect sdl_pacman_rect;

  SDL_Surface *sdl_logo_brick_surface;
  SDL_Texture *sdl_dynamite_texture;
  SDL_Point sdl_dynamite_size;
  SDL_Texture *sdl_start_menu_monster_texture;
  SDL_Point sdl_start_menu_monster_size;
  SDL_Texture *sdl_start_menu_hero_texture;
  SDL_Point sdl_start_menu_hero_size;
  SDL_Texture *sdl_win_screen_texture;
  SDL_Point sdl_win_screen_size;
  SDL_Texture *sdl_walkie_talkie_texture;
  SDL_Point sdl_walkie_talkie_size;
  SDL_Texture *sdl_airstrike_plane_texture;
  SDL_Point sdl_airstrike_plane_size;
  SDL_Texture *sdl_airstrike_explosion_texture;
  SDL_Point sdl_airstrike_explosion_size;

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
