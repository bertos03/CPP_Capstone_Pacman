/*
 * PROJEKTKOMMENTAR (monster.h)
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

#ifndef MONSTER_H
#define MONSTER_H

#include <SDL.h>

#include <mutex>
#include <thread>
#include <iostream>
#include <vector>

#include "mapelement.h"
#include "events.h"
#include "map.h"
#include "globaltypes.h"
#include "renderer.h"

struct MapCoord;
class Map;

class Monster : public MapElement {
public:
  Monster(MapCoord, int, char);
  ~Monster();
  void simulate(Events *, Map *);

protected:
private:
std::vector<Directions> options;
char monster_char;
bool is_alive;
Uint32 last_fireball_ticks;
Uint32 next_gas_cloud_ticks;
Uint32 death_started_ticks;
MapCoord death_coord;

friend class Renderer;
friend class Game;
};

#endif
