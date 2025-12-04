#pragma once // circulate.hpp

#include <algorithm>
#include <compare>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace extend {

template <class circulate> class _circulate_iterator {
public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = typename circulate::difference_type;
  using value_type = typename circulate::value_type;
  using pointer = typename circulate::pointer;
  using reference = typename circulate::reference;

private:
  circulate *circ;
  difference_type idx;

public:
  _circulate_iterator(circulate *c, difference_type i) : circ(c), idx(i) {}

  reference operator*() { return (*circ)[idx]; }
  pointer operator->() { return &(**this); }

  _circulate_iterator &operator++() {
    ++idx;
    return *this;
  }
  _circulate_iterator operator++(int) {
    _circulate_iterator tmp = *this;
    ++*this;
    return tmp;
  }
  _circulate_iterator &operator--() {
    --idx;
    return *this;
  }
  _circulate_iterator operator--(int) {
    _circulate_iterator tmp = *this;
    --*this;
    return tmp;
  }

  _circulate_iterator &operator+=(difference_type n) {
    idx += n;
    return *this;
  }
  _circulate_iterator &operator-=(difference_type n) {
    idx -= n;
    return *this;
  }
  _circulate_iterator operator+(difference_type n) const {
    return _circulate_iterator(circ, idx + n);
  }
  _circulate_iterator operator-(difference_type n) const {
    return _circulate_iterator(circ, idx - n);
  }
  difference_type operator-(const _circulate_iterator &other) const {
    return idx - other.idx;
  }

  reference operator[](difference_type n) { return *(*this + n); }

  friend _circulate_iterator operator+(difference_type n,
                                       const _circulate_iterator &it) {
    return it + n;
  }

  friend bool operator==(const _circulate_iterator &a,
                         const _circulate_iterator &b) noexcept {
    return a.circ == b.circ && a.idx == b.idx;
  }

  friend std::strong_ordering
  operator<=>(const _circulate_iterator &a,
              const _circulate_iterator &b) noexcept {
    if (a.circ != b.circ) {
      return a.circ <=> b.circ;
    }
    return a.idx <=> b.idx;
  }
};

template <class circulate> class _circulate_const_iterator {
public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = typename circulate::difference_type;
  using value_type = typename circulate::value_type;
  using pointer = typename circulate::const_pointer;
  using reference = typename circulate::const_reference;

private:
  const circulate *circ;
  difference_type idx;

public:
  _circulate_const_iterator(const circulate *c, difference_type i)
      : circ(c), idx(i) {}
  _circulate_const_iterator(const _circulate_iterator<circulate> &it)
      : circ(it.circ), idx(it.idx) {}

  reference operator*() const { return (*circ)[idx]; }
  pointer operator->() const { return &(**this); }

  _circulate_const_iterator &operator++() {
    ++idx;
    return *this;
  }
  _circulate_const_iterator operator++(int) {
    _circulate_const_iterator tmp = *this;
    ++*this;
    return tmp;
  }
  _circulate_const_iterator &operator--() {
    --idx;
    return *this;
  }
  _circulate_const_iterator operator--(int) {
    _circulate_const_iterator tmp = *this;
    --*this;
    return tmp;
  }

  _circulate_const_iterator &operator+=(difference_type n) {
    idx += n;
    return *this;
  }
  _circulate_const_iterator &operator-=(difference_type n) {
    idx -= n;
    return *this;
  }
  _circulate_const_iterator operator+(difference_type n) const {
    return _circulate_const_iterator(circ, idx + n);
  }
  _circulate_const_iterator operator-(difference_type n) const {
    return _circulate_const_iterator(circ, idx - n);
  }
  difference_type operator-(const _circulate_const_iterator &other) const {
    return idx - other.idx;
  }

  reference operator[](difference_type n) const { return *(*this + n); }

  friend _circulate_const_iterator
  operator+(difference_type n, const _circulate_const_iterator &it) {
    return it + n;
  }

  friend bool operator==(const _circulate_const_iterator &a,
                         const _circulate_const_iterator &b) noexcept {
    return a.circ == b.circ && a.idx == b.idx;
  }

  friend std::strong_ordering
  operator<=>(const _circulate_const_iterator &a,
              const _circulate_const_iterator &b) noexcept {
    if (a.circ != b.circ) {
      return a.circ <=> b.circ;
    }
    return a.idx <=> b.idx;
  }

  friend bool operator==(const _circulate_iterator<circulate> &a,
                         const _circulate_const_iterator &b) noexcept {
    return a.circ == b.circ && a.idx == b.idx;
  }

  friend bool operator!=(const _circulate_iterator<circulate> &a,
                         const _circulate_const_iterator &b) noexcept {
    return a.circ != b.circ || a.idx != b.idx;
  }

  friend std::strong_ordering
  operator<=>(const _circulate_iterator<circulate> &a,
              const _circulate_const_iterator &b) noexcept {
    if (a.circ != b.circ) {
      return a.circ <=> b.circ;
    }
    return a.idx <=> b.idx;
  }
};

template <class T, class Container = std::vector<T>> class circulate {
public:
  using value_type = T;
  using container_type = Container;
  using size_type = typename Container::size_type;
  using difference_type = typename Container::difference_type;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using iterator = _circulate_iterator<circulate>;
  using const_iterator = _circulate_const_iterator<circulate>;

public:
  Container c;

  constexpr size_type size() const noexcept { return c.size(); }
  constexpr difference_type ssize() const noexcept {
    return static_cast<difference_type>(c.size());
  }
  constexpr bool empty() const noexcept { return c.size() == 0; }

private:
  size_type start = 0;

  constexpr size_type circular_index(difference_type i) const noexcept {
    if (c.empty())
      return 0;
    difference_type idx = static_cast<difference_type>(start) + i;
    idx %= ssize();
    if (idx < 0)
      idx += ssize();
    return static_cast<size_type>(idx);
  }

public:
  circulate() = default;

  explicit circulate(Container cont, size_type Start = 0)
      : c(std::move(cont)), start(Start) {
    if (!c.empty())
      start %= c.size();
    else
      start = 0;
  }

  circulate(const circulate &other) : c(other.c), start(other.start) {}

  circulate(circulate &&other) noexcept(
      std::is_nothrow_move_constructible_v<Container>)
      : c(std::move(other.c)), start(std::exchange(other.start, 0)) {}

  circulate &operator=(const circulate &other) {
    if (this != &other) {
      c = other.c;
      start = other.start;
    }
    return *this;
  }

  circulate &
  operator=(circulate &&other) noexcept(this == &other ||
                                        noexcept(std::move(other.c))) {
    if (this != &other) {
      c = std::move(other.c);
      start = std::exchange(other.start, 0);
    }
    return *this;
  }

  void swap(circulate &other) noexcept(noexcept(std::swap(c, other.c))) {
    std::swap(c, other.c);
    std::swap(start, other.start);
  }

  void rotate(difference_type n) noexcept {
    if (!c.empty()) {
      difference_type sz = static_cast<difference_type>(c.size());
      difference_type pos = static_cast<difference_type>(start);
      pos = (pos + n) % sz;
      if (pos < 0)
        pos += sz;
      start = static_cast<size_type>(pos);
    }
  }

  void set_start(size_type new_start) noexcept {
    if (!c.empty())
      start = new_start % c.size();
  }

  constexpr reference at(difference_type index) {
    return c[circular_index(index)];
  }

  constexpr const_reference at(difference_type index) const {
    return c[circular_index(index)];
  }

  constexpr reference operator[](difference_type index) {
    return c[circular_index(index)];
  }
  constexpr const_reference operator[](difference_type index) const {
    return c[circular_index(index)];
  }

  constexpr reference front() { return c[start]; }
  constexpr const_reference front() const { return c[start]; }
  constexpr reference back() { return c[circular_index(ssize() - 1)]; }
  constexpr const_reference back() const {
    return c[circular_index(ssize() - 1)];
  }

  iterator begin() noexcept { return iterator(this, 0); }
  const_iterator begin() const noexcept { return const_iterator(this, 0); }
  const_iterator cbegin() const noexcept { return const_iterator(this, 0); }
  iterator end() noexcept { return iterator(this, ssize()); }
  const_iterator end() const noexcept { return const_iterator(this, ssize()); }
  const_iterator cend() const noexcept { return const_iterator(this, ssize()); }

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(cend());
  }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(cbegin());
  }

  friend bool operator==(const circulate &lhs, const circulate &rhs) {
    return lhs.c == rhs.c && lhs.start == rhs.start;
  }
};

template <class Container>
circulate(Container, typename Container::size_type = 0)
    -> circulate<typename Container::value_type, Container>;

} // namespace extend

namespace std {
template <class T, class Container>
void swap(
    extend::circulate<T, Container> &lhs,
    extend::circulate<T, Container> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}
} // namespace std
