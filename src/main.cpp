#include <SDL.h>
#include <iostream>
#include <thread>

#include "events.h"
#include "game.h"
#include "map.h"
#include "mapelement.h"
#include "monster.h"
#include "pacman.h"
#include "renderer.h"
#include "audio.h"

/* TODO
 Spielfeld um Text verkleinern
 Kommentieren
 Funktionen in CamelCaps
*/

int main() {
  Map *map = new Map();                 // map loading, map interfaces
  Events *events = new Events();        // SDL event handling
  Audio *audio = new Audio();                         // Audio effects
  Game *game = new Game(map, events, audio); // game logic
  Renderer renderer(map, game);         // for rendering the screen

  while (!events->is_quit()) {
    events->update();
    if (!game->is_lost() && !game->is_won()) {
      game->Update();    // check for updates/collisions
      renderer.Render(); // update screen
      // something around 25 fps should be sufficient, so sleep for 40ms to keep
      // CPU load low
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
  }
  std::cout << "Quitting app.\n";
  delete game;
  delete events;
  delete map;
}