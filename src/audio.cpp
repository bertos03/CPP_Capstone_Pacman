/*
 * PROJEKTKOMMENTAR (audio.cpp)
 * ---------------------------------------------------------------------------
 * Diese Datei ist Teil von "BobMan", einem Pacman-inspirierten SDL2-Spiel.
 * Der Code in dieser Einheit kapselt einen klaren Verantwortungsbereich, damit
 * Einsteiger die Architektur schnell verstehen: Datenmodell (Map, Elemente),
 * Laufzeitlogik (Game, Events), Darstellung (Renderer) und optionale Audio-
 * Ausgabe.
 *
 * Wichtige Hinweise fuer Newbies:
 * - Header-Dateien deklarieren Klassen, Methoden und Datentypen.
 * - CPP-Dateien enthalten die konkrete Implementierung der Logik.
 * - Mehrere Threads bewegen Spielfiguren parallel; gemeinsame Daten werden
 *   deshalb kontrolliert gelesen/geschrieben.
 * - Makros in definitions.h steuern Ressourcenpfade, Farben und Features.
 */

#include "audio.h"

/**
 * @brief Construct a new Audio:: Audio object
 * Takes care of all sound effects
 */
Audio::Audio() {
  #ifndef AUDIO
  std::cout << "No Audio\n";
  #endif
  #ifdef AUDIO
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
    exit(1);
  }
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n",
           Mix_GetError());
    exit(1);
  }

  SFX_coin = Mix_LoadWAV(COIN_SOUND_PATH);
  SFX_win = Mix_LoadWAV(WIN_SOUND_PATH);
  SFX_gameover = Mix_LoadWAV(GAMEOVER_SOUND_PATH);
  // Mix_PlayChannel(-1, SFX_coin, 0);
  if (SFX_coin == NULL || SFX_win == NULL | SFX_gameover == NULL) {
    printf("Failed to load SFX ... SDL_mixer Error: %s\n", Mix_GetError());
    exit(1);
  }
  #endif
};

/**
 * @brief Destroy the Audio:: Audio object
 * 
 */
Audio::~Audio() {
  #ifndef AUDIO
  // std::cout << "No Audio\n";
  #endif
  #ifdef AUDIO
  for (int i =0; i< audio_threads.size();i++) {
    audio_threads[i].join();
  }
  audio_threads.clear();
  
  
  Mix_FreeChunk(SFX_coin);
  Mix_FreeChunk(SFX_win);
  Mix_FreeChunk(SFX_gameover);
  Mix_Quit();
  #endif
};

#ifdef AUDIO

/**
 * @brief as the name says
 * 
 */
void Audio::PlayCoin() {
  audio_threads.emplace_back(
      std::thread([&]() { Mix_PlayChannel(-1, SFX_coin, 0); }));
};

/**
 * @brief as the name says
 * 
 */
void Audio::PlayGameOver(){
  audio_threads.emplace_back(
      std::thread([&]() { Mix_PlayChannel(-1, SFX_gameover, 0); }));
};

/**
 * @brief as the name says
 * 
 */
void Audio::PlayWin(){
  audio_threads.emplace_back(
      std::thread([&]() { Mix_PlayChannel(-1, SFX_win, 0); }));
};
#endif