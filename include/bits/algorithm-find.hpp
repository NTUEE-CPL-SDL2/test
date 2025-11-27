#pragma once // algorithm-find.hpp

#include <utility>

namespace mystd {

template <class InputIt, class UnaryPred>
constexpr InputIt find_if(InputIt first, InputIt last, UnaryPred p) {
  for (; first != last; ++first) {
    if (p(*first)) {
      return first;
    }
  }
  return last;
}

template <class InputIt, class T>
constexpr InputIt find(InputIt first, InputIt last, const T &value) {
  return find_if(first, last, [&](auto &&elem) { return elem == value; });
}

template <class InputIt, class UnaryPred>
constexpr InputIt find_if_not(InputIt first, InputIt last, UnaryPred q) {
  return find_if(first, last, [&](auto &&elem) { return !q(elem); });
}

} // namespace mystd
