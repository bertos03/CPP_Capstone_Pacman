/*
 * PROJEKTKOMMENTAR (events.h)
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

#ifndef EVENTS_H
#define EVENTS_H

#include <SDL.h>
#include <iostream>

#include "globaltypes.h"

class Events {
public:
  Events();
  ~Events();
  void update();
  void Keyreset();
  bool is_quit() { return quit; }
  Directions get_next_move(){return current_direction;}

protected:
private:
  SDL_Event *sdl_events;
  bool quit;
  Directions current_direction;
};

#endif