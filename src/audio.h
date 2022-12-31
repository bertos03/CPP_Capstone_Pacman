#ifndef AUDIO_H
#define AUDIO_H
#include <SDL.h>
#include <SDL_mixer.h>
#include <thread>
#include <vector>

class Audio {
public:
  Audio();
  ~Audio();
  void PlayCoin();
  void PlayGameOver();
  void PlayWin();

protected:
private:
  Mix_Chunk *SFX_coin;
  Mix_Chunk *SFX_win;
  Mix_Chunk *SFX_gameover;
  std::vector<std::thread> audio_threads;
};

#endif