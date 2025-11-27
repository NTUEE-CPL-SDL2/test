#pragma once // ignore.hpp

namespace mystd {

struct ignore_t {
  template <class T>
  constexpr const ignore_t &operator=(const T &) const noexcept {
    return *this;
  }

  template <class T> constexpr const ignore_t &operator=(T &&) const noexcept {
    return *this;
  }
};

inline constexpr ignore_t ignore{};

} // namespace mystd
