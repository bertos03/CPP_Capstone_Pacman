/*
 * PROJECT COMMENT (map.cpp)
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

#include "map.h"
#include "paths.h"

#include <array>
#include <cctype>
#include <filesystem>

namespace {

namespace fs = std::filesystem;

std::string TrimWhitespace(const std::string &text) {
  const auto first =
      std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
      });
  if (first == text.end()) {
    return "";
  }

  const auto last =
      std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
      }).base();
  return std::string(first, last);
}

std::string NormalizeMapIdentity(const std::string &display_name) {
  std::string identity = TrimWhitespace(display_name);
  std::transform(identity.begin(), identity.end(), identity.begin(),
                 [](unsigned char ch) {
                   return static_cast<char>(std::tolower(ch));
                 });
  return identity;
}

bool PreferMapCandidate(const MapDefinition &candidate,
                        const MapDefinition &existing) {
  const bool candidate_is_writable = Paths::IsWritableMapPath(candidate.file_path);
  const bool existing_is_writable = Paths::IsWritableMapPath(existing.file_path);
  if (candidate_is_writable != existing_is_writable) {
    return candidate_is_writable;
  }
  return candidate.file_path < existing.file_path;
}

bool ReadMapFileContents(const std::string &map_path, MapFileData &map_file) {
  std::ifstream stream(map_path);
  if (!stream) {
    return false;
  }

  map_file = {};
  if (!std::getline(stream, map_file.display_name) || map_file.display_name.empty()) {
    map_file.display_name = std::filesystem::path(map_path).stem().string();
  }

  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      map_file.layout_rows.push_back(line);
    }
  }

  return !map_file.layout_rows.empty();
}

void DiscoverMapsInDirectory(const fs::path &map_directory,
                             std::vector<MapDefinition> &maps) {
  if (!fs::exists(map_directory) || !fs::is_directory(map_directory)) {
    return;
  }

  for (const auto &entry : fs::directory_iterator(map_directory)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const std::string extension = entry.path().extension().string();
    if (extension != ".txt" && extension != ".map") {
      continue;
    }

    const std::string file_path = entry.path().string();
    MapFileData map_file;
    const std::string display_name =
        ReadMapFileContents(file_path, map_file) ? map_file.display_name
                                                 : entry.path().stem().string();
    const MapDefinition candidate{file_path, display_name};

    const auto duplicate_path =
        std::find_if(maps.begin(), maps.end(), [&](const MapDefinition &map) {
          return map.file_path == file_path;
        });
    if (duplicate_path != maps.end()) {
      continue;
    }

    const std::string candidate_identity =
        NormalizeMapIdentity(candidate.display_name);
    const auto duplicate_identity = std::find_if(
        maps.begin(), maps.end(), [&](const MapDefinition &map) {
          return NormalizeMapIdentity(map.display_name) == candidate_identity;
        });
    if (duplicate_identity != maps.end()) {
      if (PreferMapCandidate(candidate, *duplicate_identity)) {
        *duplicate_identity = candidate;
      }
      continue;
    }

    maps.push_back(candidate);
  }
}

} // namespace

std::vector<MapDefinition>
Map::DiscoverAvailableMaps(const std::string &directory_path) {
  std::vector<MapDefinition> maps;
  if (!directory_path.empty()) {
    DiscoverMapsInDirectory(fs::path(directory_path), maps);
  } else {
    for (const std::string &search_directory : Paths::GetMapSearchDirectories()) {
      DiscoverMapsInDirectory(fs::path(search_directory), maps);
    }
  }

  std::sort(maps.begin(), maps.end(),
            [](const MapDefinition &left, const MapDefinition &right) {
              if (left.display_name == right.display_name) {
                return left.file_path < right.file_path;
              }
              return left.display_name < right.display_name;
            });

  return maps;
}

bool Map::LoadMapFile(const std::string &map_path, MapFileData &map_file) {
  return ReadMapFileContents(map_path, map_file);
}

bool Map::SaveMapFile(const std::string &map_path, const MapFileData &map_file) {
  std::ofstream stream(map_path);
  if (!stream) {
    return false;
  }

  stream << map_file.display_name << "\n";
  for (const auto &row : map_file.layout_rows) {
    stream << row << "\n";
  }

  return stream.good();
}

bool Map::IsMonsterChar(char inp) {
  return inp == MONSTER_FEW || inp == MONSTER_MEDIUM ||
         inp == MONSTER_MANY || inp == MONSTER_EXTRA || inp == GOAT;
}

bool Map::IsTeleporterChar(char inp) {
  return inp >= '1' && inp <= '5';
}

/**
 * @brief convert char entry from ascii file to struct type
 *
 * @param inp
 * @return ElementType
 */
ElementType Map::Char2Type(char inp) {
  switch (inp) {
  case BRICK:
    return ElementType::TYPE_WALL;
  case PATH:
    return ElementType::TYPE_PATH;
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
    return ElementType::TYPE_TELEPORTER;
  case GOODIE:
    return ElementType::TYPE_GOODIE;
  case PACMAN:
    return ElementType::TYPE_PACMAN;
  case MONSTER_FEW:
  case MONSTER_MEDIUM:
  case MONSTER_MANY:
  case MONSTER_EXTRA:
  case GOAT:
    return ElementType::TYPE_MONSTER;
  default:
    return ElementType::TYPE_WALL;
  }
}

/**
 * @brief converts struct type to char
 *
 * @param inp
 * @return char
 */
char Map::Type2Char(ElementType inp) {
  switch (inp) {
  case ElementType::TYPE_WALL:
    return BRICK;
  case ElementType::TYPE_PATH:
  case ElementType::TYPE_CRATER:
    return PATH;
  case ElementType::TYPE_TELEPORTER:
    return '1';
  case ElementType::TYPE_GOODIE:
    return GOODIE;
  case ElementType::TYPE_MONSTER:
    return MONSTER_FEW;
  case ElementType::TYPE_PACMAN:
    return PACMAN;
  default:
    return BRICK;
  }
}

/**
 * @brief Construct a new Map:: Map object
 *
 * @param _map_path path to the ascii map file
 */
Map::Map(const std::string &_map_path) : map_path(_map_path) {
  LoadMap(map_path);
}

Map::~Map() {}

/**
 * @brief Loads the Map from the text file into the map 2D vector of the Map
 * class. First line contains the display name, following lines contain the
 * grid.
 */
void Map::LoadMap(const std::string mappath) {
  MapFileData map_file;
  if (!LoadMapFile(mappath, map_file)) {
    std::cerr << "File not found: " << mappath << "\n";
    exit(1);
  }

  map = std::make_shared<vector<vector<ElementType>>>();
  monster_coord.clear();
  monster_chars.clear();
  goodie_coord.clear();
  teleporter_pairs.clear();
  pacman_coord = {0, 0};
  map_name = map_file.display_name;
  const std::vector<std::string> &map_lines = map_file.layout_rows;
  std::array<std::vector<MapCoord>, 5> teleporter_slots;

  if (map_lines.empty()) {
    std::cerr << "Map file contains no layout rows: " << mappath << "\n";
    exit(1);
  }

  int max_length = 0;
  bool border_necessary = false;
  bool border_necessary_bottom = false;
  int rows = 0;
  MapCoord temp_coord;

  for (const auto &map_line : map_lines) {
    if (rows == 0) {
      border_necessary =
          std::count(map_line.begin(), map_line.end(), BRICK) !=
          map_line.length();
    }

    max_length =
        (static_cast<int>(map_line.length()) > max_length)
            ? static_cast<int>(map_line.length())
            : max_length;

    border_necessary = border_necessary || map_line.front() != BRICK ||
                       map_line.back() != BRICK;
    border_necessary_bottom =
        std::count(map_line.begin(), map_line.end(), BRICK) !=
        map_line.length();
    rows++;
  }

  border_necessary = border_necessary || border_necessary_bottom;
  rows = border_necessary ? rows + 2 : rows;
  (*map).resize(rows);

  if (border_necessary) {
    for (int j = 0; j < max_length + 2; j++) {
      (*map)[0].push_back(ElementType::TYPE_WALL);
      (*map)[rows - 1].push_back(ElementType::TYPE_WALL);
    }
  }

  int i = border_necessary ? 1 : 0;
  for (const auto &map_line : map_lines) {
    if (border_necessary) {
      (*map)[i].push_back(ElementType::TYPE_WALL);
    }

    for (int j = 0; j < max_length; j++) {
      const char map_char =
          (j < static_cast<int>(map_line.length())) ? map_line[j] : BRICK;
      const ElementType entry = Char2Type(map_char);
      const int column_index = border_necessary ? j + 1 : j;
      (*map)[i].push_back(entry);

      if (entry == ElementType::TYPE_MONSTER) {
        temp_coord.u = i;
        temp_coord.v = column_index;
        monster_coord.push_back(temp_coord);
        monster_chars.push_back(map_char);
      }
      if (entry == ElementType::TYPE_GOODIE) {
        temp_coord.u = i;
        temp_coord.v = column_index;
        goodie_coord.push_back(temp_coord);
      }
      if (entry == ElementType::TYPE_PACMAN) {
        temp_coord.u = i;
        temp_coord.v = column_index;
        pacman_coord = temp_coord;
      }
      if (IsTeleporterChar(map_char)) {
        temp_coord.u = i;
        temp_coord.v = column_index;
        teleporter_slots[map_char - '1'].push_back(temp_coord);
      }
    }

    if (border_necessary) {
      (*map)[i].push_back(ElementType::TYPE_WALL);
    }
    i++;
  }

  for (size_t teleporter_index = 0; teleporter_index < teleporter_slots.size();
       teleporter_index++) {
    const char digit = static_cast<char>('1' + teleporter_index);
    const size_t slot_count = teleporter_slots[teleporter_index].size();
    if (slot_count != 0 && slot_count != 2) {
      std::cerr << "Invalid teleporter definition in " << mappath
                << ": digit " << digit
                << " must appear exactly 0 or 2 times.\n";
      exit(1);
    }
    if (slot_count == 2) {
      teleporter_pairs.push_back({digit, teleporter_slots[teleporter_index][0],
                                  teleporter_slots[teleporter_index][1]});
    }
  }
}

size_t Map::get_map_rows() { return (this->map.get()->size()); };
size_t Map::get_map_cols() { return (this->map.get()->at(0).size()); };
int Map::get_number_monsters() { return this->monster_coord.size(); }
int Map::get_number_goodies() { return this->goodie_coord.size(); }

ElementType Map::map_entry(size_t row, size_t col) const {
  if (map == nullptr || row >= map->size()) {
    return ElementType::TYPE_WALL;
  }

  const auto &map_row = map->at(row);
  if (col >= map_row.size()) {
    return ElementType::TYPE_WALL;
  }

  return map_row[col];
};

MapCoord Map::get_coord_monster(int i) { return this->monster_coord[i]; }
char Map::get_char_monster(int i) { return this->monster_chars[i]; }
MapCoord Map::get_coord_goodie(int i) { return this->goodie_coord[i]; }
MapCoord Map::get_coord_pacman() { return this->pacman_coord; }

std::string Map::get_map_name() const { return map_name; }

const std::vector<TeleporterPair> &Map::get_teleporter_pairs() const {
  return teleporter_pairs;
}

bool Map::TryGetTeleporterDestination(MapCoord origin, MapCoord &destination,
                                      char &digit) const {
  for (const auto &teleporter_pair : teleporter_pairs) {
    if (teleporter_pair.first.u == origin.u &&
        teleporter_pair.first.v == origin.v) {
      destination = teleporter_pair.second;
      digit = teleporter_pair.digit;
      return true;
    }
    if (teleporter_pair.second.u == origin.u &&
        teleporter_pair.second.v == origin.v) {
      destination = teleporter_pair.first;
      digit = teleporter_pair.digit;
      return true;
    }
  }
  digit = '\0';
  return false;
}

bool Map::SetEntry(MapCoord coord, ElementType entry) {
  if (!IsInsideBounds(coord)) {
    return false;
  }

  (*map)[static_cast<size_t>(coord.u)][static_cast<size_t>(coord.v)] = entry;
  return true;
}

bool Map::IsInsideBounds(MapCoord coord) const {
  if (coord.u < 0 || coord.v < 0 || map == nullptr) {
    return false;
  }

  const size_t row = static_cast<size_t>(coord.u);
  if (row >= map->size()) {
    return false;
  }

  return static_cast<size_t>(coord.v) < map->at(row).size();
}

void Map::get_options(MapCoord in_coord, std::vector<Directions> &options) {
  options.clear();
  if (!IsInsideBounds(in_coord)) {
    return;
  }

  if (in_coord.u > 0 &&
      !IsBlockingMapElement(map_entry(static_cast<size_t>(in_coord.u - 1),
                                      static_cast<size_t>(in_coord.v)))) {
    options.emplace_back(Directions::Up);
  }
  if (in_coord.u + 1 < static_cast<int>(get_map_rows()) &&
      !IsBlockingMapElement(map_entry(static_cast<size_t>(in_coord.u + 1),
                                      static_cast<size_t>(in_coord.v)))) {
    options.emplace_back(Directions::Down);
  }
  if (in_coord.v > 0 &&
      !IsBlockingMapElement(map_entry(static_cast<size_t>(in_coord.u),
                                      static_cast<size_t>(in_coord.v - 1)))) {
    options.emplace_back(Directions::Left);
  }
  if (in_coord.v + 1 < static_cast<int>(get_map_cols()) &&
      !IsBlockingMapElement(map_entry(static_cast<size_t>(in_coord.u),
                                      static_cast<size_t>(in_coord.v + 1)))) {
    options.emplace_back(Directions::Right);
  }
}
