#include "globaltypes.h"

/**
 * @brief A wrapper function for better readability
 * 
 * @param milliseconds 
 */
void sleep(int milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}