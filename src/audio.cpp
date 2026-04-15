/*
 * PROJECT COMMENT (audio.cpp)
 * ---------------------------------------------------------------------------
 * This file is part of "BobMan", an SDL2 game inspired by Pac-Man.
 * The code in this unit has a clear responsibility so newcomers can quickly
 * understand the architecture: data model (map, elements), runtime logic
 * (game, events), rendering (renderer), and optional audio output.
 *
 * Important notes for newcomers:
 * - Header files declare classes, methods, and data types.
 * - CPP files contain the concrete implementation of the logic.
 * - Multiple threads move game entities in parallel, so shared data is read
 *   and written in a controlled way.
 * - Macros in definitions.h control resource paths, colors, and features.
 */

#include "audio.h"
#include "paths.h"

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
  SFX_monster_shot = nullptr;
  SFX_fireball_wall_hit = nullptr;
  SFX_monster_explosion = nullptr;
  SFX_monster_fart = nullptr;
  SFX_pacman_gag = nullptr;
  SFX_teleporter_zap = nullptr;
  SFX_teleporter_arc = nullptr;
  SFX_editor_blocked = nullptr;
  SFX_potion_spawn = nullptr;
  SFX_dynamite_spawn = nullptr;
  SFX_dynamite_explosion = nullptr;
  SFX_invulnerability_loop = nullptr;
  menu_music = nullptr;
  invulnerability_loop_channel = -1;

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

  const std::string coin_sound_path = Paths::GetDataFilePath("coin.wav");
  const std::string win_sound_path = Paths::GetDataFilePath("win.wav");
  const std::string monster_shot_sound_path = Paths::GetDataFilePath("schuss.wav");
  const std::string fireball_impact_sound_path =
      Paths::GetDataFilePath("einschlag.wav");
  const std::string monster_explosion_sound_path =
      Paths::GetDataFilePath("monsterexplosion.wav");
  const std::string dynamite_explosion_sound_path =
      Paths::GetDataFilePath("dynamitexplosion.wav");
  const std::string menu_music_path = Paths::GetDataFilePath("menu_music.mp3");

  SFX_coin = Mix_LoadWAV(coin_sound_path.c_str());
  SFX_win = Mix_LoadWAV(win_sound_path.c_str());
  SFX_gameover = CreateViolinLamentChunk();
  SFX_menu_move = CreateSynthChunk({1396, 1865}, 70, 0.30, 4.0, 38.0);
  SFX_menu_select = CreateSynthChunk({784, 988, 1175}, 150, 0.42, 6.0, 90.0);
  SFX_countdown_tick = CreateSynthChunk({880, 1320}, 85, 0.25, 2.0, 48.0);
  SFX_start_trumpet = CreateTrumpetChunk();
  SFX_monster_shot = Mix_LoadWAV(monster_shot_sound_path.c_str());
  if (SFX_monster_shot == nullptr) {
    SFX_monster_shot = CreateSynthChunk({698, 932, 1396}, 120, 0.30, 2.0, 56.0);
  }
  SFX_fireball_wall_hit = Mix_LoadWAV(fireball_impact_sound_path.c_str());
  if (SFX_fireball_wall_hit == nullptr) {
    SFX_fireball_wall_hit = CreateSynthChunk({172, 118}, 90, 0.26, 1.0, 44.0);
  }
  SFX_monster_explosion = Mix_LoadWAV(monster_explosion_sound_path.c_str());
  if (SFX_monster_explosion == nullptr) {
    SFX_monster_explosion = CreateMonsterExplosionChunk();
  }
  SFX_monster_fart = CreateMonsterFartChunk();
  SFX_pacman_gag = CreatePacmanGagChunk();
  SFX_teleporter_zap = CreateTeleporterZapChunk();
  SFX_teleporter_arc = CreateTeleporterArcChunk();
  SFX_editor_blocked = CreateEditorBlockedChunk();
  SFX_potion_spawn = CreatePotionSpawnChunk();
  SFX_dynamite_spawn = CreateDynamiteSpawnChunk();
  SFX_dynamite_explosion = Mix_LoadWAV(dynamite_explosion_sound_path.c_str());
  if (SFX_dynamite_explosion == nullptr) {
    SFX_dynamite_explosion = CreateDynamiteExplosionChunk();
  }
  SFX_invulnerability_loop = CreateInvulnerabilityLoopChunk();
  menu_music = Mix_LoadMUS(menu_music_path.c_str());
  if (menu_music == nullptr) {
    std::cerr << "Failed to load menu music " << menu_music_path << ": "
              << Mix_GetError() << "\n";
  }

  if (SFX_coin == nullptr || SFX_win == nullptr || SFX_gameover == nullptr ||
      SFX_menu_move == nullptr || SFX_menu_select == nullptr ||
      SFX_countdown_tick == nullptr || SFX_start_trumpet == nullptr ||
      SFX_monster_shot == nullptr || SFX_fireball_wall_hit == nullptr ||
      SFX_monster_explosion == nullptr || SFX_monster_fart == nullptr ||
      SFX_pacman_gag == nullptr || SFX_teleporter_zap == nullptr ||
      SFX_teleporter_arc == nullptr || SFX_editor_blocked == nullptr ||
      SFX_potion_spawn == nullptr || SFX_dynamite_spawn == nullptr ||
      SFX_dynamite_explosion == nullptr ||
      SFX_invulnerability_loop == nullptr) {
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
  StopInvulnerabilityLoop();
  if (audio_ready) {
    Mix_HaltChannel(-1);
    Mix_HaltMusic();
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
  if (SFX_monster_shot != nullptr) {
    Mix_FreeChunk(SFX_monster_shot);
  }
  if (SFX_fireball_wall_hit != nullptr) {
    Mix_FreeChunk(SFX_fireball_wall_hit);
  }
  if (SFX_monster_explosion != nullptr) {
    Mix_FreeChunk(SFX_monster_explosion);
  }
  if (SFX_monster_fart != nullptr) {
    Mix_FreeChunk(SFX_monster_fart);
  }
  if (SFX_pacman_gag != nullptr) {
    Mix_FreeChunk(SFX_pacman_gag);
  }
  if (SFX_teleporter_zap != nullptr) {
    Mix_FreeChunk(SFX_teleporter_zap);
  }
  if (SFX_teleporter_arc != nullptr) {
    Mix_FreeChunk(SFX_teleporter_arc);
  }
  if (SFX_editor_blocked != nullptr) {
    Mix_FreeChunk(SFX_editor_blocked);
  }
  if (SFX_potion_spawn != nullptr) {
    Mix_FreeChunk(SFX_potion_spawn);
  }
  if (SFX_dynamite_spawn != nullptr) {
    Mix_FreeChunk(SFX_dynamite_spawn);
  }
  if (SFX_dynamite_explosion != nullptr) {
    Mix_FreeChunk(SFX_dynamite_explosion);
  }
  if (SFX_invulnerability_loop != nullptr) {
    Mix_FreeChunk(SFX_invulnerability_loop);
  }
  if (menu_music != nullptr) {
    Mix_FreeMusic(menu_music);
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

Mix_Chunk *Audio::CreateViolinLamentChunk() {
  struct ViolinNote {
    double frequency;
    int duration_ms;
    double volume;
  };

  const std::vector<ViolinNote> melody{
      {659.25, 180, 0.16}, {587.33, 210, 0.18}, {523.25, 260, 0.22},
      {493.88, 240, 0.20}, {440.00, 420, 0.24}};
  std::vector<Sint16> pcm_samples;

  double note_time_offset = 0.0;
  for (const auto &note : melody) {
    const int sample_count = std::max(1, note.duration_ms * kSampleRate / 1000);
    const int attack_samples = std::max(1, sample_count / 8);
    const int release_samples = std::max(1, sample_count / 4);

    for (int i = 0; i < sample_count; i++) {
      double envelope = 1.0;
      if (i < attack_samples) {
        envelope = static_cast<double>(i) / attack_samples;
      } else if (i > sample_count - release_samples) {
        envelope =
            static_cast<double>(sample_count - i) / std::max(1, release_samples);
      }
      envelope = std::clamp(envelope, 0.0, 1.0);

      const double time = static_cast<double>(i) / kSampleRate;
      const double absolute_time = note_time_offset + time;
      const double vibrato =
          1.0 + 0.010 * std::sin(2.0 * kPi * 5.1 * absolute_time);
      const double detuned =
          1.0 + 0.004 * std::sin(2.0 * kPi * 2.7 * absolute_time + 0.8);
      const double fundamental =
          std::sin(2.0 * kPi * note.frequency * vibrato * time);
      const double upper =
          0.48 * std::sin(2.0 * kPi * note.frequency * 2.0 * detuned * time +
                          0.32);
      const double airy =
          0.18 * std::sin(2.0 * kPi * note.frequency * 3.0 * time + 1.1);
      const double bow_noise =
          0.035 * std::sin(2.0 * kPi * 91.0 * absolute_time + 0.4);
      double sample_value =
          (fundamental + upper + airy + bow_noise) * envelope * note.volume;

      sample_value = std::clamp(sample_value, -1.0, 1.0);
      const Sint16 pcm = static_cast<Sint16>(sample_value * 32767.0);
      pcm_samples.push_back(pcm);
      pcm_samples.push_back(pcm);
    }

    note_time_offset += static_cast<double>(note.duration_ms) / 1000.0;
  }

  std::vector<Uint8> wav_buffer = build_wav_buffer(pcm_samples);
  SDL_RWops *wav_stream =
      SDL_RWFromConstMem(wav_buffer.data(), static_cast<int>(wav_buffer.size()));
  if (wav_stream == nullptr) {
    return nullptr;
  }

  return Mix_LoadWAV_RW(wav_stream, 1);
}

Mix_Chunk *Audio::CreateMonsterExplosionChunk() {
  const int duration_ms = 170;
  const int sample_count = std::max(1, duration_ms * kSampleRate / 1000);
  std::vector<Sint16> pcm_samples;
  pcm_samples.reserve(sample_count * kChannels);

  for (int i = 0; i < sample_count; i++) {
    const double time = static_cast<double>(i) / kSampleRate;
    const double progress = static_cast<double>(i) / sample_count;
    double envelope = 1.0 - progress;
    envelope = std::clamp(envelope, 0.0, 1.0);

    const double low = std::sin(2.0 * kPi * 184.0 * time);
    const double mid = 0.55 * std::sin(2.0 * kPi * 133.0 * time + 0.8);
    const double high = 0.25 * std::sin(2.0 * kPi * 512.0 * time + 1.2);
    const double crackle =
        0.09 * std::sin(2.0 * kPi * (1500.0 - progress * 900.0) * time + 0.2);
    double sample_value = (low + mid + high + crackle) * envelope * 0.34;

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

Mix_Chunk *Audio::CreateMonsterFartChunk() {
  const int duration_ms = 460;
  const int sample_count = std::max(1, duration_ms * kSampleRate / 1000);
  std::vector<Sint16> pcm_samples;
  pcm_samples.reserve(sample_count * kChannels);

  for (int i = 0; i < sample_count; i++) {
    const double time = static_cast<double>(i) / kSampleRate;
    const double progress = static_cast<double>(i) / sample_count;
    const double fundamental_frequency = 86.0 - progress * 34.0;
    const double wobble =
        1.0 + 0.07 * std::sin(2.0 * kPi * 4.4 * time + 0.2);
    const double envelope =
        std::pow(std::clamp(1.0 - progress, 0.0, 1.0), 0.72);
    const double bloom = std::exp(-18.0 * std::pow(progress - 0.18, 2.0));
    const double noise =
        0.11 * std::sin(2.0 * kPi * (470.0 - progress * 170.0) * time + 0.4) +
        0.06 * std::sin(2.0 * kPi * (910.0 - progress * 260.0) * time + 1.1);
    const double body =
        0.95 * std::sin(2.0 * kPi * fundamental_frequency * wobble * time) +
        0.42 *
            std::sin(2.0 * kPi * fundamental_frequency * 1.7 * wobble * time +
                     0.6) +
        0.18 *
            std::sin(2.0 * kPi * fundamental_frequency * 2.4 * wobble * time +
                     1.5);
    double sample_value = (body + noise) * envelope * (0.24 + 0.11 * bloom);

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

Mix_Chunk *Audio::CreatePacmanGagChunk() {
  const int duration_ms = 720;
  const int sample_count = std::max(1, duration_ms * kSampleRate / 1000);
  std::vector<Sint16> pcm_samples;
  pcm_samples.reserve(sample_count * kChannels);

  for (int i = 0; i < sample_count; i++) {
    const double time = static_cast<double>(i) / kSampleRate;
    const double progress = static_cast<double>(i) / sample_count;
    const double burst_one = std::exp(-120.0 * std::pow(progress - 0.18, 2.0));
    const double burst_two = std::exp(-105.0 * std::pow(progress - 0.45, 2.0));
    const double burst_three =
        std::exp(-90.0 * std::pow(progress - 0.69, 2.0));
    const double burst_mix =
        std::clamp(burst_one + 0.95 * burst_two + 0.8 * burst_three, 0.0, 1.4);
    const double throat_frequency = 158.0 - progress * 36.0;
    const double wet_vibrato =
        1.0 + 0.05 * std::sin(2.0 * kPi * 7.3 * time + 0.7);
    const double throat =
        std::sin(2.0 * kPi * throat_frequency * wet_vibrato * time) +
        0.58 *
            std::sin(2.0 * kPi * throat_frequency * 1.96 * wet_vibrato * time +
                     0.45);
    const double retch_formant =
        0.44 * std::sin(2.0 * kPi * (540.0 + burst_mix * 70.0) * time + 1.0) +
        0.24 * std::sin(2.0 * kPi * (860.0 - progress * 180.0) * time + 0.2);
    const double saliva_noise =
        0.10 * std::sin(2.0 * kPi * (1240.0 - progress * 220.0) * time + 0.9) +
        0.07 * std::sin(2.0 * kPi * (1730.0 - progress * 340.0) * time + 1.7);
    const double envelope =
        std::pow(std::clamp(1.0 - progress, 0.0, 1.0), 0.55) * burst_mix;
    double sample_value =
        (0.78 * throat + retch_formant + saliva_noise) * envelope * 0.24;

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

Mix_Chunk *Audio::CreateTeleporterZapChunk() {
  const int duration_ms = 640;
  const int sample_count = std::max(1, duration_ms * kSampleRate / 1000);
  std::vector<Sint16> pcm_samples;
  pcm_samples.reserve(sample_count * kChannels);

  for (int i = 0; i < sample_count; i++) {
    const double time = static_cast<double>(i) / kSampleRate;
    const double progress = static_cast<double>(i) / sample_count;
    const double envelope =
        std::pow(std::clamp(1.0 - progress, 0.0, 1.0), 0.42);
    const double pulse =
        0.55 + 0.45 * std::sin(2.0 * kPi * (8.0 + progress * 12.0) * time);
    const double carrier =
        std::sin(2.0 * kPi * (620.0 + progress * 410.0) * time +
                 0.65 * std::sin(2.0 * kPi * 11.0 * time));
    const double high_arc =
        0.55 * std::sin(2.0 * kPi * (1420.0 - progress * 290.0) * time + 0.3);
    const double shimmer =
        0.22 * std::sin(2.0 * kPi * (2380.0 - progress * 700.0) * time + 0.9);
    const double crackle_gate =
        (std::sin(2.0 * kPi * 26.0 * time) > 0.2) ? 1.0 : 0.18;
    const double crackle =
        0.14 * crackle_gate *
        std::sin(2.0 * kPi * (3100.0 - progress * 1200.0) * time + 1.7);
    double sample_value =
        (carrier * pulse + high_arc + shimmer + crackle) * envelope * 0.23;

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

Mix_Chunk *Audio::CreateTeleporterArcChunk() {
  const int duration_ms = 540;
  const int sample_count = std::max(1, duration_ms * kSampleRate / 1000);
  std::vector<Sint16> pcm_samples;
  pcm_samples.reserve(sample_count * kChannels);

  for (int i = 0; i < sample_count; i++) {
    const double time = static_cast<double>(i) / kSampleRate;
    const double progress = static_cast<double>(i) / sample_count;
    const double envelope =
        std::pow(std::clamp(1.0 - progress, 0.0, 1.0), 0.33);
    const double arc_mod =
        1.0 + 0.14 * std::sin(2.0 * kPi * 13.0 * time + 0.4);
    const double fundamental =
        std::sin(2.0 * kPi * (840.0 + progress * 360.0) * arc_mod * time);
    const double upper =
        0.62 * std::sin(2.0 * kPi * (1720.0 - progress * 220.0) * time + 0.9);
    const double hiss =
        0.18 * std::sin(2.0 * kPi * (3280.0 - progress * 910.0) * time + 1.8);
    const double gating =
        (std::sin(2.0 * kPi * 31.0 * time) > -0.1) ? 1.0 : 0.10;
    const double crack =
        0.15 * gating *
        std::sin(2.0 * kPi * (4120.0 - progress * 1400.0) * time + 0.1);
    double sample_value =
        (fundamental + upper + hiss + crack) * envelope * 0.20;

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

Mix_Chunk *Audio::CreateEditorBlockedChunk() {
  const int duration_ms = 170;
  const int sample_count = std::max(1, duration_ms * kSampleRate / 1000);
  std::vector<Sint16> pcm_samples;
  pcm_samples.reserve(sample_count * kChannels);

  for (int i = 0; i < sample_count; i++) {
    const double time = static_cast<double>(i) / kSampleRate;
    const double progress = static_cast<double>(i) / sample_count;
    const double envelope =
        std::pow(std::clamp(1.0 - progress, 0.0, 1.0), 0.80);
    const double metallic =
        std::sin(2.0 * kPi * 310.0 * time) +
        0.72 * std::sin(2.0 * kPi * 517.0 * time + 0.18) +
        0.38 * std::sin(2.0 * kPi * 836.0 * time + 0.74);
    const double clink =
        0.22 * std::sin(2.0 * kPi * 1510.0 * time + 1.4) +
        0.12 * std::sin(2.0 * kPi * 2140.0 * time + 0.2);
    double sample_value = (metallic + clink) * envelope * 0.17;

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

Mix_Chunk *Audio::CreatePotionSpawnChunk() {
  struct Note {
    double frequency;
    int duration_ms;
    double volume;
  };

  const std::vector<Note> melody{
      {880.0, 90, 0.18}, {1174.66, 95, 0.20}, {1396.91, 120, 0.22},
      {1760.0, 170, 0.19}};
  std::vector<Sint16> pcm_samples;

  double time_offset = 0.0;
  for (const auto &note : melody) {
    const int sample_count = std::max(1, note.duration_ms * kSampleRate / 1000);
    const int attack_samples = std::max(1, sample_count / 10);
    const int release_samples = std::max(1, sample_count / 5);

    for (int i = 0; i < sample_count; i++) {
      double envelope = 1.0;
      if (i < attack_samples) {
        envelope = static_cast<double>(i) / attack_samples;
      } else if (i > sample_count - release_samples) {
        envelope =
            static_cast<double>(sample_count - i) / std::max(1, release_samples);
      }
      envelope = std::clamp(envelope, 0.0, 1.0);

      const double time = static_cast<double>(i) / kSampleRate;
      const double absolute_time = time_offset + time;
      const double shimmer =
          1.0 + 0.015 * std::sin(2.0 * kPi * 8.0 * absolute_time + 0.3);
      const double fundamental =
          std::sin(2.0 * kPi * note.frequency * shimmer * time);
      const double bright =
          0.48 * std::sin(2.0 * kPi * note.frequency * 2.0 * time + 0.6);
      const double sparkle =
          0.18 * std::sin(2.0 * kPi * (note.frequency * 3.4) * time + 1.1);
      double sample_value =
          (fundamental + bright + sparkle) * envelope * note.volume;

      sample_value = std::clamp(sample_value, -1.0, 1.0);
      const Sint16 pcm = static_cast<Sint16>(sample_value * 32767.0);
      pcm_samples.push_back(pcm);
      pcm_samples.push_back(pcm);
    }

    time_offset += static_cast<double>(note.duration_ms) / 1000.0;
  }

  std::vector<Uint8> wav_buffer = build_wav_buffer(pcm_samples);
  SDL_RWops *wav_stream =
      SDL_RWFromConstMem(wav_buffer.data(), static_cast<int>(wav_buffer.size()));
  if (wav_stream == nullptr) {
    return nullptr;
  }

  return Mix_LoadWAV_RW(wav_stream, 1);
}

Mix_Chunk *Audio::CreateDynamiteSpawnChunk() {
  struct Note {
    double frequency;
    int duration_ms;
    double volume;
  };

  const std::vector<Note> melody{
      {392.0, 120, 0.18}, {523.25, 120, 0.19}, {659.25, 160, 0.21},
      {784.0, 220, 0.22}, {659.25, 260, 0.18}};
  std::vector<Sint16> pcm_samples;

  double time_offset = 0.0;
  for (const auto &note : melody) {
    const int sample_count = std::max(1, note.duration_ms * kSampleRate / 1000);
    const int attack_samples = std::max(1, sample_count / 12);
    const int release_samples = std::max(1, sample_count / 4);

    for (int i = 0; i < sample_count; i++) {
      double envelope = 1.0;
      if (i < attack_samples) {
        envelope = static_cast<double>(i) / attack_samples;
      } else if (i > sample_count - release_samples) {
        envelope =
            static_cast<double>(sample_count - i) / std::max(1, release_samples);
      }
      envelope = std::clamp(envelope, 0.0, 1.0);

      const double time = static_cast<double>(i) / kSampleRate;
      const double absolute_time = time_offset + time;
      const double swell =
          0.84 + 0.16 * std::sin(2.0 * kPi * 5.2 * absolute_time + 0.3);
      const double fundamental =
          std::sin(2.0 * kPi * note.frequency * time);
      const double fifth =
          0.46 * std::sin(2.0 * kPi * note.frequency * 1.5 * time + 0.2);
      const double brass =
          0.24 * std::sin(2.0 * kPi * note.frequency * 2.0 * time + 0.8);
      const double tension =
          0.10 * std::sin(2.0 * kPi * (note.frequency * 2.8 + 40.0) * time +
                          1.1);
      double sample_value =
          (fundamental + fifth + brass + tension) * swell * envelope *
          note.volume;

      sample_value = std::clamp(sample_value, -1.0, 1.0);
      const Sint16 pcm = static_cast<Sint16>(sample_value * 32767.0);
      pcm_samples.push_back(pcm);
      pcm_samples.push_back(pcm);
    }

    time_offset += static_cast<double>(note.duration_ms) / 1000.0;
  }

  std::vector<Uint8> wav_buffer = build_wav_buffer(pcm_samples);
  SDL_RWops *wav_stream =
      SDL_RWFromConstMem(wav_buffer.data(), static_cast<int>(wav_buffer.size()));
  if (wav_stream == nullptr) {
    return nullptr;
  }

  return Mix_LoadWAV_RW(wav_stream, 1);
}

Mix_Chunk *Audio::CreateDynamiteExplosionChunk() {
  const int duration_ms = 440;
  const int sample_count = std::max(1, duration_ms * kSampleRate / 1000);
  std::vector<Sint16> pcm_samples;
  pcm_samples.reserve(sample_count * kChannels);

  for (int i = 0; i < sample_count; i++) {
    const double time = static_cast<double>(i) / kSampleRate;
    const double progress = static_cast<double>(i) / sample_count;
    const double envelope =
        std::pow(std::clamp(1.0 - progress, 0.0, 1.0), 0.38);
    const double boom =
        std::sin(2.0 * kPi * (72.0 - progress * 18.0) * time + 0.1);
    const double body =
        0.72 * std::sin(2.0 * kPi * (118.0 - progress * 22.0) * time + 0.4);
    const double snap =
        0.34 * std::sin(2.0 * kPi * (620.0 - progress * 240.0) * time + 1.3);
    const double crackle =
        0.18 * std::sin(2.0 * kPi * (1820.0 - progress * 960.0) * time + 0.9) +
        0.08 * std::sin(2.0 * kPi * (2640.0 - progress * 1100.0) * time + 1.7);
    const double rumble_gate =
        0.70 + 0.30 * std::sin(2.0 * kPi * 7.0 * time + 0.2);
    double sample_value =
        (boom + body + snap + crackle) * rumble_gate * envelope * 0.36;

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

Mix_Chunk *Audio::CreateInvulnerabilityLoopChunk() {
  struct Note {
    double frequency;
    int duration_ms;
    double volume;
  };

  const std::vector<Note> melody{
      {392.0, 160, 0.11}, {523.25, 160, 0.11}, {659.25, 160, 0.11},
      {783.99, 160, 0.12}, {659.25, 160, 0.11}, {523.25, 160, 0.11},
      {880.0, 160, 0.12}, {783.99, 220, 0.12}};
  std::vector<Sint16> pcm_samples;

  double time_offset = 0.0;
  for (const auto &note : melody) {
    const int sample_count = std::max(1, note.duration_ms * kSampleRate / 1000);
    const int attack_samples = std::max(1, sample_count / 20);
    const int release_samples = std::max(1, sample_count / 8);

    for (int i = 0; i < sample_count; i++) {
      double envelope = 1.0;
      if (i < attack_samples) {
        envelope = static_cast<double>(i) / attack_samples;
      } else if (i > sample_count - release_samples) {
        envelope =
            static_cast<double>(sample_count - i) / std::max(1, release_samples);
      }
      envelope = std::clamp(envelope, 0.0, 1.0);

      const double time = static_cast<double>(i) / kSampleRate;
      const double absolute_time = time_offset + time;
      const double square =
          (std::sin(2.0 * kPi * note.frequency * time) >= 0.0) ? 1.0 : -1.0;
      const double octave =
          (std::sin(2.0 * kPi * note.frequency * 2.0 * time + 0.4) >= 0.0)
              ? 1.0
              : -1.0;
      const double bass =
          std::sin(2.0 * kPi * (note.frequency / 2.0) * time + 0.2);
      const double shimmer =
          0.74 + 0.26 * std::sin(2.0 * kPi * 6.0 * absolute_time);
      double sample_value =
          (0.58 * square + 0.26 * octave + 0.20 * bass) * shimmer *
          envelope * note.volume;

      sample_value = std::clamp(sample_value, -1.0, 1.0);
      const Sint16 pcm = static_cast<Sint16>(sample_value * 32767.0);
      pcm_samples.push_back(pcm);
      pcm_samples.push_back(pcm);
    }

    time_offset += static_cast<double>(note.duration_ms) / 1000.0;
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

bool Audio::StartMenuMusic() {
  if (!audio_ready || menu_music == nullptr) {
    return false;
  }

  if (Mix_PlayingMusic() != 0) {
    return true;
  }

  return Mix_PlayMusic(menu_music, -1) == 0;
}

bool Audio::FadeOutMenuMusic(int fade_out_ms) {
  if (!audio_ready || menu_music == nullptr || Mix_PlayingMusic() == 0) {
    return false;
  }

  if (fade_out_ms <= 0) {
    Mix_HaltMusic();
    return true;
  }

  return Mix_FadeOutMusic(fade_out_ms) == 1;
}

void Audio::StopMenuMusic() {
  if (!audio_ready || Mix_PlayingMusic() == 0) {
    return;
  }

  Mix_HaltMusic();
}

bool Audio::IsMenuMusicPlaying() const {
  if (!audio_ready || menu_music == nullptr) {
    return false;
  }

  return Mix_PlayingMusic() != 0;
}

void Audio::PlayMonsterShot() { PlayChunk(SFX_monster_shot); };

void Audio::PlayFireballWallHit() { PlayChunk(SFX_fireball_wall_hit); };

void Audio::PlayMonsterExplosion() { PlayChunk(SFX_monster_explosion); };

void Audio::PlayMonsterFart() { PlayChunk(SFX_monster_fart); };

void Audio::PlayPacmanGag() { PlayChunk(SFX_pacman_gag); };

void Audio::PlayTeleporterZap() { PlayChunk(SFX_teleporter_zap); };

void Audio::PlayTeleporterArc() { PlayChunk(SFX_teleporter_arc); };

void Audio::PlayEditorBlocked() { PlayChunk(SFX_editor_blocked); };

void Audio::PlayPotionSpawn() { PlayChunk(SFX_potion_spawn); };

void Audio::PlayDynamiteSpawn() { PlayChunk(SFX_dynamite_spawn); };

void Audio::PlayDynamiteExplosion() { PlayChunk(SFX_dynamite_explosion); };

void Audio::StartInvulnerabilityLoop() {
  if (!audio_ready || SFX_invulnerability_loop == nullptr) {
    return;
  }

  if (invulnerability_loop_channel != -1 &&
      Mix_Playing(invulnerability_loop_channel) != 0) {
    return;
  }

  invulnerability_loop_channel = Mix_PlayChannel(-1, SFX_invulnerability_loop, -1);
}

void Audio::StopInvulnerabilityLoop() {
  if (!audio_ready || invulnerability_loop_channel == -1) {
    return;
  }

  Mix_HaltChannel(invulnerability_loop_channel);
  invulnerability_loop_channel = -1;
}

#endif
