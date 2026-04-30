/*
 * PROJECT COMMENT (map.h)
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

#ifndef MAP_H
#define MAP_H

#include <fstream>
#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "definitions.h"
#include "globaltypes.h"

using std::string;
using std::vector;

class Renderer;

struct MapDefinition {
  // Full file path of the map.
  std::string file_path;
  // Display name from the first line of the file.
  std::string display_name;
};

struct MapFileData {
  // Name shown in menus/HUD.
  std::string display_name;
  // Layout lines only, without the header.
  std::vector<std::string> layout_rows;
};

struct TeleporterPair {
  // Teleporter ID ('1'..'5').
  char digit;
  // Entry and exit coordinates of this portal.
  MapCoord first;
  MapCoord second;
};

enum class ElementType {
  TYPE_WALL,
  TYPE_PATH,
  TYPE_CRATER,
  TYPE_TELEPORTER,
  TYPE_GOODIE,
  TYPE_PACMAN,
  TYPE_MONSTER
};

inline bool IsBlockingMapElement(ElementType entry) {
  return entry == ElementType::TYPE_WALL || entry == ElementType::TYPE_CRATER;
}

class Map {
public:
  Map(const std::string &map_path);
  ~Map();
  static std::vector<MapDefinition>
  DiscoverAvailableMaps(const std::string &directory_path = "");
  static bool LoadMapFile(const std::string &map_path, MapFileData &map_file);
  static bool SaveMapFile(const std::string &map_path, const MapFileData &map_file);
  // Teleporters are defined as digits 1..5.
  static bool IsTeleporterChar(char);
  size_t get_map_rows();
  size_t get_map_cols();
  ElementType map_entry(size_t, size_t) const;
  ElementType Char2Type(char);
  char Type2Char(ElementType);
  int get_number_monsters();
  int get_number_goodies();
  MapCoord get_coord_monster(int);
  char get_char_monster(int);
  MapCoord get_coord_goodie(int);
  MapCoord get_coord_pacman();
  std::string get_map_name() const;
  void get_options(MapCoord, std::vector<Directions>&);
  const std::vector<TeleporterPair> &get_teleporter_pairs() const;
  bool TryGetTeleporterDestination(MapCoord, MapCoord &, char &) const;
  bool SetEntry(MapCoord coord, ElementType entry);

protected:
private:
  bool IsInsideBounds(MapCoord) const;
  void LoadMap(const std::string);
  bool IsMonsterChar(char);
  std::vector<MapCoord> monster_coord;
  std::vector<char> monster_chars;
  std::vector<MapCoord> goodie_coord;
  std::vector<TeleporterPair> teleporter_pairs;
  MapCoord pacman_coord;
  std::string map_name;
  std::string map_path;
  std::shared_ptr<vector<vector<ElementType>>> map;
  friend class Renderer;
};

#endif
