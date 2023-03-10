#ifndef AUDIO_H
#define AUDIO_H

#include "definitions.h"
#include <SDL.h>
#ifdef AUDIO
#include <SDL_mixer.h>
#endif
#include <iostream>
#include <thread>
#include <vector>

class Audio {

public:
  Audio();
  ~Audio();
#ifdef AUDIO
  void PlayCoin();
  void PlayGameOver();
  void PlayWin();

protected:
private:
  Mix_Chunk *SFX_coin;
  Mix_Chunk *SFX_win;
  Mix_Chunk *SFX_gameover;
  std::vector<std::thread> audio_threads;
#endif
};

#endif