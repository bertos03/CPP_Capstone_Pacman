/*
 * PROJEKTKOMMENTAR (main.cpp)
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

#include <SDL.h>
#include <iostream>
#include <memory>
#include <thread>

#include "audio.h"
#include "events.h"
#include "game.h"
#include "map.h"
#include "mapelement.h"
#include "monster.h"
#include "pacman.h"
#include "renderer.h"
#include "globaltypes.h"

/* TODO
 Spielfeld um Text verkleinern
 Kommentieren
 Funktionen in CamelCaps
*/

int main() {
  std::cout << "Pacman starting up ...\n";
  std::shared_ptr<Map> map(new Map());          // map loading, map interfaces
  std::shared_ptr<Events> events(new Events()); // SDL event handling
  std::shared_ptr<Audio> audio(new Audio());    // Audio effects
  std::shared_ptr<Game> game(
      new Game(map.get(), events.get(), audio.get())); // game logic
  Renderer renderer(map.get(), game.get()); // for rendering the screen

  while (!events.get()->is_quit()) {
    events.get()->update();
    if (!game.get()->is_lost() && !game.get()->is_won()) {
      game.get()->Update();    // check for updates/collisions
      renderer.Render(); // update screen
      // something around 25 fps should be sufficient, so sleep for 40ms to keep
      // CPU load low
      sleep(40);
    }
  }
  std::cout << "Quitting app.\n";
}