#ifndef MAP_H
#define MAP_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "definitions.h"
#include "globaltypes.h"
#include "renderer.h"

using std::string;
using std::vector;

class Renderer;

enum class ElementType {
  TYPE_WALL,
  TYPE_PATH,
  TYPE_GOODIE,
  TYPE_PACMAN,
  TYPE_MONSTER
};

class Map {
public:
  Map();
  ~Map();
  size_t get_map_rows();
  size_t get_map_cols();
  ElementType map_entry(size_t, size_t);
  ElementType Char2Type(char);
  char Type2Char(ElementType);
  int get_number_monsters();
  int get_number_goodies();
  MapCoord get_coord_monster(int);
  MapCoord get_coord_goodie(int);
  MapCoord get_coord_pacman();
  void get_options(MapCoord, std::vector<Directions>&);

protected:
private:
  void LoadMap(const std::string);
  std::vector<MapCoord> monster_coord;
  std::vector<MapCoord> goodie_coord;
  MapCoord pacman_coord;
  std::shared_ptr<vector<vector<ElementType>>> map;
  friend class Renderer;
};

#endif