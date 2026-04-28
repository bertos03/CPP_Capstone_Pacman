/*
 * PROJECT COMMENT (definitions.h)
 * ---------------------------------------------------------------------------
 * This header is the single source of truth for BobMan's compile-time
 * configuration. It groups feature flags, asset paths, gameplay timing,
 * editor defaults, rendering colors, HUD tuning, and audio analysis values.
 *
 * Design rules for this file:
 * - Preprocessor feature toggles stay macros because #ifdef checks need them.
 * - Regular parameters use typed inline constexpr constants.
 * - Asset paths are stored as data-relative paths and are resolved at runtime
 *   through Paths::GetDataFilePath().
 * - Every value documents its unit, valid range, and the gameplay or visual
 *   consequence of changing it.
 */

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <SDL.h>

#include <array>

/**
 * @brief Compile-time feature toggles.
 *
 * Comment out these macros only when the target environment lacks the matching
 * SDL subsystem or font support. They remain macros on purpose because the
 * codebase uses them with #ifdef / #ifndef guards.
 */
#define AUDIO
#define FONT_FINE

/**
 * @brief Tuple-style draw colors intentionally kept as macros.
 *
 * These values are passed directly into SDL_SetRenderDrawColor() at a few
 * call sites that expect a raw channel list instead of an SDL_Color object.
 * Valid values are 0..255 per channel.
 */
#define COLOR_BACK 10, 0, 35
#define COLOR_PACMAN 200, 200, 0
#define COLOR_MONSTER 255, 10, 10
#define COLOR_GOODIE 0, 50, 255
#define COLOR_PATH 40, 40, 40

/**
 * @brief Asset and window configuration.
 *
 * All paths are relative to the bundled `data/` directory. They are resolved
 * at runtime to support both source-tree execution and packaged app bundles.
 */
inline constexpr const char *WINDOW_TITLE = "BobMan";
inline constexpr const char *MAPS_SUBDIRECTORY = "maps";
inline constexpr const char *FONT_ASSET_PATH = "font.ttf";
inline constexpr const char *APP_ICON_RGBA_ASSET_PATH = "app_icon_rgba.png";
inline constexpr const char *APP_ICON_FALLBACK_ASSET_PATH = "app_icon.png";
inline constexpr const char *WALL_TEXTURE_ASSET_PATH = "brick.bmp";
inline constexpr int FLOOR_TEXTURE_OPTION_COUNT = 5;
inline constexpr int FLOOR_TEXTURE_DEFAULT_INDEX = 0;
inline constexpr std::array<const char *, FLOOR_TEXTURE_OPTION_COUNT>
    FLOOR_TEXTURE_ASSET_PATHS{
        "floor_textures/floor_texture_1.jpg",
        "floor_textures/floor_texture_2.jpg",
        "floor_textures/floor_texture_3.jpg",
        "floor_textures/floor_texture_4.jpg",
        "floor_textures/floor_texture_5.jpg",
    };
inline constexpr const char *GOODIE_TEXTURE_ASSET_PATH = "goodie.bmp";
inline constexpr const char *LOGO_BRICK_TEXTURE_PATH = "brick.png";
inline constexpr const char *START_MENU_MONSTER_ASSET_PATH =
    "start_menu_monster.png";
inline constexpr const char *START_MENU_HERO_ASSET_PATH =
    "start_menu_spielfigur.png";
inline constexpr const char *WIN_SCREEN_ASSET_PATH = "win_screen_triumph.png";
inline constexpr const char *LOSE_SCREEN_ASSET_PATH = "lose_screen_defeat.png";
inline constexpr const char *WALKIE_TALKIE_ASSET_PATH = "walkie_talkie.png";
inline constexpr const char *ROCKET_ASSET_PATH = "rocket.png";
inline constexpr const char *AIRSTRIKE_PLANE_ASSET_PATH =
    "airstrike_plane.png";
inline constexpr int HUD_KEYCAP_ASSET_COUNT = 5;
inline constexpr std::array<const char *, HUD_KEYCAP_ASSET_COUNT>
    HUD_KEYCAP_ASSET_PATHS{
        "hud_keycaps/key_1.png",
        "hud_keycaps/key_2.png",
        "hud_keycaps/key_3.png",
        "hud_keycaps/key_4.png",
        "hud_keycaps/key_5.png",
    };
inline constexpr const char *AIRSTRIKE_EXPLOSION_SHEET_ASSET_PATH =
    "airstrike_explosion_sheet.png";
inline constexpr const char *MONSTER_EXPLOSION_SHEET_ASSET_PATH =
    "monster_explosion_sheet.png";
inline constexpr const char *FART_CLOUD_SHEET_ASSET_PATH =
    "fart_cloud_sheet.png";
inline constexpr const char *SLIME_BALL_ASSET_PATH = "slime_ball.png";
inline constexpr const char *SLIME_OVERLAY_ASSET_PATH = "slime_overlay.png";
inline constexpr const char *SLIME_SPLASH_SHEET_ASSET_PATH =
    "slime_splash_sheet.png";
inline constexpr const char *DYNAMITE_ASSET_PATH = "dynamite.png";
inline constexpr const char *MENU_MUSIC_PATH = "menu_music.mp3";
inline constexpr const char *WIN_MUSIC_PATH = "win_melody.mp3";
inline constexpr const char *LOSE_MUSIC_PATH = "lose_melody.mp3";
inline constexpr const char *COIN_SOUND_PATH = "coin.wav";
inline constexpr const char *WIN_SOUND_PATH = "win.wav";
inline constexpr const char *MONSTER_SHOT_SOUND_PATH = "schuss.wav";
inline constexpr const char *FIREBALL_IMPACT_SOUND_PATH = "einschlag.wav";
inline constexpr const char *SLIME_SHOT_SOUND_PATH = "slime_shot.mp3";
inline constexpr const char *SLIME_IMPACT_SOUND_PATH = "slime_impact.mp3";
inline constexpr const char *MONSTER_EXPLOSION_SOUND_PATH =
    "monsterexplosion.wav";
inline constexpr const char *MONSTER_FART_SOUND_PATH = "monster_fart.mp3";
inline constexpr const char *DYNAMITE_EXPLOSION_SOUND_PATH =
    "dynamitexplosion.wav";
inline constexpr const char *DYNAMITE_IGNITE_SOUND_PATH =
    "dynamite_ignite.mp3";
inline constexpr const char *PLASTIC_EXPLOSIVE_WALL_BREAK_SOUND_PATH =
    "plastic_explosive_wall_break.mp3";
inline constexpr const char *AIRSTRIKE_RADIO_SOUND_PATH =
    "airstrike_radio.wav";
inline constexpr const char *AIRSTRIKE_EXPLOSION_SOUND_PATH =
    "airstrike_explosion.wav";
inline constexpr const char *ROCKET_LAUNCH_SOUND_PATH = "rocket_launch.mp3";
inline constexpr const char *BIOHAZARD_BEAM_SOUND_PATH = "biohazard_beam.wav";
inline constexpr const char *ELECTRIFIED_MONSTER_ROAR_SOUND_PATH =
    "electrified_monster_roar.mp3";
inline constexpr const char *ELECTRIFIED_MONSTER_IMPACT_SOUND_PATH =
    "electrified_monster_impact.mp3";
inline constexpr const char *SETTINGS_FILE_NAME = "settings.cfg";

/**
 * @brief Map editor defaults and UI timing.
 *
 * The row and column counts define the exact map canvas size for newly created
 * maps. The end-screen minimum keeps win/loss overlays visible long enough to
 * read before the user can dismiss them.
 */
inline constexpr int EDITOR_SMALL_MAP_ROWS = 13;
inline constexpr int EDITOR_SMALL_MAP_COLS = 22;
inline constexpr int EDITOR_MEDIUM_MAP_ROWS = 17;
inline constexpr int EDITOR_MEDIUM_MAP_COLS = 30;
inline constexpr int EDITOR_LARGE_MAP_ROWS = 21;
inline constexpr int EDITOR_LARGE_MAP_COLS = 38;
inline constexpr int END_SCREEN_MINIMUM_MS = 3000;

/**
 * @brief Startup menu timing and spectrum-surface layout.
 *
 * `GAME_START_COUNTDOWN` is clamped to 3..9 seconds in code so the countdown
 * always fits on screen and remains readable. The spectrum values tune the
 * translucent side surfaces in the start menu; the margin caps how far the
 * FFT curve may expand towards the screen edge and the fill alpha controls the
 * overall intensity.
 */
inline constexpr int GAME_START_COUNTDOWN = 3;
inline constexpr int MENU_MUSIC_FADE_OUT_MS = 2000;
inline constexpr int START_MENU_SPECTRUM_OUTER_MARGIN = 10;
inline constexpr Uint8 START_MENU_SPECTRUM_FILL_ALPHA = 118;

/**
 * @brief Player life configuration.
 *
 * `PLAYER_DEFAULT_LIVES` is used when no setting has been stored yet.
 * The config menu clamps the runtime value into the inclusive range defined by
 * `PLAYER_LIVES_MIN` and `PLAYER_LIVES_MAX`.
 */
inline constexpr int PLAYER_LIVES_MIN = 1;
inline constexpr int PLAYER_LIVES_MAX = 5;
inline constexpr int PLAYER_DEFAULT_LIVES = 3;

/**
 * @brief Movement speed ratings for Pacman and monsters.
 *
 * These ratings are intentionally kept on a simple 1..10 scale so gameplay can
 * be tuned without touching the movement loops. Higher values mean less delay
 * between micro-steps and therefore faster movement.
 */
inline constexpr int SPEED_PACMAN = 8;
inline constexpr int SPEED_MONSTER = 6;

/**
 * @brief Score and loss-recovery tuning.
 *
 * `SCORE_PER_GOODIE` controls how rewarding each pellet is. The extra-life
 * invulnerability duration is used after consuming one remaining life instead
 * of ending the run immediately. Flicker settings control the visual feedback:
 * the sprite alternates between full opacity and `PACMAN_RECOVERY_ALPHA`.
 * `FINAL_LOSS_SOUND_LEAD_IN_MS` freezes the gameplay scene for two seconds so
 * the final defeat cue can land before the lose screen takes over.
 */
inline constexpr int SCORE_PER_GOODIE = 10;
inline constexpr Uint32 PACMAN_EXTRA_LIFE_INVULNERABLE_MS = 5000;
inline constexpr Uint8 PACMAN_RECOVERY_ALPHA = 128;
inline constexpr Uint32 PACMAN_RECOVERY_FLICKER_PHASE_MS = 120;
inline constexpr Uint32 FINAL_LOSS_SOUND_LEAD_IN_MS = 2000;

/**
 * @brief Pickup, projectile, status, and effect timing.
 *
 * All values are measured in milliseconds unless noted otherwise. Raising
 * visible durations makes pickups easier to collect; lowering cooldowns makes
 * enemies more aggressive; increasing radii or durations makes explosives and
 * status effects more punishing.
 */
inline constexpr Uint32 MONSTER_ANIMATION_FRAME_MS = 180;
inline constexpr Uint32 FIREBALL_STEP_DURATION_MS = 160;
inline constexpr Uint32 MONSTER_GAS_MIN_INTERVAL_MS = 40000;
inline constexpr Uint32 MONSTER_GAS_MAX_INTERVAL_MS = 60000;
inline constexpr Uint32 GAS_CLOUD_ACTIVE_MS = 20000;
inline constexpr Uint32 GAS_CLOUD_FADE_MS = 4000;
inline constexpr int GAS_CLOUD_MIN_PARTICLE_COUNT = 6;
inline constexpr int GAS_CLOUD_MAX_PARTICLE_COUNT = 10;
inline constexpr Uint32 BLUE_POTION_SPAWN_MIN_INTERVAL_MS = 20000;
inline constexpr Uint32 BLUE_POTION_SPAWN_MAX_INTERVAL_MS = 60000;
inline constexpr Uint32 BLUE_POTION_VISIBLE_MS = 15000;
inline constexpr Uint32 BLUE_POTION_FADE_MS = 1000;
inline constexpr Uint32 DYNAMITE_SPAWN_MIN_INTERVAL_MS = 5000;
inline constexpr Uint32 DYNAMITE_SPAWN_MAX_INTERVAL_MS = 10000;
inline constexpr Uint32 DYNAMITE_VISIBLE_MS = 60000;
inline constexpr Uint32 DYNAMITE_FADE_MS = 1000;
inline constexpr Uint32 PLASTIC_EXPLOSIVE_SPAWN_MIN_INTERVAL_MS = 5000;
inline constexpr Uint32 PLASTIC_EXPLOSIVE_SPAWN_MAX_INTERVAL_MS = 10000;
inline constexpr Uint32 PLASTIC_EXPLOSIVE_VISIBLE_MS = 60000;
inline constexpr Uint32 PLASTIC_EXPLOSIVE_FADE_MS = 1000;
inline constexpr Uint32 WALKIE_TALKIE_SPAWN_MIN_INTERVAL_MS = 12000;
inline constexpr Uint32 WALKIE_TALKIE_SPAWN_MAX_INTERVAL_MS = 22000;
inline constexpr Uint32 WALKIE_TALKIE_VISIBLE_MS = 60000;
inline constexpr Uint32 WALKIE_TALKIE_FADE_MS = 1000;
inline constexpr Uint32 ROCKET_SPAWN_MIN_INTERVAL_MS = 8000;
inline constexpr Uint32 ROCKET_SPAWN_MAX_INTERVAL_MS = 16000;
inline constexpr Uint32 ROCKET_VISIBLE_MS = 60000;
inline constexpr Uint32 ROCKET_FADE_MS = 1000;
inline constexpr Uint32 BIOHAZARD_SPAWN_MIN_INTERVAL_MS = 14000;
inline constexpr Uint32 BIOHAZARD_SPAWN_MAX_INTERVAL_MS = 26000;
inline constexpr Uint32 BIOHAZARD_VISIBLE_MS = 60000;
inline constexpr Uint32 BIOHAZARD_FADE_MS = 1000;
inline constexpr Uint32 BIOHAZARD_BEAM_ACTIVE_MS = 3000;
inline constexpr Uint32 BIOHAZARD_BEAM_MIN_VISIBLE_MS = 1000;
inline constexpr Uint32 BIOHAZARD_HIT_SEQUENCE_MS = 1000;
inline constexpr Uint32 BIOHAZARD_IMPACT_FLASH_DURATION_MS =
    BIOHAZARD_HIT_SEQUENCE_MS;
inline constexpr int BIOHAZARD_CHARGE_STEP_DELAY_MS = 1;
inline constexpr double BIOHAZARD_SIDE_BEAM_RAISE_FACTOR = 0.18;
inline constexpr Uint32 PLASMA_SHOCKWAVE_DURATION_MS = 1350;
inline constexpr int PLASMA_SHOCKWAVE_RADIUS_CELLS = 10;
inline constexpr Uint32 DYNAMITE_COUNTDOWN_MS = 5000;
inline constexpr Uint32 DYNAMITE_EXPLOSION_DURATION_MS = 2000;
inline constexpr int DYNAMITE_EXPLOSION_RADIUS_CELLS = 3;
inline constexpr Uint32 DYNAMITE_CHAIN_BASE_DELAY_MS = 140;
inline constexpr Uint32 DYNAMITE_CHAIN_STEP_DELAY_MS = 80;
inline constexpr Uint32 MONSTER_EXPLOSION_FRAME_MS = 35;
inline constexpr int MONSTER_EXPLOSION_FRAME_COUNT = 24;
inline constexpr Uint32 MONSTER_EXPLOSION_FADE_IN_DURATION_MS =
    MONSTER_EXPLOSION_FRAME_MS * 2;
inline constexpr int MONSTER_EXPLOSION_FADE_OUT_TAIL_FRAME_COUNT = 5;
inline constexpr Uint32 MONSTER_EXPLOSION_DURATION_MS =
    MONSTER_EXPLOSION_FRAME_MS * MONSTER_EXPLOSION_FRAME_COUNT;

/**
 * @brief Tuning for the debris/smoke trail that accompanies a monster
 *  explosion. Counts are inclusive; speeds and radii are expressed in cells
 *  (1 cell = one map tile). Lifetimes are milliseconds.
 */
inline constexpr int MONSTER_EXPLOSION_PARTICLE_MIN_COUNT = 18;
inline constexpr int MONSTER_EXPLOSION_PARTICLE_MAX_COUNT = 38;
inline constexpr float MONSTER_EXPLOSION_PARTICLE_MIN_SPEED_CELLS_PER_SEC =
    1.0f;
inline constexpr float MONSTER_EXPLOSION_PARTICLE_MAX_SPEED_CELLS_PER_SEC =
    4.5f;
inline constexpr float MONSTER_EXPLOSION_PARTICLE_GRAVITY_CELLS_PER_SEC2 =
    3.0f;
inline constexpr float MONSTER_EXPLOSION_PARTICLE_INITIAL_UPWARD_BIAS = 0.60f;
inline constexpr Uint32 MONSTER_EXPLOSION_PARTICLE_LIFETIME_MS = 400;
inline constexpr float MONSTER_EXPLOSION_PARTICLE_RADIUS_CELLS = 0.13f;
inline constexpr int MONSTER_EXPLOSION_PARTICLE_BLOB_MIN_COUNT = 8;
inline constexpr int MONSTER_EXPLOSION_PARTICLE_BLOB_MAX_COUNT = 10;
inline constexpr float MONSTER_EXPLOSION_PARTICLE_BLOB_OFFSET_FACTOR = 0.85f;
inline constexpr float MONSTER_EXPLOSION_PARTICLE_BLOB_RADIUS_MIN_FACTOR =
    0.35f;
inline constexpr float MONSTER_EXPLOSION_PARTICLE_BLOB_RADIUS_MAX_FACTOR =
    0.65f;
inline constexpr Uint8 MONSTER_EXPLOSION_PARTICLE_INITIAL_ALPHA = 220;
inline constexpr SDL_Color MONSTER_EXPLOSION_PARTICLE_COLOR{56, 52, 50, 255};
inline constexpr SDL_Color MONSTER_EXPLOSION_PARTICLE_HIGHLIGHT_COLOR{96, 90,
                                                                     86, 255};
inline constexpr Uint32 MONSTER_EXPLOSION_SMOKE_SPAWN_INTERVAL_MS = 32;
inline constexpr Uint32 MONSTER_EXPLOSION_SMOKE_LIFETIME_MS = 1500;
inline constexpr float MONSTER_EXPLOSION_SMOKE_INITIAL_RADIUS_CELLS = 0.10f;
inline constexpr float MONSTER_EXPLOSION_SMOKE_FINAL_RADIUS_CELLS = 0.46f;
inline constexpr Uint8 MONSTER_EXPLOSION_SMOKE_INITIAL_ALPHA = 165;
inline constexpr SDL_Color MONSTER_EXPLOSION_SMOKE_COLOR{82, 76, 72, 255};
inline constexpr SDL_Color MONSTER_EXPLOSION_SMOKE_HIGHLIGHT_COLOR{132, 124,
                                                                  118, 255};
inline constexpr int MONSTER_EXPLOSION_SMOKE_BLOB_MIN_COUNT = 5;
inline constexpr int MONSTER_EXPLOSION_SMOKE_BLOB_MAX_COUNT = 9;
inline constexpr float MONSTER_EXPLOSION_SMOKE_BLOB_OFFSET_FACTOR = 0.95f;
inline constexpr float MONSTER_EXPLOSION_SMOKE_BLOB_RADIUS_MIN_FACTOR = 0.45f;
inline constexpr float MONSTER_EXPLOSION_SMOKE_BLOB_RADIUS_MAX_FACTOR = 1.10f;
inline constexpr float MONSTER_EXPLOSION_SMOKE_WOBBLE_AMPLITUDE_CELLS = 0.04f;
inline constexpr float MONSTER_EXPLOSION_SMOKE_WOBBLE_FREQUENCY_HZ = 1.4f;

inline constexpr int PLASTIC_EXPLOSIVE_WALL_DEBRIS_MIN_COUNT = 12;
inline constexpr int PLASTIC_EXPLOSIVE_WALL_DEBRIS_MAX_COUNT = 26;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DEBRIS_MIN_SPEED_CELLS_PER_SEC = 2.9f;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DEBRIS_MAX_SPEED_CELLS_PER_SEC = 9.3f;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DEBRIS_GRAVITY_CELLS_PER_SEC2 = 3.4f;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DEBRIS_INITIAL_UPWARD_BIAS = 1.32f;
inline constexpr Uint32 PLASTIC_EXPLOSIVE_WALL_DEBRIS_LIFETIME_MS = 920;
inline constexpr float PLASTIC_EXPLOSIVE_WALL_DEBRIS_RADIUS_CELLS = 0.12f;
inline constexpr int PLASTIC_EXPLOSIVE_WALL_DEBRIS_BLOB_MIN_COUNT = 3;
inline constexpr int PLASTIC_EXPLOSIVE_WALL_DEBRIS_BLOB_MAX_COUNT = 9;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DEBRIS_BLOB_OFFSET_FACTOR = 0.68f;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DEBRIS_BLOB_RADIUS_MIN_FACTOR = 0.42f;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DEBRIS_BLOB_RADIUS_MAX_FACTOR = 0.94f;
inline constexpr Uint8 PLASTIC_EXPLOSIVE_WALL_DEBRIS_INITIAL_ALPHA = 232;
inline constexpr SDL_Color PLASTIC_EXPLOSIVE_WALL_DEBRIS_COLOR{118, 88, 76,
                                                               255};
inline constexpr SDL_Color
    PLASTIC_EXPLOSIVE_WALL_DEBRIS_HIGHLIGHT_COLOR{174, 136, 118, 255};
inline constexpr Uint32 PLASTIC_EXPLOSIVE_WALL_DUST_SPAWN_INTERVAL_MS = 28;
inline constexpr Uint32 PLASTIC_EXPLOSIVE_WALL_DUST_LIFETIME_MS = 800;
inline constexpr float PLASTIC_EXPLOSIVE_WALL_DUST_INITIAL_RADIUS_CELLS = 0.10f;
inline constexpr float PLASTIC_EXPLOSIVE_WALL_DUST_FINAL_RADIUS_CELLS = 0.62f;
inline constexpr Uint8 PLASTIC_EXPLOSIVE_WALL_DUST_INITIAL_ALPHA = 46;
inline constexpr SDL_Color PLASTIC_EXPLOSIVE_WALL_DUST_COLOR{146, 78, 66, 255};
inline constexpr SDL_Color
    PLASTIC_EXPLOSIVE_WALL_DUST_HIGHLIGHT_COLOR{196, 122, 108, 255};
inline constexpr int PLASTIC_EXPLOSIVE_WALL_DUST_BLOB_MIN_COUNT = 4;
inline constexpr int PLASTIC_EXPLOSIVE_WALL_DUST_BLOB_MAX_COUNT = 8;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DUST_BLOB_OFFSET_FACTOR = 0.98f;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DUST_BLOB_RADIUS_MIN_FACTOR = 0.48f;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DUST_BLOB_RADIUS_MAX_FACTOR = 1.18f;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DUST_WOBBLE_AMPLITUDE_CELLS = 0.05f;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DUST_WOBBLE_FREQUENCY_HZ = 1.2f;
inline constexpr int PLASTIC_EXPLOSIVE_WALL_DUST_INITIAL_PUFF_MIN_COUNT = 8;
inline constexpr int PLASTIC_EXPLOSIVE_WALL_DUST_INITIAL_PUFF_MAX_COUNT = 14;
inline constexpr float
    PLASTIC_EXPLOSIVE_WALL_DUST_INITIAL_SPREAD_CELLS = 0.34f;

inline constexpr int EXPLOSION_SMOKE_CLOUD_MIN_PUFF_COUNT = 14;
inline constexpr int EXPLOSION_SMOKE_CLOUD_MAX_PUFF_COUNT = 22;
inline constexpr float EXPLOSION_SMOKE_CLOUD_RING_MIN_RADIUS_CELLS = 0.55f;
inline constexpr float EXPLOSION_SMOKE_CLOUD_RING_INNER_RADIUS_FACTOR = 0.48f;
inline constexpr float EXPLOSION_SMOKE_CLOUD_RING_OUTER_RADIUS_FACTOR = 0.82f;
inline constexpr Uint32 EXPLOSION_SMOKE_CLOUD_LIFETIME_MS = 1200;
inline constexpr float EXPLOSION_SMOKE_CLOUD_INITIAL_RADIUS_CELLS = 0.09f;
inline constexpr float EXPLOSION_SMOKE_CLOUD_FINAL_RADIUS_CELLS = 0.34f;
inline constexpr Uint8 EXPLOSION_SMOKE_CLOUD_INITIAL_ALPHA = 54;
inline constexpr SDL_Color EXPLOSION_SMOKE_CLOUD_COLOR{78, 74, 72, 255};
inline constexpr SDL_Color EXPLOSION_SMOKE_CLOUD_HIGHLIGHT_COLOR{120, 114, 108,
                                                                 255};
inline constexpr int EXPLOSION_SMOKE_CLOUD_BLOB_MIN_COUNT = 4;
inline constexpr int EXPLOSION_SMOKE_CLOUD_BLOB_MAX_COUNT = 7;
inline constexpr float EXPLOSION_SMOKE_CLOUD_BLOB_OFFSET_FACTOR = 0.86f;
inline constexpr float EXPLOSION_SMOKE_CLOUD_BLOB_RADIUS_MIN_FACTOR = 0.44f;
inline constexpr float EXPLOSION_SMOKE_CLOUD_BLOB_RADIUS_MAX_FACTOR = 0.98f;
inline constexpr float EXPLOSION_SMOKE_CLOUD_WOBBLE_AMPLITUDE_CELLS = 0.03f;
inline constexpr float EXPLOSION_SMOKE_CLOUD_WOBBLE_FREQUENCY_HZ = 1.0f;

inline constexpr int WALL_RUBBLE_CENTER_MIN_COUNT = 16;
inline constexpr int WALL_RUBBLE_CENTER_MAX_COUNT = 25;
inline constexpr int WALL_RUBBLE_NEIGHBOR_ORTHOGONAL_MIN_COUNT = 3;
inline constexpr int WALL_RUBBLE_NEIGHBOR_ORTHOGONAL_MAX_COUNT = 8;
inline constexpr int WALL_RUBBLE_NEIGHBOR_DIAGONAL_MIN_COUNT = 0;
inline constexpr int WALL_RUBBLE_NEIGHBOR_DIAGONAL_MAX_COUNT = 2;
inline constexpr float WALL_RUBBLE_MIN_HALF_SIZE_CELLS = 0.040f;
inline constexpr float WALL_RUBBLE_MAX_HALF_SIZE_CELLS = 0.10f;
inline constexpr float WALL_RUBBLE_MIN_ASPECT_RATIO = 0.55f;
inline constexpr float WALL_RUBBLE_MAX_ASPECT_RATIO = 1.75f;
inline constexpr Uint8 WALL_RUBBLE_BASE_ALPHA = 224;
inline constexpr SDL_Color WALL_RUBBLE_DARK_COLOR{84, 62, 54, 255};
inline constexpr SDL_Color WALL_RUBBLE_LIGHT_COLOR{164, 132, 110, 255};

inline constexpr Uint32 MONSTER_EXPLOSION_FIREBALL_DURATION_MS = 720;
inline constexpr Uint32 MONSTER_EXPLOSION_FIREBALL_EXPANSION_MS = 170;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_PEAK_RADIUS_CELLS = 1.05f;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_LATE_GROWTH = 0.18f;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_FADE_START_PROGRESS = 0.45f;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_BLOB_OFFSET_FACTOR = 0.55f;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_BLOB_RADIUS_MIN_FACTOR = 0.50f;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_BLOB_RADIUS_MAX_FACTOR = 0.95f;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_WOBBLE_AMPLITUDE_CELLS = 0.07f;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_WOBBLE_FREQUENCY_HZ = 5.0f;

inline constexpr SDL_Color MONSTER_EXPLOSION_FIREBALL_HALO_COLOR{255, 96, 32, 255};
inline constexpr float MONSTER_EXPLOSION_FIREBALL_HALO_RADIUS_FACTOR = 1.45f;
inline constexpr Uint8 MONSTER_EXPLOSION_FIREBALL_HALO_ALPHA = 90;

inline constexpr SDL_Color MONSTER_EXPLOSION_FIREBALL_OUTER_COLOR{170, 32, 14, 255};
inline constexpr float MONSTER_EXPLOSION_FIREBALL_OUTER_RADIUS_FACTOR = 1.00f;
inline constexpr int MONSTER_EXPLOSION_FIREBALL_OUTER_BLOB_COUNT = 11;
inline constexpr Uint8 MONSTER_EXPLOSION_FIREBALL_OUTER_ALPHA = 165;

inline constexpr SDL_Color MONSTER_EXPLOSION_FIREBALL_RED_COLOR{226, 64, 24, 255};
inline constexpr float MONSTER_EXPLOSION_FIREBALL_RED_RADIUS_FACTOR = 0.78f;
inline constexpr int MONSTER_EXPLOSION_FIREBALL_RED_BLOB_COUNT = 10;
inline constexpr Uint8 MONSTER_EXPLOSION_FIREBALL_RED_ALPHA = 210;

inline constexpr SDL_Color MONSTER_EXPLOSION_FIREBALL_ORANGE_COLOR{255, 138, 38, 255};
inline constexpr float MONSTER_EXPLOSION_FIREBALL_ORANGE_RADIUS_FACTOR = 0.56f;
inline constexpr int MONSTER_EXPLOSION_FIREBALL_ORANGE_BLOB_COUNT = 9;
inline constexpr Uint8 MONSTER_EXPLOSION_FIREBALL_ORANGE_ALPHA = 235;

inline constexpr SDL_Color MONSTER_EXPLOSION_FIREBALL_YELLOW_COLOR{255, 222, 108, 255};
inline constexpr float MONSTER_EXPLOSION_FIREBALL_YELLOW_RADIUS_FACTOR = 0.36f;
inline constexpr int MONSTER_EXPLOSION_FIREBALL_YELLOW_BLOB_COUNT = 7;
inline constexpr Uint8 MONSTER_EXPLOSION_FIREBALL_YELLOW_ALPHA = 245;

inline constexpr SDL_Color MONSTER_EXPLOSION_FIREBALL_CORE_COLOR{255, 250, 224, 255};
inline constexpr float MONSTER_EXPLOSION_FIREBALL_CORE_RADIUS_FACTOR = 0.22f;
inline constexpr int MONSTER_EXPLOSION_FIREBALL_CORE_BLOB_COUNT = 5;
inline constexpr Uint8 MONSTER_EXPLOSION_FIREBALL_CORE_ALPHA = 255;

inline constexpr int MONSTER_EXPLOSION_FIREBALL_FLARE_COUNT = 14;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_FLARE_INNER_FACTOR = 0.55f;
inline constexpr float MONSTER_EXPLOSION_FIREBALL_FLARE_OUTER_FACTOR = 1.30f;
inline constexpr Uint8 MONSTER_EXPLOSION_FIREBALL_FLARE_ALPHA = 200;
inline constexpr SDL_Color MONSTER_EXPLOSION_FIREBALL_FLARE_COLOR{255, 232, 152, 255};

inline constexpr Uint32 ROCKET_STEP_DURATION_MS = 90;
inline constexpr Uint32 ROCKET_ANIMATION_FRAME_MS = 90;
inline constexpr int AIRSTRIKE_BOMB_COUNT = 10;
inline constexpr Uint32 AIRSTRIKE_RADIO_DELAY_MS = 2000;
inline constexpr Uint32 AIRSTRIKE_BOMB_FALL_MS = 320;
inline constexpr Uint32 AIRSTRIKE_PLANE_CELL_TRAVEL_MS = 105;
inline constexpr Uint32 AIRSTRIKE_PLANE_MIN_FLIGHT_MS = 2200;
inline constexpr Uint32 AIRSTRIKE_EXPLOSION_FRAME_MS = 110;
inline constexpr int AIRSTRIKE_EXPLOSION_FRAME_COUNT = 5;
inline constexpr Uint32 AIRSTRIKE_EXPLOSION_DURATION_MS =
    AIRSTRIKE_EXPLOSION_FRAME_MS * AIRSTRIKE_EXPLOSION_FRAME_COUNT;
inline constexpr int AIRSTRIKE_EXPLOSION_RADIUS_CELLS = 1;
inline constexpr Uint32 PACMAN_INVULNERABLE_MS = 30000;
inline constexpr Uint32 PACMAN_PARALYSIS_MS = 3000;
inline constexpr Uint32 SLIME_OVERLAY_HOLD_MS = 2000;
inline constexpr Uint32 SLIME_OVERLAY_FADE_MS = 1000;
inline constexpr Uint32 SLIME_SPLASH_FRAME_MS = 80;
inline constexpr int SLIME_SPLASH_FRAME_COUNT = 9;
inline constexpr Uint32 SLIME_SPLASH_FADE_MS = 1000;
inline constexpr Uint32 SLIME_SPLASH_TOTAL_DURATION_MS =
    SLIME_SPLASH_FRAME_MS * SLIME_SPLASH_FRAME_COUNT + SLIME_SPLASH_FADE_MS;
inline constexpr Uint32 TELEPORT_ANIMATION_PHASE_MS = 1000;
inline constexpr Uint32 MONSTER_EXPLOSION_EFFECT_DURATION_MS =
    MONSTER_EXPLOSION_DURATION_MS;
inline constexpr Uint32 WALL_IMPACT_EFFECT_DURATION_MS = 180;
inline constexpr Uint32 GOODIE_SPARKLE_MIN_INTERVAL_MS = 3000;
inline constexpr Uint32 GOODIE_SPARKLE_MAX_INTERVAL_MS = 10000;
inline constexpr Uint32 GOODIE_SPARKLE_DURATION_MS = 420;
inline constexpr double PLASTIC_EXPLOSIVE_MONSTER_HIT_RADIUS_CELLS = 0.72;
inline constexpr float AIRSTRIKE_PLANE_MARGIN_CELLS = 2.0f;
inline constexpr float AIRSTRIKE_PATH_SAMPLES_PER_CELL = 14.0f;

/**
 * @brief Difficulty preset values used to derive enemy behavior.
 *
 * `monster_speed_offset_from_base` is added to `SPEED_MONSTER` and clamped to
 * at least 1 at runtime. Positive spawn interval scales make extra pickups
 * rarer; values below 1.0 make them appear more frequently.
 */
struct DifficultyTuningPreset {
  int monster_speed_offset_from_base;
  double monster_step_delay_scale;
  Uint32 fireball_cooldown_ms;
  Uint32 fireball_step_duration_ms;
  Uint32 pickup_visible_ms;
  double extra_spawn_interval_scale;
};

inline constexpr DifficultyTuningPreset TRAINING_DIFFICULTY_TUNING{
    -4, 1.5, 15000, 330, 60000, 0.75};
inline constexpr DifficultyTuningPreset EASY_DIFFICULTY_TUNING{
    -3, 1.0, 15000, 220, 60000, 0.75};
inline constexpr DifficultyTuningPreset MEDIUM_DIFFICULTY_TUNING{
    -2, 1.0, 10000, 160, 40000, 1.0};
inline constexpr DifficultyTuningPreset HARD_DIFFICULTY_TUNING{
    0, 1.0, 6500, 120, 20000, 1.4};

/**
 * @brief Rendering palette used for HUD, menus, and effects.
 *
 * All channel values are 0..255. Changing these constants lets the whole UI
 * be recolored without touching renderer logic.
 */
inline constexpr SDL_Color HUD_TEXT_COLOR{243, 236, 222, 255};
inline constexpr SDL_Color MENU_TEXT_COLOR{225, 223, 218, 255};
inline constexpr SDL_Color SELECTED_MENU_TEXT_COLOR{255, 248, 238, 255};
inline constexpr SDL_Color STATUS_TEXT_COLOR{255, 189, 163, 255};
inline constexpr SDL_Color WARNING_TEXT_COLOR{255, 110, 110, 255};
inline constexpr SDL_Color PANEL_FILL_COLOR{10, 6, 18, 205};
inline constexpr SDL_Color PANEL_BORDER_COLOR{196, 130, 92, 255};
inline constexpr SDL_Color START_MENU_PANEL_FILL_COLOR{10, 6, 18, 118};
inline constexpr SDL_Color START_MENU_PANEL_BORDER_COLOR{196, 130, 92, 180};
inline constexpr SDL_Color BRICK_OUTLINE_COLOR{78, 20, 14, 255};
inline constexpr SDL_Color SHIELD_TEXT_COLOR{116, 220, 255, 255};
inline constexpr SDL_Color START_LOGO_HIGHLIGHT_COLOR{238, 252, 255, 255};
inline constexpr SDL_Color START_LOGO_LIGHT_COLOR{88, 234, 255, 255};
inline constexpr SDL_Color START_LOGO_MID_COLOR{44, 156, 255, 255};
inline constexpr SDL_Color START_LOGO_DEEP_COLOR{18, 70, 222, 255};
inline constexpr SDL_Color START_LOGO_OUTLINE_COLOR{6, 18, 102, 255};
inline constexpr SDL_Color TELEPORTER_BLUE_COLOR{78, 160, 255, 255};
inline constexpr SDL_Color TELEPORTER_RED_COLOR{255, 104, 104, 255};
inline constexpr SDL_Color TELEPORTER_GREEN_COLOR{110, 205, 118, 255};
inline constexpr SDL_Color TELEPORTER_GREY_COLOR{172, 176, 186, 255};
inline constexpr SDL_Color TELEPORTER_YELLOW_COLOR{244, 214, 88, 255};

/**
 * @brief Rendering animation and sprite-layout tuning.
 *
 * Frame counts must match the shipped sprite sheets. Scale values are
 * multiplicative factors relative to one map cell. Alpha values control the
 * baseline opacity of overlays and particle sprites. The floor and shadow
 * settings below tune the visual depth of the tilted 3D gameplay view.
 */
inline constexpr int PACMAN_FRAMES_PER_DIRECTION = 4;
inline constexpr int MONSTER_FRAMES_PER_DIRECTION = 4;
inline constexpr int ROCKET_FLIGHT_FRAME_COUNT = 2;
inline constexpr int FART_CLOUD_FRAME_COUNT = 4;
inline constexpr int SLIME_SPLASH_GRID_COLUMNS = 3;
inline constexpr int SLIME_SPLASH_GRID_ROWS = 3;
inline constexpr Uint32 PACMAN_WALK_FRAME_MS = 140;
inline constexpr Uint32 FART_CLOUD_OPEN_DURATION_MS = 320;
inline constexpr Uint32 FART_CLOUD_FRAME_MS =
    FART_CLOUD_OPEN_DURATION_MS / FART_CLOUD_FRAME_COUNT;
inline constexpr Uint8 SLIME_BALL_BASE_ALPHA = 176;
inline constexpr Uint8 SLIME_OVERLAY_BASE_ALPHA = 140;
inline constexpr Uint8 SLIME_SPLASH_BASE_ALPHA = 176;
inline constexpr double MONSTER_RENDER_SCALE = 1.19;
inline constexpr double MONSTER_EXPLOSION_RENDER_SCALE = 1.10;
inline constexpr double MONSTER_EXPLOSION_INITIAL_OPACITY = 0.50;
inline constexpr double MONSTER_EXPLOSION_FINAL_OPACITY = 0.10;
inline constexpr double FART_CLOUD_RENDER_SCALE = 0.70;
inline constexpr double SLIME_BALL_RENDER_SCALE = 0.90;
inline constexpr double SLIME_OVERLAY_RENDER_SCALE = 1.22;
inline constexpr double SLIME_SPLASH_RENDER_SCALE = 1.26;
inline constexpr double FART_CLOUD_DRIFT_X_FACTOR = 0.06;
inline constexpr double FART_CLOUD_DRIFT_Y_FACTOR = 0.15;
inline constexpr double FART_CLOUD_SCALE_PULSE_AMPLITUDE = 0.14;
inline constexpr double ROCKET_SPRITE_ANGLE_OFFSET_DEGREES = 0.0;
inline constexpr SDL_Color FLOOR_3D_NEAR_COLOR{62, 60, 72, 255};
inline constexpr SDL_Color FLOOR_3D_FAR_COLOR{34, 34, 42, 255};
inline constexpr SDL_Color FLOOR_3D_TEXTURE_NEAR_COLOR{228, 226, 220, 255};
inline constexpr SDL_Color FLOOR_3D_TEXTURE_FAR_COLOR{178, 176, 170, 255};
inline constexpr int FLOOR_TEXTURE_SAMPLE_SIZE_PX = 96;
inline constexpr int FLOOR_TEXTURE_SAMPLE_STRIDE_PX = 48;
inline constexpr double FLOOR_3D_TILE_VARIATION_STRENGTH = 0.08;
inline constexpr double FLOOR_3D_WALL_OCCLUSION_STRENGTH = 0.14;
inline constexpr double FLOOR_3D_OPEN_EDGE_HIGHLIGHT_STRENGTH = 0.09;
inline constexpr Uint8 CHARACTER_GROUND_SHADOW_BASE_ALPHA = 95;
inline constexpr double CHARACTER_GROUND_SHADOW_WIDTH_FACTOR = 0.42;
inline constexpr double CHARACTER_GROUND_SHADOW_HEIGHT_FACTOR = 0.15;
inline constexpr double CHARACTER_GROUND_SHADOW_Y_OFFSET_FACTOR = 0.06;
inline constexpr Uint8 WALKIE_TALKIE_ICON_ALPHA = 255;
inline constexpr Uint8 HUD_ICON_ALPHA = 255;

/**
 * @brief Audio mixer and spectrum-analyser configuration.
 *
 * Sample rate and channel count define the PCM format used for synth-generated
 * sounds. Spectrum values tune the retro menu equalizer; the min/max dB window
 * decides how aggressively quiet sounds are suppressed or amplified visually.
 */
inline constexpr int MENU_SPECTRUM_BAND_COUNT = 32;
inline constexpr int AUDIO_SAMPLE_RATE_HZ = 44100;
inline constexpr int AUDIO_OUTPUT_CHANNEL_COUNT = 2;
inline constexpr int AUDIO_MIX_CHUNK_SIZE = 2048;
inline constexpr int AUDIO_ALLOCATED_CHANNEL_COUNT = 32;
inline constexpr int AUDIO_RESERVED_CHANNEL_COUNT = 1;
inline constexpr double MENU_SPECTRUM_MIN_FREQUENCY_HZ = 40.0;
inline constexpr double MENU_SPECTRUM_MAX_FREQUENCY_HZ = 12000.0;
inline constexpr double MENU_SPECTRUM_MIN_DB = -52.0;
inline constexpr double MENU_SPECTRUM_MAX_DB = -8.0;
inline constexpr int MENU_SPECTRUM_MAX_FRAMES = 4096;

/**
 * @brief Map file symbols.
 *
 * These characters define the serialized map format and the keyboard shortcuts
 * used by the editor. Changing them requires existing map files to be updated
 * to the same symbol set.
 */
inline constexpr char BRICK = 'x';
inline constexpr char PATH = '.';
inline constexpr char GOODIE = 'G';
inline constexpr char PACMAN = 'P';
inline constexpr char MONSTER_FEW = 'M';
inline constexpr char MONSTER_MEDIUM = 'N';
inline constexpr char MONSTER_MANY = 'O';
inline constexpr char MONSTER_EXTRA = 'K';

/**
 * @brief Tilted 3D gameplay projection.
 *
 * When enabled, the gameplay renderer uses an oblique parallel projection:
 * walls are extruded upward from the map as textured cuboids while entities
 * remain billboarded 2D sprites drawn on top of the tilted floor. The tilt
 * angle rotates the camera backward around the horizontal axis (0 degrees =
 * classic top-down, 90 degrees = side view). The wall-height factor sets
 * the extrusion height as a multiple of one map tile. Menus, HUD, and the
 * map editor continue to render in pure 2D and are unaffected by this flag.
 */
inline constexpr bool ENABLE_3D_VIEW = true;
inline constexpr double VIEW_3D_TILT_DEGREES = 30.0;
inline constexpr double VIEW_3D_WALL_HEIGHT_FACTOR = 1.0;

#endif
