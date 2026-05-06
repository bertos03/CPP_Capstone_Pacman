#ifndef TEXTCACHE_H
#define TEXTCACHE_H

#include <SDL.h>
#include <SDL_ttf.h>

#include <cstdint>
#include <string>
#include <unordered_map>

class TextCache {
public:
  struct Entry {
    SDL_Texture *texture = nullptr;
    int w = 0;
    int h = 0;
  };

  ~TextCache() { Clear(); }

  void Clear() {
    for (auto &kv : entries_) {
      if (kv.second.texture != nullptr) {
        SDL_DestroyTexture(kv.second.texture);
      }
    }
    entries_.clear();
  }

  const Entry *GetOrCreate(SDL_Renderer *renderer, TTF_Font *font,
                           const std::string &text, SDL_Color color) {
    if (renderer == nullptr || font == nullptr || text.empty()) {
      return nullptr;
    }
    Key key{font, text, PackColor(color)};
    auto it = entries_.find(key);
    if (it != entries_.end()) {
      return &it->second;
    }
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (surface == nullptr) {
      return nullptr;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    Entry entry;
    if (texture != nullptr) {
      entry.texture = texture;
      entry.w = surface->w;
      entry.h = surface->h;
    }
    SDL_FreeSurface(surface);
    if (entry.texture == nullptr) {
      return nullptr;
    }
    auto inserted = entries_.emplace(std::move(key), entry);
    return &inserted.first->second;
  }

private:
  struct Key {
    TTF_Font *font;
    std::string text;
    std::uint32_t color;
    bool operator==(const Key &o) const noexcept {
      return font == o.font && color == o.color && text == o.text;
    }
  };
  struct KeyHash {
    size_t operator()(const Key &k) const noexcept {
      size_t h = std::hash<TTF_Font *>{}(k.font);
      h ^= std::hash<std::string>{}(k.text) + 0x9e3779b9u + (h << 6) +
           (h >> 2);
      h ^= std::hash<std::uint32_t>{}(k.color) + 0x9e3779b9u + (h << 6) +
           (h >> 2);
      return h;
    }
  };

  static std::uint32_t PackColor(SDL_Color c) {
    return (std::uint32_t(c.r) << 24) | (std::uint32_t(c.g) << 16) |
           (std::uint32_t(c.b) << 8) | std::uint32_t(c.a);
  }

  std::unordered_map<Key, Entry, KeyHash> entries_;
};

#endif
