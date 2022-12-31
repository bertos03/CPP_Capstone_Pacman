#include "goodie.h"

Goodie::Goodie(MapCoord _coord, int _id)  {
  id=_id;
  // std::unique_lock <std::mutex> lck (mtx);
  // std::cout << "Creating goodie #" << id << "\n";
  // lck.unlock();
  map_coord = _coord;
}

Goodie::~Goodie() {
    std::unique_lock <std::mutex> lck (mtx);
    // std::cout << "Goodie #" << id << " is being destroyed \n";
    lck.unlock();
}

void Goodie::Deactivate(){
  is_active = false;
}