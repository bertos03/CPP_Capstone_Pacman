/*
 * PROJEKTKOMMENTAR (goodie.h)
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

#ifndef GOODIE_H
#define GOODIE_H

#include "mapelement.h"
#include "map.h"
#include "globaltypes.h"

struct MapCoord;

/**
 * @brief The class for goodies
 *
 */
class Goodie : public MapElement {
public:
  Goodie(MapCoord, int);
  ~Goodie();
  void Deactivate();

protected:
private:
friend class Renderer;
friend class Game;
};

#endif