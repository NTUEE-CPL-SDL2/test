#pragma once // swap.hpp

#include <cstddef>
#include <type_traits>
#include <utility>

namespace mystd {

template <class T>
  requires(std::is_move_constructible_v<T> && std::is_move_assignable_v<T>)
constexpr void swap(T &a,
                    T &b) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                   std::is_nothrow_move_assignable_v<T>) {
  T tmp = std::move(a);
  a = std::move(b);
  b = std::move(tmp);
}

template <class T, std::size_t N>
  requires std::is_swappable_v<T>
constexpr void swap(T (&a)[N],
                    T (&b)[N]) noexcept(noexcept(mystd::swap(a[0], b[0]))) {
  for (std::size_t i = 0; i < N; ++i) {
    mystd::swap(a[i], b[i]);
  }
}

} // namespace mystd
