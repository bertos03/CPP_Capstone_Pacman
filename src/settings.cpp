/*
 * PROJECT COMMENT (settings.cpp)
 * ---------------------------------------------------------------------------
 * This file persists lightweight menu settings in a simple key=value text
 * file located in SDL's per-user preferences directory.
 */

#include "settings.h"

#include <SDL.h>

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

std::string MonsterAmountToString(MonsterAmount monster_amount) {
  switch (monster_amount) {
  case MonsterAmount::Few:
    return "few";
  case MonsterAmount::Medium:
    return "medium";
  case MonsterAmount::Many:
  default:
    return "many";
  }
}

MonsterAmount ParseMonsterAmount(const std::string &value) {
  const std::string trimmed_value = TrimWhitespace(value);
  if (trimmed_value == "few") {
    return MonsterAmount::Few;
  }
  if (trimmed_value == "medium") {
    return MonsterAmount::Medium;
  }
  if (trimmed_value == "many") {
    return MonsterAmount::Many;
  }
  return MonsterAmount::Many;
}

fs::path GetSettingsFilePath() {
  const std::string preference_root =
      ConsumeSdlPath(SDL_GetPrefPath("BobMan", "BobMan"));
  if (preference_root.empty()) {
    return fs::path("settings.cfg");
  }

  const fs::path settings_directory(preference_root);
  std::error_code create_error;
  fs::create_directories(settings_directory, create_error);
  return settings_directory / "settings.cfg";
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
    if (key == "monster_amount") {
      settings.monster_amount = ParseMonsterAmount(value);
    } else if (key == "selected_map_path") {
      settings.selected_map_path = value;
    }
  }

  return settings;
}

bool SaveAppSettings(const AppSettings &settings) {
  const fs::path settings_path = GetSettingsFilePath();
  std::ofstream output(settings_path, std::ios::trunc);
  if (!output.is_open()) {
    std::cerr << "Could not write settings file: " << settings_path << "\n";
    return false;
  }

  output << "monster_amount="
         << MonsterAmountToString(settings.monster_amount) << "\n";
  output << "selected_map_path=" << settings.selected_map_path << "\n";

  if (!output.good()) {
    std::cerr << "Failed while writing settings file: " << settings_path
              << "\n";
    return false;
  }

  return true;
}
