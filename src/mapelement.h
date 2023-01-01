#ifndef MAPELEMENT_H
#define MAPELEMENT_H

#include "globaltypes.h"
#include <mutex>

/**
 * @brief Defines the base class for dynamic elements (pacman, monsters,
 * goodies) such as Pacman, the monsters and goodies.
 *
 */
class MapElement {
public:
  /* since all derived classes have own constructors/destructors, no definition
   * of those for base class */
protected:
  PixelCoord px_coord; // Coordinates in pixelmap
  MapCoord map_coord;  // coordinates in logical map
  PixelCoord px_delta; // percentage(!) of derivation (0-100) from position
                       // (100: 1 element-size)

  bool is_active = true;
  bool locked = false;
  static std::mutex mtx;
  int id;

private:
};

#endif