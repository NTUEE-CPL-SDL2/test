#pragma once // algorithm-remove.hpp

#include <utility>

#include "algorithm-find.hpp"

namespace mystd {

template <class ForwardIt, class UnaryPred>
constexpr ForwardIt remove_if(ForwardIt first, ForwardIt last, UnaryPred p) {
  first = mystd::find_if(first, last, p);
  if (first == last)
    return first;
  ForwardIt result = first;
  for (++first; first != last; ++first) {
    if (!p(*first)) {
      *result = std::move(*first);
      ++result;
    }
  }
  return result;
}

template <class ForwardIt, class T>
constexpr ForwardIt remove(ForwardIt first, ForwardIt last, const T &value) {
  return remove_if(first, last, [&](auto &&elem) { return elem == value; });
}

} // namespace mystd
