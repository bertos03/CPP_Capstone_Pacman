/*
 * PROJEKTKOMMENTAR (audio.h)
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