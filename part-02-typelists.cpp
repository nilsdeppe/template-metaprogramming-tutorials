
/*==================================================================================================
  Copyright (c) 2017 Nils Deppe
  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
=================================================================================================**/

#include <type_traits>

template <typename... Ts>
struct typelist {};

struct no_such_type {};

namespace detail {
template <typename Seq>
struct size_impl;

template <template <typename...> class Seq, typename... Ts>
struct size_impl<Seq<Ts...>> {
  using type = std::integral_constant<std::size_t, sizeof...(Ts)>;
};
}  // namespace detail

template <typename Seq>
using size = typename detail::size_impl<Seq>::type;

static_assert(size<typelist<double, char, bool, double>>::value == 4,
              "The implementation of size is bad");

namespace detail {
template <typename Seq>
struct front_impl;

template <template <typename...> class Seq, typename T, typename... Ts>
struct front_impl<Seq<T, Ts...>> {
  using type = T;
};

template <template <typename...> class Seq>
struct front_impl<Seq<>> {
  using type = no_such_type;
};
}  // namespace detail

template <typename Seq>
using front = typename detail::front_impl<Seq>::type;

static_assert(
    std::is_same<front<typelist<double, char, bool, double>>, double>::value,
    "The implementation of front is bad");

static_assert(std::is_same<front<typelist<>>, no_such_type>::value,
              "The implementation of front for an empty list is bad");

namespace detail {
template <typename Seq>
struct pop_front_impl;

template <template <typename...> class Seq, typename T, typename... Ts>
struct pop_front_impl<Seq<T, Ts...>> {
  using type = Seq<Ts...>;
};

template <template <typename...> class Seq>
struct pop_front_impl<Seq<>> {
  using type = no_such_type;
};
}  // namespace detail

template <typename Seq>
using pop_front = typename detail::pop_front_impl<Seq>::type;

static_assert(std::is_same<pop_front<typelist<double, char, bool, double>>,
                           typelist<char, bool, double>>::value,
              "The implementation of pop_front is bad");

static_assert(std::is_same<pop_front<typelist<>>, no_such_type>::value,
              "The implementation of pop_front for an empty list is bad");

namespace detail {
template <typename Seq, typename T>
struct push_front_impl;

template <template <typename...> class Seq, typename T, typename... Ts>
struct push_front_impl<Seq<Ts...>, T> {
  using type = Seq<T, Ts...>;
};
}  // namespace detail

template <typename Seq, typename T>
using push_front = typename detail::push_front_impl<Seq, T>::type;

static_assert(
    std::is_same<push_front<typelist<double, char, bool, double>, char>,
                 typelist<char, double, char, bool, double>>::value,
    "The implementation of push_front is bad");

namespace detail {
template <typename Seq, typename T>
struct push_back_impl;

template <template <typename...> class Seq, typename T, typename... Ts>
struct push_back_impl<Seq<Ts...>, T> {
  using type = Seq<Ts..., T>;
};
}  // namespace detail

template <typename Seq, typename T>
using push_back = typename detail::push_back_impl<Seq, T>::type;

static_assert(
    std::is_same<push_back<typelist<double, char, bool, double>, char>,
                 typelist<double, char, bool, double, char>>::value,
    "The implementation of push_back is bad");

int main() {}
