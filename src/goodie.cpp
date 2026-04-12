/*
 * PROJECT COMMENT (goodie.cpp)
 * ---------------------------------------------------------------------------
 * This file is part of "BobMan", an SDL2 game inspired by Pac-Man.
 * The code in this unit has a clear responsibility so newcomers can quickly
 * understand the architecture: data model (map, elements), runtime logic
 * (game, events), rendering (renderer), and optional audio output.
 *
 * Important notes for newcomers:
 * - Header files declare classes, methods, and data types.
 * - CPP files contain the concrete implementation of the logic.
 * - Multiple threads move game entities in parallel, so shared data is read
 *   and written in a controlled way.
 * - Macros in definitions.h control resource paths, colors, and features.
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
