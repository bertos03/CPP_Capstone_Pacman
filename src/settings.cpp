/*
 * PROJECT COMMENT (settings.cpp)
 * ---------------------------------------------------------------------------
 * This file persists lightweight menu settings in a simple key=value text
 * file located in SDL's per-user preferences directory.
 */

#include "settings.h"

#include <SDL.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

namespace {

namespace fs = std::filesystem;

std::string ConsumeSdlPath(char *raw_path) {
  if (raw_path == nullptr) {
    return "";
  }

  std::string path(raw_path);
  SDL_free(raw_path);
  return path;
}

std::string TrimWhitespace(const std::string &text) {
  const size_t first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }

  const size_t last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

std::string DifficultyToString(Difficulty difficulty) {
  switch (difficulty) {
  case Difficulty::Training:
    return "training";
  case Difficulty::Easy:
    return "easy";
  case Difficulty::Medium:
    return "medium";
  case Difficulty::Hard:
  default:
    return "hard";
  }
}

Difficulty ParseDifficulty(const std::string &value) {
  const std::string trimmed_value = TrimWhitespace(value);
  if (trimmed_value == "training") {
    return Difficulty::Training;
  }
  if (trimmed_value == "easy") {
    return Difficulty::Easy;
  }
  if (trimmed_value == "medium") {
    return Difficulty::Medium;
  }
  if (trimmed_value == "hard") {
    return Difficulty::Hard;
  }
  return Difficulty::Medium;
}

Difficulty ParseLegacyMonsterAmountAsDifficulty(const std::string &value) {
  const std::string trimmed_value = TrimWhitespace(value);
  if (trimmed_value == "few") {
    return Difficulty::Easy;
  }
  if (trimmed_value == "medium") {
    return Difficulty::Medium;
  }
  if (trimmed_value == "many") {
    return Difficulty::Hard;
  }
  return Difficulty::Medium;
}

int ParsePlayerLives(const std::string &value) {
  const std::string trimmed_value = TrimWhitespace(value);
  if (trimmed_value.empty()) {
    return PLAYER_DEFAULT_LIVES;
  }

  try {
    return std::clamp(std::stoi(trimmed_value), PLAYER_LIVES_MIN,
                      PLAYER_LIVES_MAX);
  } catch (...) {
    return PLAYER_DEFAULT_LIVES;
  }
}

int ParseFloorTextureIndex(const std::string &value) {
  const std::string trimmed_value = TrimWhitespace(value);
  if (trimmed_value.empty()) {
    return FLOOR_TEXTURE_DEFAULT_INDEX;
  }

  try {
    return std::clamp(std::stoi(trimmed_value), 0,
                      FLOOR_TEXTURE_OPTION_COUNT - 1);
  } catch (...) {
    return FLOOR_TEXTURE_DEFAULT_INDEX;
  }
}

fs::path GetSettingsFilePath() {
  const std::string preference_root =
      ConsumeSdlPath(SDL_GetPrefPath("BobMan", "BobMan"));
  if (preference_root.empty()) {
    return fs::path(SETTINGS_FILE_NAME);
  }

  const fs::path settings_directory(preference_root);
  std::error_code create_error;
  fs::create_directories(settings_directory, create_error);
  return settings_directory / SETTINGS_FILE_NAME;
}

} // namespace

AppSettings LoadAppSettings() {
  AppSettings settings;
  std::ifstream input(GetSettingsFilePath());
  if (!input.is_open()) {
    return settings;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    if (line.empty() || line.front() == '#') {
      continue;
    }

    const size_t separator = line.find('=');
    if (separator == std::string::npos) {
      continue;
    }

    const std::string key = TrimWhitespace(line.substr(0, separator));
    const std::string value = line.substr(separator + 1);
    if (key == "difficulty") {
      settings.difficulty = ParseDifficulty(value);
    } else if (key == "monster_amount") {
      settings.difficulty = ParseLegacyMonsterAmountAsDifficulty(value);
    } else if (key == "selected_map_path") {
      settings.selected_map_path = value;
    } else if (key == "player_lives") {
      settings.player_lives = ParsePlayerLives(value);
    } else if (key == "floor_texture_index") {
      settings.floor_texture_index = ParseFloorTextureIndex(value);
    }
  }

  settings.player_lives =
      std::clamp(settings.player_lives, PLAYER_LIVES_MIN, PLAYER_LIVES_MAX);
  settings.floor_texture_index =
      std::clamp(settings.floor_texture_index, 0, FLOOR_TEXTURE_OPTION_COUNT - 1);
  return settings;
}

bool SaveAppSettings(const AppSettings &settings) {
  const fs::path settings_path = GetSettingsFilePath();
  std::ofstream output(settings_path, std::ios::trunc);
  if (!output.is_open()) {
    std::cerr << "Could not write settings file: " << settings_path << "\n";
    return false;
  }

  output << "difficulty=" << DifficultyToString(settings.difficulty) << "\n";
  output << "selected_map_path=" << settings.selected_map_path << "\n";
  output << "player_lives="
         << std::clamp(settings.player_lives, PLAYER_LIVES_MIN,
                       PLAYER_LIVES_MAX)
         << "\n";
  output << "floor_texture_index="
         << std::clamp(settings.floor_texture_index, 0,
                       FLOOR_TEXTURE_OPTION_COUNT - 1)
         << "\n";

  if (!output.good()) {
    std::cerr << "Failed while writing settings file: " << settings_path
              << "\n";
    return false;
  }

  return true;
}
