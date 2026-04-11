/*
 * PROJEKTKOMMENTAR (goodie.cpp)
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

#include "goodie.h"

/**
 * @brief Construct a new Goodie:: Goodie object
 * 
 * @param _coord the map coordinates for placing the monster
 * @param _id A unique "goodie ID" (currently not used)
 */
Goodie::Goodie(MapCoord _coord, int _id)  {
  id=_id;
  // std::unique_lock <std::mutex> lck (mtx);
  // std::cout << "Creating goodie #" << id << "\n";
  // lck.unlock();
  map_coord = _coord;
}

/**
 * @brief Destroy the Goodie:: Goodie object
 * 
 */
Goodie::~Goodie() {
    std::unique_lock <std::mutex> lck (mtx);
    // std::cout << "Goodie #" << id << " is being destroyed \n";
    lck.unlock();
}

/**
 * @brief Goodie gets deactivated once it is collected
 * 
 */
void Goodie::Deactivate(){
  is_active = false;
}