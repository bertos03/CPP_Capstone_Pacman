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