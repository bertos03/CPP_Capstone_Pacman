/*
 * PROJEKTKOMMENTAR (globaltypes.h)
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

#ifndef GLOBALTYPES_H
#define GLOBALTYPES_H

#include <thread>

extern void sleep(int);

struct PixelCoord {
  int x;
  int y;
};

struct MapCoord {
  int u;
  int v;
};

enum class Directions { None, Up, Down, Left, Right };
enum class MonsterAmount { Few, Medium, Many };


// struct DirOptions{
//   bool up = false;
//   bool down = false;
//   bool left = false;
//   bool right = false;
// };



#endif
