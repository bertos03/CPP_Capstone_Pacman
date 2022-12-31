#ifndef PACMAN_H
#define PACMAN_H

#include <iostream>
#include <mutex>
#include <vector>

#include "mapelement.h"
#include "map.h"
#include "renderer.h"
#include "globaltypes.h"


class Map;
class Renderer;

/**
 * @brief The class for Pacman
 *
 */

class Pacman : public MapElement {
public:
  Pacman(MapCoord);
  ~Pacman();
  void simulate( Events *,Map *);

protected:
private:
  bool is_locked;
  Map *map;
  friend class Renderer;
  friend class Game;
};

#endif