#ifndef GOODIE_H
#define GOODIE_H

#include "mapelement.h"
#include "map.h"
#include "globaltypes.h"

struct MapCoord;

/**
 * @brief The class for goodies
 *
 */
class Goodie : public MapElement {
public:
  Goodie(MapCoord, int);
  ~Goodie();
  void Deactivate();

protected:
private:
friend class Renderer;
friend class Game;
};

#endif