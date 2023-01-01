#include "pacman.h"

/**
 * @brief Construct a new Pacman:: Pacman object
 * 
 * @param _coord map coordinates for placing pacman
 */
Pacman::Pacman(MapCoord _coord) {
  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Creating Pacman\n";
  lck.unlock();
  map_coord = _coord;
};

/**
 * @brief Destroy the Pacman:: Pacman object
 * 
 */
Pacman::~Pacman() {
  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Pacman is being destroyed \n";
  lck.unlock();
};

/**
 * @brief The simulation loop for pacman
 * 
 * @param events Events object to read keyboard inputs
 * @param map  Map object to check the possible options for pacman
 */
void Pacman::simulate(Events *events, Map *map) {
  std::vector<Directions> options;
  bool valid_choice = false;
  Directions next_move;

  std::unique_lock<std::mutex> lck(mtx);
  // std::cout << "Simulating Pacman\n";
  lck.unlock();

  while (!events->is_quit()) {
    next_move = events->get_next_move();
    map->get_options(map_coord, options);
    events->Keyreset();
    sleep(5);
    for (auto i : options) {
      if (i == next_move) {
        switch (next_move) {
        case Directions::Up:
          for (int i = 0; i > -100; i--) {
            px_delta.y = i;
            sleep(10 - SPEED_PACMAN);
          }
          px_delta.y = 0;
          map_coord.u--;
          break;
        case Directions::Down:
          for (int i = 0; i < 100; i++) {
            px_delta.y = i;
            sleep(10 - SPEED_PACMAN);
          }
          px_delta.y = 0;
          map_coord.u++;
          break;
        case Directions::Left:
          for (int i = 0; i > -100; i--) {
            px_delta.x = i;
            sleep(10 - SPEED_PACMAN);
          }
          px_delta.x = 0;
          map_coord.v--;
          break;
        case Directions::Right:
          for (int i = 0; i < 100; i++) {
            px_delta.x = i;
            sleep(10 - SPEED_PACMAN);
          }
          px_delta.x = 0;
          map_coord.v++;
        default:
          break;
        };
      }
    }
  }
}