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
#include <vector>

class Audio {

public:
  Audio();
  ~Audio();
#ifdef AUDIO
  void PlayCoin();
  void PlayGameOver();
  void PlayWin();
  void PlayMenuMove();
  void PlayMenuSelect();
  void PlayCountdownTick();
  void PlayStartTrumpet();
  void PlayMonsterShot();
  void PlayFireballWallHit();
  void PlayMonsterExplosion();
  void PlayMonsterFart();
  void PlayPacmanGag();
  void PlayTeleporterZap();
  void PlayTeleporterArc();
  void PlayEditorBlocked();

protected:
private:
  Mix_Chunk *CreateSynthChunk(const std::vector<int> &frequencies,
                              int duration_ms, double volume, double attack_ms,
                              double release_ms);
  Mix_Chunk *CreateTrumpetChunk();
  Mix_Chunk *CreateViolinLamentChunk();
  Mix_Chunk *CreateMonsterExplosionChunk();
  Mix_Chunk *CreateMonsterFartChunk();
  Mix_Chunk *CreatePacmanGagChunk();
  Mix_Chunk *CreateTeleporterZapChunk();
  Mix_Chunk *CreateTeleporterArcChunk();
  Mix_Chunk *CreateEditorBlockedChunk();
  void PlayChunk(Mix_Chunk *);

  Mix_Chunk *SFX_coin;
  Mix_Chunk *SFX_win;
  Mix_Chunk *SFX_gameover;
  Mix_Chunk *SFX_menu_move;
  Mix_Chunk *SFX_menu_select;
  Mix_Chunk *SFX_countdown_tick;
  Mix_Chunk *SFX_start_trumpet;
  Mix_Chunk *SFX_monster_shot;
  Mix_Chunk *SFX_fireball_wall_hit;
  Mix_Chunk *SFX_monster_explosion;
  Mix_Chunk *SFX_monster_fart;
  Mix_Chunk *SFX_pacman_gag;
  Mix_Chunk *SFX_teleporter_zap;
  Mix_Chunk *SFX_teleporter_arc;
  Mix_Chunk *SFX_editor_blocked;
  bool audio_ready;
#endif
};

#endif
