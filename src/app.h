/*
 * PROJECT COMMENT (app.h)
 * ---------------------------------------------------------------------------
 * This header exposes the high-level application entry object used by
 * `main.cpp`. The intent is to keep the process entry point tiny while the
 * menu flow, editor session, and gameplay bootstrap live in `app.cpp`.
 */

#ifndef APP_H
#define APP_H

class BobManApp {
public:
  int Run();
};

#endif
