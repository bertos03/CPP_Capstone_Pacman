/*
 * PROJEKTKOMMENTAR (map.cpp)
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

#include "map.h"

#include <filesystem>

namespace {

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

} // namespace

std::vector<MapDefinition>
Map::DiscoverAvailableMaps(const std::string &directory_path) {
  namespace fs = std::filesystem;

  std::vector<MapDefinition> maps;
  const fs::path map_directory(directory_path);
  if (!fs::exists(map_directory) || !fs::is_directory(map_directory)) {
    return maps;
  }

  for (const auto &entry : fs::directory_iterator(map_directory)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const std::string extension = entry.path().extension().string();
    if (extension != ".txt" && extension != ".map") {
      continue;
    }

    MapFileData map_file;
    const std::string display_name =
        ReadMapFileContents(entry.path().string(), map_file)
            ? map_file.display_name
            : entry.path().stem().string();
    maps.push_back({entry.path().string(), display_name});
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
  return inp == MONSTER_FEW || inp == MONSTER_MEDIUM || inp == MONSTER_MANY;
}

bool Map::IsMonsterEnabled(char inp) {
  if (!IsMonsterChar(inp)) {
    return false;
  }

  switch (monster_amount) {
  case MonsterAmount::Few:
    return inp == MONSTER_FEW;
  case MonsterAmount::Medium:
    return inp == MONSTER_FEW || inp == MONSTER_MEDIUM;
  case MonsterAmount::Many:
    return true;
  default:
    return false;
  }
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
  case GOODIE:
    return ElementType::TYPE_GOODIE;
  case PACMAN:
    return ElementType::TYPE_PACMAN;
  case MONSTER_FEW:
  case MONSTER_MEDIUM:
  case MONSTER_MANY:
    return IsMonsterEnabled(inp) ? ElementType::TYPE_MONSTER
                                 : ElementType::TYPE_PATH;
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
    return PATH;
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
 * @param _monster_amount selected monster amount setting
 */
Map::Map(const std::string &_map_path, MonsterAmount _monster_amount)
    : map_path(_map_path), monster_amount(_monster_amount) {
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
  pacman_coord = {0, 0};
  map_name = map_file.display_name;
  const std::vector<std::string> &map_lines = map_file.layout_rows;

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
      (*map)[i].push_back(entry);

      if (entry == ElementType::TYPE_MONSTER) {
        temp_coord.u = i;
        temp_coord.v = j;
        monster_coord.push_back(temp_coord);
        monster_chars.push_back(map_char);
      }
      if (entry == ElementType::TYPE_GOODIE) {
        temp_coord.u = i;
        temp_coord.v = j;
        goodie_coord.push_back(temp_coord);
      }
      if (entry == ElementType::TYPE_PACMAN) {
        temp_coord.u = i;
        temp_coord.v = j;
        pacman_coord = temp_coord;
      }
    }

    if (border_necessary) {
      (*map)[i].push_back(ElementType::TYPE_WALL);
    }
    i++;
  }
}

size_t Map::get_map_rows() { return (this->map.get()->size()); };
size_t Map::get_map_cols() { return (this->map.get()->at(0).size()); };
int Map::get_number_monsters() { return this->monster_coord.size(); }
int Map::get_number_goodies() { return this->goodie_coord.size(); }

ElementType Map::map_entry(size_t row, size_t col) {
  return (this->map.get()->at(row).at(col));
};

MapCoord Map::get_coord_monster(int i) { return this->monster_coord[i]; }
char Map::get_char_monster(int i) { return this->monster_chars[i]; }
MapCoord Map::get_coord_goodie(int i) { return this->goodie_coord[i]; }
MapCoord Map::get_coord_pacman() { return this->pacman_coord; }

std::string Map::get_map_name() const { return map_name; }

void Map::get_options(MapCoord in_coord, std::vector<Directions> &options) {
  options.clear();
  if (in_coord.u != 0) {
    if (map.get()->at(in_coord.u - 1).at(in_coord.v) != ElementType::TYPE_WALL) {
      options.emplace_back(Directions::Up);
    }
  }
  if (in_coord.u != get_map_rows()) {
    if (map.get()->at(in_coord.u + 1).at(in_coord.v) != ElementType::TYPE_WALL) {
      options.emplace_back(Directions::Down);
    }
  }
  if (in_coord.v != 0) {
    if (map.get()->at(in_coord.u).at(in_coord.v - 1) != ElementType::TYPE_WALL) {
      options.emplace_back(Directions::Left);
    }
  }
  if (in_coord.v != get_map_cols()) {
    if (map.get()->at(in_coord.u).at(in_coord.v + 1) != ElementType::TYPE_WALL) {
      options.emplace_back(Directions::Right);
    }
  }
  return;
}
