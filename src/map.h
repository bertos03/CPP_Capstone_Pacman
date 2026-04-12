/*
 * PROJEKTKOMMENTAR (map.h)
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

#ifndef MAP_H
#define MAP_H

#include <fstream>
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
  std::string file_path;
  std::string display_name;
};

enum class ElementType {
  TYPE_WALL,
  TYPE_PATH,
  TYPE_GOODIE,
  TYPE_PACMAN,
  TYPE_MONSTER
};

class Map {
public:
  Map(const std::string &map_path,
      MonsterAmount monster_amount = MonsterAmount::Many);
  ~Map();
  static std::vector<MapDefinition>
  DiscoverAvailableMaps(const std::string &directory_path = MAPS_DIRECTORY_PATH);
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
  std::string get_map_name() const;
  void get_options(MapCoord, std::vector<Directions>&);

protected:
private:
  void LoadMap(const std::string);
  bool IsMonsterChar(char);
  bool IsMonsterEnabled(char);
  std::vector<MapCoord> monster_coord;
  std::vector<MapCoord> goodie_coord;
  MapCoord pacman_coord;
  std::string map_name;
  std::string map_path;
  MonsterAmount monster_amount;
  std::shared_ptr<vector<vector<ElementType>>> map;
  friend class Renderer;
};

#endif
