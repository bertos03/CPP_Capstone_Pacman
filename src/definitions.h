/*
 * PROJECT COMMENT (definitions.h)
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


// Flags to comment out for compatibility with Udacity workspace
#define AUDIO 
#define FONT_FINE

#define COLOR_BACK 10,0,35
#define COLOR_PACMAN 200, 200, 0
#define COLOR_MONSTER 255, 10, 10
#define COLOR_GOODIE 0, 50, 255
#define COLOR_PATH 40,40,40

#define MAPS_DIRECTORY_PATH "../data/maps"
#define FONT_PATH "../data/font.ttf"
#define LOGO_BRICK_TEXTURE_PATH "../data/brick.png"
#define FONT_COLOR &white
#define WINDOW_TITLE "Bobman"

#define GAME_START_COUNTDOWN 3 // allowed values: 3..9 seconds

#define COIN_SOUND_PATH "../data/coin.wav"
#define WIN_SOUND_PATH "../data/win.wav"
#define GAMEOVER_SOUND_PATH "../data/gameover.wav"

#define WALL_PATH "../data/brick.bmp"
#define PACMAN_PATH "../data/pacman.bmp"
#define GOODIE_PATH "../data/goodie.bmp"
#define MONSTER_PATH "../data/monster.bmp"
#define MONSTER_FRAME_1_PATH "../data/monster_1.bmp"
#define MONSTER_FRAME_2_PATH "../data/monster_2.bmp"
#define MONSTER_FRAME_3_PATH "../data/monster_3.bmp"

#define MONSTER_ANIMATION_FRAME_MS 180
#define FIREBALL_STEP_DURATION_MS 160 // higher values make fireballs slower
#define MONSTER_GAS_MIN_INTERVAL_MS 30000
#define MONSTER_GAS_MAX_INTERVAL_MS 60000
#define GAS_CLOUD_ACTIVE_MS 5000
#define GAS_CLOUD_FADE_MS 1000
#define PACMAN_PARALYSIS_MS 2000
#define TELEPORT_ANIMATION_PHASE_MS 1000

#define BRICK 'x'
#define PATH '.'
#define GOODIE 'G'
#define PACMAN 'P'
#define MONSTER_FEW 'M'
#define MONSTER_MEDIUM 'N'
#define MONSTER_MANY 'O'

#define SPEED_PACMAN 8 // 1..10
#define SPEED_MONSTER 6 // 1..10
// #define SIZE_PACMAN 5 // cellwidth - 2*X
// #define SIZE_MONSTER 10 // cellwidth - 2*X
#define SCORE_PER_GOODIE 10 //
