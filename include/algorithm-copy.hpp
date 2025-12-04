#include <cstring>
#include <iterator>
#include <type_traits>

namespace mystd {

template <std::input_iterator InputIt, std::weakly_incrementable OutputIt>
  requires std::indirectly_writable<OutputIt, std::iter_value_t<InputIt>>
constexpr OutputIt copy(InputIt first, InputIt last, OutputIt d_first) {
  if constexpr (std::contiguous_iterator<InputIt> &&
                std::contiguous_iterator<OutputIt> &&
                std::is_trivially_copyable_v<
                    typename std::iterator_traits<InputIt>::value_type> &&
                std::is_same_v<
                    typename std::iterator_traits<InputIt>::value_type,
                    typename std::iterator_traits<OutputIt>::value_type>) {
    if (first == last)
      return d_first;
    const auto *src = std::to_address(first);
    const auto *src_end = std::to_address(last);
    auto *dest = std::to_address(d_first);
    const std::size_t count = (src_end - src) * sizeof(*src);
    std::memcpy(dest, src, count);
    return d_first + (last - first);
  }
  while (first != last) {
    *d_first = *first;
    ++first;
    ++d_first;
  }
  return d_first;
}

template <std::input_iterator InputIt, std::weakly_incrementable OutputIt,
          typename UnaryPred>
  requires std::indirectly_writable<OutputIt, std::iter_value_t<InputIt>>
constexpr OutputIt copy_if(InputIt first, InputIt last, OutputIt d_first,
                           UnaryPred pred) {
  while (first != last) {
    if (pred(*first)) {
      *d_first = *first;
      ++d_first;
    }
    ++first;
  }
  return d_first;
}

template <std::bidirectional_iterator BidirIt1,
          std::bidirectional_iterator BidirIt2>
constexpr BidirIt2 copy_backward(BidirIt1 first, BidirIt1 last,
                                 BidirIt2 d_last) {
  using ValueType1 = typename std::iterator_traits<BidirIt1>::value_type;
  using ValueType2 = typename std::iterator_traits<BidirIt2>::value_type;
  if constexpr (std::contiguous_iterator<BidirIt1> &&
                std::contiguous_iterator<BidirIt2> &&
                std::is_trivially_copyable_v<ValueType1> &&
                std::is_same_v<ValueType1, ValueType2>) {
    auto n = last - first;
    if (n > 0) {
      std::memmove(std::addressof(*d_last) - n, std::addressof(*first),
                   n * sizeof(ValueType1));
    }
    return d_last - n;
  } else {
    while (first != last) {
      --last;
      --d_last;
      *d_last = *last;
    }
    return d_last;
  }
}

} // namespace mystd
