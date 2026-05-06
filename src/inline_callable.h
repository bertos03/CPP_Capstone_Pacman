#ifndef INLINE_CALLABLE_H
#define INLINE_CALLABLE_H

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

template <std::size_t Size = 192>
class InlineCallable {
public:
  InlineCallable() = default;

  template <typename F,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<F>, InlineCallable>>>
  InlineCallable(F &&f) {
    using DF = std::decay_t<F>;
    static_assert(sizeof(DF) <= Size,
                  "InlineCallable: callable too large; increase Size.");
    static_assert(alignof(DF) <= alignof(std::max_align_t),
                  "InlineCallable: callable alignment too large.");
    new (storage_) DF(std::forward<F>(f));
    vt_ = &VTableFor<DF>;
  }

  ~InlineCallable() {
    if (vt_) vt_->destroy(storage_);
  }

  InlineCallable(const InlineCallable &) = delete;
  InlineCallable &operator=(const InlineCallable &) = delete;

  InlineCallable(InlineCallable &&other) noexcept {
    if (other.vt_) {
      other.vt_->move(other.storage_, storage_);
      vt_ = other.vt_;
      other.vt_->destroy(other.storage_);
      other.vt_ = nullptr;
    }
  }

  InlineCallable &operator=(InlineCallable &&other) noexcept {
    if (this != &other) {
      if (vt_) vt_->destroy(storage_);
      vt_ = nullptr;
      if (other.vt_) {
        other.vt_->move(other.storage_, storage_);
        vt_ = other.vt_;
        other.vt_->destroy(other.storage_);
        other.vt_ = nullptr;
      }
    }
    return *this;
  }

  void operator()() const { vt_->invoke(const_cast<unsigned char *>(storage_)); }

  explicit operator bool() const noexcept { return vt_ != nullptr; }

  void Reset() noexcept {
    if (vt_) {
      vt_->destroy(storage_);
      vt_ = nullptr;
    }
  }

private:
  struct VTable {
    void (*invoke)(void *);
    void (*destroy)(void *);
    void (*move)(void *src, void *dst);
  };

  template <typename DF>
  static inline const VTable VTableFor = {
      [](void *p) { (*static_cast<DF *>(p))(); },
      [](void *p) { static_cast<DF *>(p)->~DF(); },
      [](void *src, void *dst) {
        new (dst) DF(std::move(*static_cast<DF *>(src)));
      }};

  alignas(std::max_align_t) unsigned char storage_[Size]{};
  const VTable *vt_ = nullptr;
};

#endif
