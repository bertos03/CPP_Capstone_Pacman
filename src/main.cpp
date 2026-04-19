/*
 * PROJECT COMMENT (main.cpp)
 * ---------------------------------------------------------------------------
 * This file is intentionally tiny: it is only the process entry point and
 * hands control to the application bootstrap object.
 */

#include "app.h"

int main() {
  BobManApp app;
  return app.Run();
}
