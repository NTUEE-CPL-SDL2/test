#pragma once // tuple.hpp

#include <compare>
#include <type_traits>
#include <utility>

#include "allocator.hpp"
#include "ignore.hpp"
#include "swap.hpp"
#include "tuple-using.hpp"

namespace mystd {

template <class First, class... Rest> struct _first_type {
  using type = First;
};

template <class... Ts> using _first_type_t = typename _first_type<Ts...>::type;

template <typename T> struct _is_tuple : std::false_type {};

template <typename T> inline constexpr bool _is_tuple_v = _is_tuple<T>::value;

template <std::size_t I, class... Types> struct _tuple_impl;

template <std::size_t I> struct _tuple_impl<I> {
  template <std::size_t J> using element_type = void;

  constexpr _tuple_impl() {}
  constexpr _tuple_impl(const _tuple_impl &) noexcept {}
  constexpr _tuple_impl(_tuple_impl &&) noexcept {}

  template <class Alloc>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &) noexcept {}

  template <class Alloc>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &,
                        const _tuple_impl &) noexcept {}

  template <class Alloc>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &,
                        _tuple_impl &&) noexcept {}

  constexpr _tuple_impl &operator=(const _tuple_impl &) noexcept {
    return *this;
  }

  constexpr _tuple_impl &operator=(_tuple_impl &&) noexcept { return *this; }

  void swap(_tuple_impl &other) {}
};

template <std::size_t I, class Head, class... Tail>
struct _tuple_impl<I, Head, Tail...> : public _tuple_impl<I + 1, Tail...> {
  using base = _tuple_impl<I + 1, Tail...>;
  Head head;

  template <std::size_t J>
  using element_type =
      std::conditional_t<J == I, Head, typename base::template element_type<J>>;

  constexpr _tuple_impl()
    requires(std::is_default_constructible_v<Head> &&
             (std::is_default_constructible_v<Tail> && ...))
      : base(), head() {}

  explicit((!std::is_convertible_v<const Head &, Head> ||
            (!std::is_convertible_v<const Tail &, Tail> ||
             ...))) constexpr _tuple_impl(const Head &arg, const Tail &...args)
    requires std::is_constructible_v<Head, const Head &> &&
                 (std::is_constructible_v<Tail, const Tail &> && ...)
      : base(args...), head(arg) {}

  template <class UHead, class... UTail>
  explicit((!std::is_convertible_v<UHead, Head> ||
            (!std::is_convertible_v<UTail, Tail> ||
             ...))) constexpr _tuple_impl(UHead &&arg, UTail &&...args)
    requires(sizeof...(Tail) == sizeof...(UTail)) &&
                std::is_constructible_v<Head, UHead> &&
                (std::is_constructible_v<Tail, UTail> && ...)
      : base(std::forward<UTail>(args)...), head(std::forward<UHead>(arg)) {}

  template <typename Alloc>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &alloc)
    requires std::is_default_constructible_v<Head> &&
                 (std::is_default_constructible_v<Tail> && ...)
      : base(mystd::allocator_arg, alloc), head() {}

  template <typename Alloc>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &alloc,
                        const Head &arg, const Tail &...args)
    requires std::is_constructible_v<Head, const Head &> &&
                 (std::is_constructible_v<Tail, const Tail &> && ...)
      : base(mystd::allocator_arg, alloc, args...), head(arg) {}

  template <typename Alloc, class UHead, class... UTail>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &alloc, UHead &&arg,
                        UTail &&...args)
    requires(sizeof...(Tail) == sizeof...(UTail)) &&
                std::is_constructible_v<Head, UHead> &&
                (std::is_constructible_v<Tail, UTail> && ...)
      : base(mystd::allocator_arg, alloc, std::forward<UTail>(args)...),
        head(std::forward<UHead>(arg)) {}

  constexpr _tuple_impl(const _tuple_impl &other)
    requires(std::is_copy_constructible_v<Head> &&
             std::is_copy_constructible_v<base>)
      : base(static_cast<const base &>(other)), head(other.head) {}

  constexpr _tuple_impl(_tuple_impl &&other)
    requires(std::is_move_constructible_v<Head> &&
             std::is_move_constructible_v<base>)
      : base(std::move(static_cast<base &>(other))),
        head(std::move(other.head)) {}

  template <class Alloc>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &alloc,
                        const _tuple_impl &other)
    requires(std::is_copy_constructible_v<Head> &&
             std::is_copy_constructible_v<base>)
      : base(mystd::allocator_arg, alloc, static_cast<const base &>(other)),
        head(other.head) {}

  template <class Alloc>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &alloc,
                        _tuple_impl &&other)
    requires(std::is_move_constructible_v<Head> &&
             std::is_move_constructible_v<base>)
      : base(mystd::allocator_arg, alloc,
             std::move(static_cast<base &>(other))),
        head(std::move(other.head)) {}

  template <class Alloc, class... UTypes>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &alloc,
                        const _tuple_impl<I, UTypes...> &other)
    requires(sizeof...(Tail) + 1 == sizeof...(UTypes)) &&
                std::is_constructible_v<Head,
                                        const typename std::remove_cvref_t<
                                            _first_type_t<UTypes...>> &> &&
                std::is_constructible_v<
                    base, const typename _tuple_impl<I, UTypes...>::base &>
      : base(mystd::allocator_arg, alloc,
             static_cast<const typename _tuple_impl<I, UTypes...>::base &>(
                 other)),
        head(other.head) {}

  template <class Alloc, class... UTypes>
  constexpr _tuple_impl(mystd::allocator_arg_t, const Alloc &alloc,
                        _tuple_impl<I, UTypes...> &&other)
    requires(sizeof...(Tail) + 1 == sizeof...(UTypes)) &&
                std::is_constructible_v<
                    Head, std::remove_cvref_t<_first_type_t<UTypes...>>> &&
                std::is_constructible_v<
                    base, typename _tuple_impl<I, UTypes...>::base>
      : base(mystd::allocator_arg, alloc,
             std::move(static_cast<typename _tuple_impl<I, UTypes...>::base &>(
                 other))),
        head(std::move(other.head)) {}

  constexpr _tuple_impl &operator=(const _tuple_impl &other)
    requires std::is_copy_assignable_v<Head> && std::is_copy_assignable_v<base>
  {
    head = other.head;
    base::operator=(other);
    return *this;
  }

  constexpr _tuple_impl &operator=(_tuple_impl &&other) noexcept(
      std::is_nothrow_move_assignable_v<Head> &&
      std::is_nothrow_move_assignable_v<base>)
    requires(std::is_move_assignable_v<Head> && std::is_move_assignable_v<base>)
  {
    head = std::move(other.head);
    base::operator=(std::move(other));
    return *this;
  }

  template <class UHead, class... UTail>
  constexpr _tuple_impl &operator=(const _tuple_impl<I, UHead, UTail...> &other)
    requires(sizeof...(Tail) == sizeof...(UTail)) &&
            std::is_assignable_v<Head &, const UHead &> &&
            std::is_assignable_v<base &, const _tuple_impl<I + 1, UTail...> &>
  {
    head = other.head;
    base::operator=(static_cast<const _tuple_impl<I + 1, UTail...> &>(other));
    return *this;
  }

  template <class UHead, class... UTail>
  constexpr _tuple_impl &
  operator=(_tuple_impl<I, UHead, UTail...> &&other) noexcept(
      std::is_nothrow_assignable_v<Head &, UHead &&> &&
      std::is_nothrow_assignable_v<base &, _tuple_impl<I + 1, UTail...> &&>)
    requires(sizeof...(Tail) == sizeof...(UTail)) &&
            std::is_assignable_v<Head &, UHead &&> &&
            std::is_assignable_v<base &, _tuple_impl<I + 1, UTail...> &&>
  {
    head = std::move(other.head);
    base::operator=(
        static_cast<_tuple_impl<I + 1, UTail...> &&>(std::move(other)));
    return *this;
  }

  constexpr void swap(_tuple_impl &other) noexcept(
      std::is_nothrow_swappable_v<Head> &&
      noexcept(std::declval<base &>().swap(std::declval<base &>()))) {
    mystd::swap(head, other.head);
    static_cast<base &>(*this).swap(static_cast<base &>(other));
  }

  template <std::size_t J>
  constexpr decltype(auto) get() &
    requires(J >= I && J <= I + sizeof...(Tail))
  {
    if constexpr (I == J) {
      return (head);
    } else {
      return static_cast<base &>(*this).template get<J>();
    }
  }

  template <std::size_t J>
  constexpr decltype(auto) get() const &
    requires(J >= I && J <= I + sizeof...(Tail))
  {
    if constexpr (I == J) {
      return (head);
    } else {
      return static_cast<const base &>(*this).template get<J>();
    }
  }

  template <std::size_t J>
  constexpr decltype(auto) get() &&
    requires(J >= I && J <= I + sizeof...(Tail))
  {
    if constexpr (I == J) {
      return std::move(head);
    } else {
      return static_cast<base &&>(*this).template get<J>();
    }
  }

  template <std::size_t J>
  constexpr decltype(auto) get() const &&
    requires(J >= I && J <= I + sizeof...(Tail))
  {
    if constexpr (I == J) {
      return std::move(head);
    } else {
      return static_cast<const base &&>(*this).template get<J>();
    }
  }
};

template <class... Types> class tuple {
public:
  _tuple_impl<0, Types...> _impl;

  constexpr tuple()
    requires(std::is_default_constructible_v<Types> && ...)
      : _impl() {}

  explicit((!std::is_convertible_v<const Types &, Types> ||
            ...)) constexpr tuple(const Types &...args)
    requires(sizeof...(Types) >= 1) &&
            (std::is_copy_constructible_v<Types> && ...)
      : _impl(args...) {}

  template <class... UTypes>
  explicit((!std::is_convertible_v<UTypes, Types> ||
            ...)) constexpr tuple(UTypes &&...args)
    requires(sizeof...(Types) >= 1) &&
            (sizeof...(Types) == sizeof...(UTypes)) &&
            (std::is_constructible_v<Types, UTypes> && ...) &&
            (sizeof...(Types) > 1 ||
             !mystd::_is_tuple_v<
                 std::remove_cvref_t<_first_type_t<UTypes...>>>) &&
            (sizeof...(Types) > 3 ||
             !std::is_same_v<std::remove_cvref_t<_first_type_t<UTypes...>>,
                             mystd::allocator_arg_t> ||
             std::is_same_v<_first_type_t<Types...>, mystd::allocator_arg_t>)
      : _impl(std::forward<UTypes>(args)...) {}

  template <class... UTypes>
  explicit((!std::is_convertible_v<Types, const UTypes &> ||
            ...)) constexpr tuple(const tuple<UTypes...> &other)
    requires(sizeof...(Types) == sizeof...(UTypes)) &&
            (std::is_constructible_v<Types, const UTypes &> && ...) &&
            (!(sizeof...(Types) == 1 &&
               ((std::is_convertible_v<UTypes, Types> || ...) ||
                (std::is_constructible_v<Types, UTypes> || ...) ||
                (std::is_same_v<Types, UTypes> || ...))))
      : _impl(other._impl) {}

  template <class... UTypes>
  explicit((!std::is_convertible_v<Types, UTypes &&> ||
            ...)) constexpr tuple(tuple<UTypes...> &&other)
    requires(sizeof...(Types) == sizeof...(UTypes)) &&
            (std::is_constructible_v<Types, UTypes &&> && ...) &&
            (!(sizeof...(Types) == 1 &&
               ((std::is_convertible_v<UTypes, Types> || ...) ||
                (std::is_constructible_v<Types, UTypes> || ...) ||
                (std::is_same_v<Types, UTypes> || ...))))
      : _impl(std::move(other._impl)) {}

  template <class U1, class U2>
  explicit(
      (!std::is_convertible_v<
           U1, typename _tuple_impl<0, Types...>::template element_type<0>> ||
       !std::is_convertible_v<
           U2, typename _tuple_impl<0, Types...>::template element_type<
                   1>>)) constexpr tuple(const std::pair<U1, U2> &p)
    requires(sizeof...(Types) == 2) &&
            std::is_constructible_v<
                typename _tuple_impl<0, Types...>::template element_type<0>,
                U1> &&
            std::is_constructible_v<
                typename _tuple_impl<0, Types...>::template element_type<1>, U2>
      : _impl(p.first, p.second) {}

  template <class U1, class U2>
  explicit((
      !std::is_convertible_v<
          U1 &&, typename _tuple_impl<0, Types...>::template element_type<0>> ||
      !std::is_convertible_v<
          U2 &&, typename _tuple_impl<0, Types...>::template element_type<
                     1>>)) constexpr tuple(std::pair<U1, U2> &&p)
    requires(sizeof...(Types) == 2) &&
            std::is_constructible_v<
                typename _tuple_impl<0, Types...>::template element_type<0>,
                U1 &&> &&
            std::is_constructible_v<
                typename _tuple_impl<0, Types...>::template element_type<1>,
                U2 &&>
      : _impl(std::move(p.first), std::move(p.second)) {}

  tuple(const tuple &other) : _impl(other._impl) {}

  tuple(tuple &&other) : _impl(std::move(other._impl)) {}

  template <class Alloc>
  constexpr tuple(mystd::allocator_arg_t, const Alloc &a)
    requires(std::is_default_constructible_v<Types> && ...)
      : _impl(mystd::allocator_arg, a) {}

  template <class Alloc>
  explicit((!std::is_convertible_v<const Types &, Types> ||
            ...)) constexpr tuple(mystd::allocator_arg_t, const Alloc &a,
                                  const Types &...args)
    requires(sizeof...(Types) >= 1) &&
            (std::is_copy_constructible_v<Types> && ...)
      : _impl(mystd::allocator_arg, a, args...) {}

  template <class Alloc, class... UTypes>
  explicit((!std::is_convertible_v<UTypes, Types> ||
            ...)) constexpr tuple(mystd::allocator_arg_t, const Alloc &a,
                                  UTypes &&...args)
    requires(sizeof...(Types) >= 1) &&
            (sizeof...(Types) == sizeof...(UTypes)) &&
            (std::is_constructible_v<Types, UTypes> && ...) &&
            (sizeof...(Types) > 1 ||
             !mystd::_is_tuple_v<std::remove_cvref_t<_first_type_t<UTypes...>>>)
      : _impl(mystd::allocator_arg, a, std::forward<UTypes>(args)...) {}

  template <class Alloc, class... UTypes>
  explicit((!std::is_convertible_v<Types, const UTypes &> ||
            ...)) constexpr tuple(mystd::allocator_arg_t, const Alloc &a,
                                  const tuple<UTypes...> &other)
    requires(sizeof...(Types) == sizeof...(UTypes)) &&
            (std::is_constructible_v<Types, const UTypes &> && ...) &&
            (!(sizeof...(Types) == 1 &&
               ((std::is_convertible_v<UTypes, Types> || ...) ||
                (std::is_constructible_v<Types, UTypes> || ...) ||
                (std::is_same_v<Types, UTypes> || ...))))
      : _impl(mystd::allocator_arg, a, other._impl) {}

  template <class Alloc, class... UTypes>
  explicit((!std::is_convertible_v<Types, UTypes &&> ||
            ...)) constexpr tuple(mystd::allocator_arg_t, const Alloc &a,
                                  tuple<UTypes...> &&other)
    requires(sizeof...(Types) == sizeof...(UTypes)) &&
            (std::is_constructible_v<Types, UTypes &&> && ...) &&
            (!(sizeof...(Types) == 1 &&
               ((std::is_convertible_v<UTypes, Types> || ...) ||
                (std::is_constructible_v<Types, UTypes> || ...) ||
                (std::is_same_v<Types, UTypes> || ...))))
      : _impl(mystd::allocator_arg, a, std::move(other._impl)) {}

  template <class Alloc, class U1, class U2>
  explicit(
      (!std::is_convertible_v<
           U1, typename _tuple_impl<0, Types...>::template element_type<0>> ||
       !std::is_convertible_v<
           U2, typename _tuple_impl<0, Types...>::template element_type<
                   1>>)) constexpr tuple(mystd::allocator_arg_t, const Alloc &a,
                                         const std::pair<U1, U2> &p)
    requires(sizeof...(Types) == 2) &&
            std::is_constructible_v<
                typename _tuple_impl<0, Types...>::template element_type<0>,
                U1> &&
            std::is_constructible_v<
                typename _tuple_impl<0, Types...>::template element_type<1>, U2>
      : _impl(mystd::allocator_arg, a, p.first, p.second) {}

  template <class Alloc, class U1, class U2>
  explicit((
      !std::is_convertible_v<
          U1 &&, typename _tuple_impl<0, Types...>::template element_type<0>> ||
      !std::is_convertible_v<
          U2 &&, typename _tuple_impl<0, Types...>::template element_type<
                     1>>)) constexpr tuple(mystd::allocator_arg_t,
                                           const Alloc &a,
                                           std::pair<U1, U2> &&p)
    requires(sizeof...(Types) == 2) &&
            std::is_constructible_v<
                typename _tuple_impl<0, Types...>::template element_type<0>,
                U1 &&> &&
            std::is_constructible_v<
                typename _tuple_impl<0, Types...>::template element_type<1>,
                U2 &&>
      : _impl(mystd::allocator_arg, a, std::move(p.first),
              std::move(p.second)) {}

  template <class Alloc>
  tuple(mystd::allocator_arg_t, const Alloc &a, const tuple &other)
      : _impl(mystd::allocator_arg, a, other._impl) {}

  template <class Alloc>
  tuple(mystd::allocator_arg_t, const Alloc &a, tuple &&other)
      : _impl(mystd::allocator_arg, a, std::move(other._impl)) {}

  constexpr tuple &operator=(const tuple &other)
    requires(std::is_copy_assignable_v<Types> && ...)
  {
    _impl = other._impl;
    return *this;
  }

  constexpr tuple &operator=(tuple &&other) noexcept(
      (std::is_nothrow_move_assignable_v<Types> && ...))
    requires(std::is_move_assignable_v<Types> && ...)
  {
    _impl = std::move(other._impl);
    return *this;
  }

  template <class... UTypes>
  constexpr tuple &operator=(const tuple<UTypes...> &other)
    requires(sizeof...(Types) == sizeof...(UTypes)) &&
            (std::is_assignable_v<Types &, const UTypes &> && ...)
  {
    _impl = other._impl;
    return *this;
  }

  template <class... UTypes>
  constexpr tuple &operator=(tuple<UTypes...> &&other)
    requires(sizeof...(Types) == sizeof...(UTypes)) &&
            (std::is_assignable_v<Types &, UTypes &&> && ...)
  {
    _impl = std::move(other._impl);
    return *this;
  }

  template <std::size_t J> constexpr decltype(auto) get() & noexcept {
    return _impl.template get<J>();
  }

  template <std::size_t J> constexpr decltype(auto) get() && noexcept {
    return std::move(_impl).template get<J>();
  }

  template <std::size_t J> constexpr decltype(auto) get() const & noexcept {
    return _impl.template get<J>();
  }

  template <std::size_t J> constexpr decltype(auto) get() const && noexcept {
    return std::move(_impl).template get<J>();
  }

  template <class E1, class E2>
  constexpr tuple &operator=(const std::pair<E1, E2> &p)
    requires(sizeof...(Types) == 2) &&
            std::is_assignable_v<decltype(this -> get<0>()), const E1 &> &&
            std::is_assignable_v<decltype(this->get<1>()), const E2 &>
  {
    this->get<0>() = p.first;
    this->get<1>() = p.second;
    return *this;
  }

  template <class E1, class E2>
  constexpr tuple &operator=(std::pair<E1, E2> &&p) noexcept(
      std::is_nothrow_assignable_v<decltype(this->get<0>()), E1 &&> &&
      std::is_nothrow_assignable_v<decltype(this->get<1>()), E2 &&>)
    requires(sizeof...(Types) == 2) &&
            std::is_assignable_v<decltype(this -> get<0>()), E1 &&> &&
            std::is_assignable_v<decltype(this->get<1>()), E2 &&>
  {
    this->get<0>() = std::move(p.first);
    this->get<1>() = std::move(p.second);
    return *this;
  }

  constexpr void swap(tuple &other) noexcept(
      noexcept((std::is_nothrow_swappable_v<Types> && ...))) {
    _impl.swap(other._impl);
  }

private:
  template <std::size_t... I>
  static constexpr bool tuple_equal_impl(const tuple &a, const tuple &b,
                                         std::index_sequence<I...>) {
    return ((a.get<I>() == b.get<I>()) && ...);
  }

public:
  friend constexpr bool operator==(const tuple &a, const tuple &b) {
    return tuple_equal_impl(a, b, std::make_index_sequence<sizeof...(Types)>{});
  }

private:
  template <std::size_t... I>
  static constexpr auto tuple_three_way_impl(const tuple &a, const tuple &b,
                                             std::index_sequence<I...>)
      -> std::common_comparison_category_t<decltype(a.get<I>() <=>
                                                    b.get<I>())...> {
    using result_t = std::common_comparison_category_t<decltype(a.get<I>() <=>
                                                                b.get<I>())...>;
    result_t result = result_t::equal;
    ((result = (result != 0 ? result : (a.get<I>() <=> b.get<I>()))), ...);
    return result;
  }

public:
  friend constexpr auto operator<=>(const tuple &a, const tuple &b) {
    return tuple_three_way_impl(a, b,
                                std::make_index_sequence<sizeof...(Types)>{});
  }
};

template <> class tuple<> {
public:
  constexpr tuple() noexcept = default;

  constexpr tuple(const tuple &) noexcept = default;
  constexpr tuple(tuple &&) noexcept = default;

  constexpr tuple &operator=(const tuple &) noexcept = default;
  constexpr tuple &operator=(tuple &&) noexcept = default;

  constexpr void swap(tuple &) noexcept {}

  friend constexpr bool operator==(const tuple &, const tuple &) noexcept {
    return true;
  }

  friend constexpr auto operator<=>(const tuple &, const tuple &) noexcept {
    return std::strong_ordering::equal;
  }
};

template <class... UTypes> tuple(UTypes...) -> tuple<UTypes...>;

template <class T1, class T2> tuple(std::pair<T1, T2>) -> tuple<T1, T2>;

template <class Alloc, class... UTypes>
tuple(std::allocator_arg_t, Alloc, UTypes...) -> tuple<UTypes...>;

template <class Alloc, class T1, class T2>
tuple(std::allocator_arg_t, Alloc, std::pair<T1, T2>) -> tuple<T1, T2>;

template <class Alloc, class... UTypes>
tuple(std::allocator_arg_t, Alloc, tuple<UTypes...>) -> tuple<UTypes...>;

template <typename... Ts>
struct _is_tuple<mystd::tuple<Ts...>> : std::true_type {};

template <std::size_t I, class... Types>
constexpr decltype(auto) get(tuple<Types...> &t) noexcept {
  return t.template get<I>();
}

template <std::size_t I, class... Types>
constexpr decltype(auto) get(tuple<Types...> &&t) noexcept {
  return std::move(t).template get<I>();
}

template <std::size_t I, class... Types>
constexpr decltype(auto) get(const tuple<Types...> &t) noexcept {
  return t.template get<I>();
}

template <std::size_t I, class... Types>
constexpr decltype(auto) get(const tuple<Types...> &&t) noexcept {
  return std::move(t).template get<I>();
}

template <class T, class... Types> struct _count_of_type;

template <class T>
struct _count_of_type<T> : std::integral_constant<std::size_t, 0> {};

template <class T, class Head, class... Tail>
struct _count_of_type<T, Head, Tail...>
    : std::integral_constant<std::size_t,
                             (std::is_same_v<T, Head> ? 1 : 0) +
                                 _count_of_type<T, Tail...>::value> {};

template <class T, class... Types>
inline constexpr std::size_t _count_of_type_v =
    _count_of_type<T, Types...>::value;

template <class T, class... Types> struct _index_of_type;

template <class T, class... Tail>
struct _index_of_type<T, T, Tail...> : std::integral_constant<std::size_t, 0> {
};

template <class T, class Head, class... Tail>
struct _index_of_type<T, Head, Tail...>
    : std::integral_constant<std::size_t,
                             1 + _index_of_type<T, Tail...>::value> {};

template <class T> struct _index_of_type<T> {};

template <class T, class... Types>
inline constexpr std::size_t _index_of_type_v =
    _index_of_type<T, Types...>::value;

template <class T, class... Types>
constexpr decltype(auto) get(mystd::tuple<Types...> &t) noexcept
  requires(_count_of_type_v<T, Types...> == 1)
{
  return get<_index_of_type_v<T, Types...>>(t);
}

template <class T, class... Types>
constexpr decltype(auto) get(mystd::tuple<Types...> &&t) noexcept
  requires(_count_of_type_v<T, Types...> == 1)
{
  return get<_index_of_type_v<T, Types...>>(std::move(t));
}

template <class T, class... Types>
constexpr decltype(auto) get(const mystd::tuple<Types...> &t) noexcept
  requires(_count_of_type_v<T, Types...> == 1)
{
  return get<_index_of_type_v<T, Types...>>(t);
}

template <class T, class... Types>
constexpr decltype(auto) get(const mystd::tuple<Types...> &&t) noexcept
  requires(_count_of_type_v<T, Types...> == 1)
{
  return get<_index_of_type_v<T, Types...>>(std::move(t));
}

template <class... Types> constexpr auto make_tuple(Types &&...args) {
  return mystd::tuple<typename std::decay<Types>::type...>(
      std::forward<Types>(args)...);
}

template <class... Types>
constexpr mystd::tuple<Types &...> tie(Types &...args) noexcept {
  return mystd::tuple<Types &...>(args...);
}

template <class... Types>
constexpr tuple<Types &&...> forward_as_tuple(Types &&...args) noexcept {
  return tuple<Types &&...>(std::forward<Types>(args)...);
}

template <class... Tuples> struct _tuple_cat_type;

template <> struct _tuple_cat_type<> {
  using type = mystd::tuple<>;
};

template <class... Ts> struct _tuple_cat_type<mystd::tuple<Ts...>> {
  using type = mystd::tuple<Ts...>;
};

template <class... Ts1, class... Ts2, class... Rest>
struct _tuple_cat_type<mystd::tuple<Ts1...>, mystd::tuple<Ts2...>, Rest...> {
  using type =
      typename _tuple_cat_type<mystd::tuple<Ts1..., Ts2...>, Rest...>::type;
};

template <class... Tuples>
using _tuple_cat_type_t = typename _tuple_cat_type<Tuples...>::type;

template <class T1, class T2, std::size_t... I1, std::size_t... I2>
constexpr auto _tuple_cat_two_impl(T1 &&t1, T2 &&t2, std::index_sequence<I1...>,
                                   std::index_sequence<I2...>) {
  return mystd::tuple{mystd::get<I1>(std::forward<T1>(t1))...,
                      mystd::get<I2>(std::forward<T2>(t2))...};
}

template <class T1, class T2> constexpr auto _tuple_cat_two(T1 &&t1, T2 &&t2) {
  return _tuple_cat_two_impl(
      std::forward<T1>(t1), std::forward<T2>(t2),
      std::make_index_sequence<
          std::tuple_size<std::remove_cvref_t<T1>>::value>{},
      std::make_index_sequence<
          std::tuple_size<std::remove_cvref_t<T2>>::value>{});
}

template <class T> constexpr auto _tuple_cat_impl(T &&t) {
  return std::forward<T>(t);
}

template <class T1, class T2, class... Rest>
constexpr auto _tuple_cat_impl(T1 &&t1, T2 &&t2, Rest &&...rest) {
  auto first = _tuple_cat_two(std::forward<T1>(t1), std::forward<T2>(t2));
  return _tuple_cat_impl(std::move(first), std::forward<Rest>(rest)...);
}

template <class... Tuples> constexpr auto tuple_cat(Tuples &&...tpls) {
  return _tuple_cat_impl(std::forward<Tuples>(tpls)...);
}

template <class... Types>
constexpr void
swap(mystd::tuple<Types...> &lhs,
     mystd::tuple<Types...> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

template <class... Types, class Alloc>
struct uses_allocator<std::tuple<Types...>, Alloc> : std::true_type {};

} // namespace mystd

namespace std {

template <class... Types>
struct tuple_size<mystd::tuple<Types...>>
    : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <class Head, class... Tail>
struct tuple_element<0, mystd::tuple<Head, Tail...>> {
  using type = Head;
};

template <std::size_t I, class Head, class... Tail>
struct tuple_element<I, mystd::tuple<Head, Tail...>>
    : tuple_element<I - 1, mystd::tuple<Tail...>> {};

} // namespace std
