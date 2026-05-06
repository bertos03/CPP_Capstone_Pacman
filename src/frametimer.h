#ifndef FRAMETIMER_H
#define FRAMETIMER_H

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cstddef>

class FrameTimer {
public:
  struct Stats {
    double avg_ms = 0.0;
    double fps = 0.0;
    Uint32 min_ms = 0;
    Uint32 p50_ms = 0;
    Uint32 p95_ms = 0;
    Uint32 p99_ms = 0;
    Uint32 max_ms = 0;
    size_t window_frames = 0;
  };

  FrameTimer() {
    last_tick_ = SDL_GetTicks();
    last_recompute_ = last_tick_;
  }

  void Tick() {
    const Uint32 now = SDL_GetTicks();
    const Uint32 dt = now - last_tick_;
    last_tick_ = now;
    samples_[ring_pos_] = dt;
    ring_pos_ = (ring_pos_ + 1) % samples_.size();
    if (samples_count_ < samples_.size()) ++samples_count_;
    if (now - last_recompute_ >= recompute_interval_ms_) {
      Recompute();
      last_recompute_ = now;
    }
  }

  const Stats &GetStats() const { return stats_; }

  void Reset() {
    samples_count_ = 0;
    ring_pos_ = 0;
    stats_ = Stats{};
    last_tick_ = SDL_GetTicks();
    last_recompute_ = last_tick_;
  }

private:
  void Recompute() {
    const size_t n = samples_count_;
    if (n == 0) {
      stats_ = Stats{};
      return;
    }
    std::array<Uint32, 256> sorted{};
    for (size_t i = 0; i < n; ++i) sorted[i] = samples_[i];
    std::sort(sorted.begin(), sorted.begin() + n);
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) sum += sorted[i];
    stats_.avg_ms = sum / static_cast<double>(n);
    stats_.fps = stats_.avg_ms > 0.0 ? 1000.0 / stats_.avg_ms : 0.0;
    stats_.min_ms = sorted[0];
    stats_.p50_ms = sorted[n / 2];
    stats_.p95_ms = sorted[std::min(n - 1, (n * 95) / 100)];
    stats_.p99_ms = sorted[std::min(n - 1, (n * 99) / 100)];
    stats_.max_ms = sorted[n - 1];
    stats_.window_frames = n;
  }

  std::array<Uint32, 256> samples_{};
  size_t samples_count_ = 0;
  size_t ring_pos_ = 0;
  Uint32 last_tick_ = 0;
  Uint32 last_recompute_ = 0;
  Uint32 recompute_interval_ms_ = 250;
  Stats stats_{};
};

#endif
