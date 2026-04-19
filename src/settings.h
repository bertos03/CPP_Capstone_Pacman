/*
 * PROJECT COMMENT (settings.h)
 * ---------------------------------------------------------------------------
 * This file stores small persistent app settings for BobMan. The intent is to
 * keep player menu preferences across restarts without introducing external
 * dependencies or large config systems.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

#include "definitions.h"
#include "globaltypes.h"

struct AppSettings {
  Difficulty difficulty{Difficulty::Medium};
  std::string selected_map_path;
  int player_lives{PLAYER_DEFAULT_LIVES};
  int floor_texture_index{FLOOR_TEXTURE_DEFAULT_INDEX};
};

AppSettings LoadAppSettings();
bool SaveAppSettings(const AppSettings &settings);

#endif
