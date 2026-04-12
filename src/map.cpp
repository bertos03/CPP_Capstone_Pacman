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
    break;
  case PATH:
    return ElementType::TYPE_PATH;
    break;
  case GOODIE:
    return ElementType::TYPE_GOODIE;
    break;
  case PACMAN:
    return ElementType::TYPE_PACMAN;
    break;
  case MONSTER_FEW:
  case MONSTER_MEDIUM:
  case MONSTER_MANY:
    return IsMonsterEnabled(inp) ? ElementType::TYPE_MONSTER
                                 : ElementType::TYPE_PATH;
    break;
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
 * @param mappath: accepts a (valid) path with the ascii file containing the map
 * data
 */
Map::Map(MonsterAmount _monster_amount) : monster_amount(_monster_amount) {
  LoadMap(MAP_PATH);
}

Map::~Map() {}

/**
 * @brief Loads the Map from the text file into the map 2D vector of the Map
 * class. Uses hard-coded MAP_PATH. (TODO: accept as input argument from user)
 */
void Map::LoadMap(const std::string mappath) {
  string line;
  int max_length(0);
  bool border_necessary(false);
  bool border_necessary_bottom;
  int rows(0);
  MapCoord temp_coord;
  map = std::make_shared<vector<vector<ElementType>>>();
  monster_coord.clear();
  goodie_coord.clear();
  pacman_coord = {0, 0};
  // TODO: Copy constructor!

  // check if file exists
  std::ifstream stream(mappath);
  if (!stream) {
    std::cerr << "File not found.\n";
    exit(1);
  }

  // check line length consistency and if map is closed
  while (std::getline(stream, line)) {
    // Check first line for closed wall
    if (rows == 0) {
      border_necessary =
          std::count(line.begin(), line.end(), BRICK) != line.length();
    }

    // Check for width of Map in case the file is not constructed well
    max_length = (line.length() > max_length) ? line.length() : max_length;

    // Check side wall if closed
    border_necessary = border_necessary || line[0] != BRICK ||
                       line.at(line.length() - 1) != BRICK;
    // Check every line so at end of the loop the evaluation for the bottom is
    // stored
    border_necessary_bottom =
        std::count(line.begin(), line.end(), BRICK) != line.length();

    rows++;
  }
  border_necessary = border_necessary || border_necessary_bottom;
  rows = border_necessary ? rows + 2 : rows;
  // data check finished - starting construction of 2D vector

  stream.clear();
  stream.seekg(0, std::ios::beg);
  (*map).resize(rows);

  // Top border
  if (border_necessary) {
    for (int j = 0; j < max_length + 2; j++) {
      (*map)[0].push_back(ElementType::TYPE_WALL);
      (*map)[rows - 1].push_back(ElementType::TYPE_WALL);
    }
  }
  int i(border_necessary ? 1 : 0);
  while (std::getline(stream, line)) {
    if (border_necessary) {
      (*map)[i].push_back(ElementType::TYPE_WALL);
    }

    for (int j = 0; j < max_length; j++) {
      const char map_char =
          (j < static_cast<int>(line.length())) ? line[j] : BRICK;
      const ElementType entry = Char2Type(map_char);
      (*map)[i].push_back(entry);

      if (entry == ElementType::TYPE_MONSTER) {
        temp_coord.u = i;
        temp_coord.v = j;
        monster_coord.push_back(temp_coord);
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
MapCoord Map::get_coord_goodie(int i) { return this->goodie_coord[i]; }
MapCoord Map::get_coord_pacman() { return this->pacman_coord; }

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
