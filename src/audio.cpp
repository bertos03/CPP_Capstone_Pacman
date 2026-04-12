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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

constexpr int kSampleRate = 44100;
constexpr int kChannels = 2;
constexpr double kPi = 3.14159265358979323846;

void append_le16(std::vector<Uint8> &buffer, Uint16 value) {
  buffer.push_back(static_cast<Uint8>(value & 0xFF));
  buffer.push_back(static_cast<Uint8>((value >> 8) & 0xFF));
}

void append_le32(std::vector<Uint8> &buffer, Uint32 value) {
  buffer.push_back(static_cast<Uint8>(value & 0xFF));
  buffer.push_back(static_cast<Uint8>((value >> 8) & 0xFF));
  buffer.push_back(static_cast<Uint8>((value >> 16) & 0xFF));
  buffer.push_back(static_cast<Uint8>((value >> 24) & 0xFF));
}

std::vector<Uint8> build_wav_buffer(const std::vector<Sint16> &pcm_samples) {
  const Uint32 data_size =
      static_cast<Uint32>(pcm_samples.size() * sizeof(Sint16));
  std::vector<Uint8> wav;
  wav.reserve(44 + data_size);

  wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
  append_le32(wav, 36 + data_size);
  wav.insert(wav.end(), {'W', 'A', 'V', 'E'});
  wav.insert(wav.end(), {'f', 'm', 't', ' '});
  append_le32(wav, 16);
  append_le16(wav, 1);
  append_le16(wav, kChannels);
  append_le32(wav, kSampleRate);
  append_le32(wav, kSampleRate * kChannels * sizeof(Sint16));
  append_le16(wav, kChannels * sizeof(Sint16));
  append_le16(wav, 16);
  wav.insert(wav.end(), {'d', 'a', 't', 'a'});
  append_le32(wav, data_size);

  const Uint8 *sample_bytes =
      reinterpret_cast<const Uint8 *>(pcm_samples.data());
  wav.insert(wav.end(), sample_bytes, sample_bytes + data_size);
  return wav;
}

} // namespace

/**
 * @brief Construct a new Audio:: Audio object
 * Takes care of all sound effects
 */
Audio::Audio() {
#ifndef AUDIO
  std::cout << "No Audio\n";
#endif
#ifdef AUDIO
  audio_ready = false;
  SFX_coin = nullptr;
  SFX_win = nullptr;
  SFX_gameover = nullptr;
  SFX_menu_move = nullptr;
  SFX_menu_select = nullptr;
  SFX_countdown_tick = nullptr;
  SFX_start_trumpet = nullptr;

  if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0 &&
      SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    std::cerr << "SDL audio could not initialize: " << SDL_GetError() << "\n";
    return;
  }

  if (Mix_OpenAudio(kSampleRate, AUDIO_S16SYS, kChannels, 2048) < 0) {
    std::cerr << "SDL_mixer could not initialize: " << Mix_GetError() << "\n";
    return;
  }

  Mix_AllocateChannels(16);

  SFX_coin = Mix_LoadWAV(COIN_SOUND_PATH);
  SFX_win = Mix_LoadWAV(WIN_SOUND_PATH);
  SFX_gameover = Mix_LoadWAV(GAMEOVER_SOUND_PATH);
  SFX_menu_move = CreateSynthChunk({1396, 1865}, 70, 0.30, 4.0, 38.0);
  SFX_menu_select = CreateSynthChunk({784, 988, 1175}, 150, 0.42, 6.0, 90.0);
  SFX_countdown_tick = CreateSynthChunk({880, 1320}, 85, 0.25, 2.0, 48.0);
  SFX_start_trumpet = CreateTrumpetChunk();

  if (SFX_coin == nullptr || SFX_win == nullptr || SFX_gameover == nullptr ||
      SFX_menu_move == nullptr || SFX_menu_select == nullptr ||
      SFX_countdown_tick == nullptr || SFX_start_trumpet == nullptr) {
    std::cerr << "Failed to load SFX: " << Mix_GetError() << "\n";
    return;
  }

  audio_ready = true;
#endif
};

/**
 * @brief Destroy the Audio:: Audio object
 *
 */
Audio::~Audio() {
#ifdef AUDIO
  if (audio_ready) {
    Mix_HaltChannel(-1);
  }

  if (SFX_coin != nullptr) {
    Mix_FreeChunk(SFX_coin);
  }
  if (SFX_win != nullptr) {
    Mix_FreeChunk(SFX_win);
  }
  if (SFX_gameover != nullptr) {
    Mix_FreeChunk(SFX_gameover);
  }
  if (SFX_menu_move != nullptr) {
    Mix_FreeChunk(SFX_menu_move);
  }
  if (SFX_menu_select != nullptr) {
    Mix_FreeChunk(SFX_menu_select);
  }
  if (SFX_countdown_tick != nullptr) {
    Mix_FreeChunk(SFX_countdown_tick);
  }
  if (SFX_start_trumpet != nullptr) {
    Mix_FreeChunk(SFX_start_trumpet);
  }

  if (Mix_QuerySpec(nullptr, nullptr, nullptr) != 0) {
    Mix_CloseAudio();
  }

  if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) != 0) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }
#endif
};

#ifdef AUDIO

Mix_Chunk *Audio::CreateSynthChunk(const std::vector<int> &frequencies,
                                   int duration_ms, double volume,
                                   double attack_ms, double release_ms) {
  const int sample_count = std::max(1, duration_ms * kSampleRate / 1000);
  const int attack_samples =
      std::max(1, static_cast<int>(attack_ms * kSampleRate / 1000.0));
  const int release_samples =
      std::max(1, static_cast<int>(release_ms * kSampleRate / 1000.0));

  std::vector<Sint16> pcm_samples;
  pcm_samples.reserve(sample_count * kChannels);

  for (int i = 0; i < sample_count; i++) {
    const double time = static_cast<double>(i) / kSampleRate;
    double envelope = 1.0;

    if (i < attack_samples) {
      envelope = static_cast<double>(i) / attack_samples;
    } else if (i > sample_count - release_samples) {
      envelope =
          static_cast<double>(sample_count - i) / std::max(1, release_samples);
    }

    envelope = std::clamp(envelope, 0.0, 1.0);

    double sample_value = 0.0;
    for (size_t freq_index = 0; freq_index < frequencies.size(); freq_index++) {
      const double frequency = static_cast<double>(frequencies[freq_index]);
      const double harmonic_weight = 1.0 / (freq_index + 1.0);
      sample_value +=
          harmonic_weight * std::sin(2.0 * kPi * frequency * time);
      sample_value += 0.22 * harmonic_weight *
                      std::sin(4.0 * kPi * frequency * time + 0.3);
    }

    sample_value /= std::max<size_t>(1, frequencies.size());
    sample_value *= envelope * volume;
    sample_value = std::clamp(sample_value, -1.0, 1.0);

    const Sint16 pcm = static_cast<Sint16>(sample_value * 32767.0);
    pcm_samples.push_back(pcm);
    pcm_samples.push_back(pcm);
  }

  std::vector<Uint8> wav_buffer = build_wav_buffer(pcm_samples);
  SDL_RWops *wav_stream =
      SDL_RWFromConstMem(wav_buffer.data(), static_cast<int>(wav_buffer.size()));
  if (wav_stream == nullptr) {
    return nullptr;
  }

  return Mix_LoadWAV_RW(wav_stream, 1);
}

Mix_Chunk *Audio::CreateTrumpetChunk() {
  const std::vector<int> melody{523, 659, 784};
  std::vector<Sint16> pcm_samples;

  for (size_t note_index = 0; note_index < melody.size(); note_index++) {
    const int duration_ms = (note_index + 1 == melody.size()) ? 240 : 110;
    const int sample_count = duration_ms * kSampleRate / 1000;

    for (int i = 0; i < sample_count; i++) {
      const double time = static_cast<double>(i) / kSampleRate;
      const double absolute_time =
          (note_index * 0.11) + static_cast<double>(i) / kSampleRate;
      double envelope = 1.0;

      const int attack_samples = std::max(1, sample_count / 10);
      const int release_samples = std::max(1, sample_count / 5);
      if (i < attack_samples) {
        envelope = static_cast<double>(i) / attack_samples;
      } else if (i > sample_count - release_samples) {
        envelope =
            static_cast<double>(sample_count - i) / std::max(1, release_samples);
      }

      envelope = std::clamp(envelope, 0.0, 1.0);

      const double frequency = melody[note_index];
      const double vibrato =
          1.0 + 0.012 * std::sin(2.0 * kPi * 6.2 * absolute_time);
      const double fundamental =
          std::sin(2.0 * kPi * frequency * vibrato * time);
      const double third_harmonic =
          0.55 * std::sin(2.0 * kPi * frequency * 2.0 * vibrato * time + 0.2);
      const double fifth_harmonic =
          0.22 * std::sin(2.0 * kPi * frequency * 3.0 * vibrato * time + 0.4);
      double sample_value =
          (fundamental + third_harmonic + fifth_harmonic) * envelope * 0.40;

      sample_value = std::clamp(sample_value, -1.0, 1.0);
      const Sint16 pcm = static_cast<Sint16>(sample_value * 32767.0);
      pcm_samples.push_back(pcm);
      pcm_samples.push_back(pcm);
    }
  }

  std::vector<Uint8> wav_buffer = build_wav_buffer(pcm_samples);
  SDL_RWops *wav_stream =
      SDL_RWFromConstMem(wav_buffer.data(), static_cast<int>(wav_buffer.size()));
  if (wav_stream == nullptr) {
    return nullptr;
  }

  return Mix_LoadWAV_RW(wav_stream, 1);
}

void Audio::PlayChunk(Mix_Chunk *chunk) {
  if (!audio_ready || chunk == nullptr) {
    return;
  }
  Mix_PlayChannel(-1, chunk, 0);
}

/**
 * @brief as the name says
 *
 */
void Audio::PlayCoin() { PlayChunk(SFX_coin); };

/**
 * @brief as the name says
 *
 */
void Audio::PlayGameOver() { PlayChunk(SFX_gameover); };

/**
 * @brief as the name says
 *
 */
void Audio::PlayWin() { PlayChunk(SFX_win); };

void Audio::PlayMenuMove() { PlayChunk(SFX_menu_move); };

void Audio::PlayMenuSelect() { PlayChunk(SFX_menu_select); };

void Audio::PlayCountdownTick() { PlayChunk(SFX_countdown_tick); };

void Audio::PlayStartTrumpet() { PlayChunk(SFX_start_trumpet); };

#endif
