/*
 * PROJEKTKOMMENTAR (pacman.h)
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

#ifndef PACMAN_H
#define PACMAN_H

#include <iostream>
#include <mutex>
#include <vector>

#include "mapelement.h"
#include "map.h"
#include "renderer.h"
#include "globaltypes.h"


class Map;
class Renderer;

/**
 * @brief The class for Pacman
 *
 */

class Pacman : public MapElement {
public:
  Pacman(MapCoord);
  ~Pacman();
  void simulate( Events *,Map *);

protected:
private:
  bool is_locked;
  Map *map;
  friend class Renderer;
  friend class Game;
};

#endif