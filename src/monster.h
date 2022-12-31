#ifndef MONSTER_H
#define MONSTER_H

#include <mutex>
#include <thread>
#include <iostream>
#include <vector>

#include "mapelement.h"
#include "events.h"
#include "map.h"
#include "globaltypes.h"
#include "renderer.h"

struct MapCoord;
class Map;

class Monster : public MapElement {
public:
  Monster(MapCoord, int);
  ~Monster();
  void simulate(Events *, Map *);

protected:
private:
std::vector<Directions> options;

friend class Renderer;
friend class Game;
};

#endif