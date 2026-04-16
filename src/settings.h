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

#include "globaltypes.h"

struct AppSettings {
  MonsterAmount monster_amount{MonsterAmount::Many};
  std::string selected_map_path;
};

AppSettings LoadAppSettings();
bool SaveAppSettings(const AppSettings &settings);

#endif
