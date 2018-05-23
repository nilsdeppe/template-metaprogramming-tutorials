/*
Copyright 2017 Nils Deppe
Distributed under the Boost Software License.

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

/*
 * Compile the code using:
 * clang++ -std=c++14 ./for_constexpr.cpp
 * g++ -std=c++14 ./for_constexpr.cpp
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <utility>

using ssize_t = typename std::make_signed<std::size_t>::type;

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define ALWAYS_INLINE __forceinline inline
#else
#define ALWAYS_INLINE inline  // :(
#endif

/*!
 * \ingroup UtilitiesGroup
 * \brief Specify the lower and upper bounds in a for_constexpr loop
 *
 * \see for_constexpr for_symm_lower for_symm_upper
 */
template <size_t Lower, size_t Upper>
struct for_bounds {
  static constexpr const size_t lower = Lower;
  static constexpr const size_t upper = Upper;
};

/*!
 * \ingroup UtilitiesGroup
 * \brief Specify the loop index to symmetrize over, the lower bound, and an
 * offset to add to `Index`'s current loop value in a for_constexpr loop. Loops
 * from `Lower` to `Index`'s current loop value plus `Offset`.
 *
 * \see for_constexpr for_bounds for_symm_upper
 */
template <size_t Index, size_t Lower, ssize_t Offset = 0>
struct for_symm_lower {};

/*!
 * \ingroup UtilitiesGroup
 * \brief Specify the loop index to symmetrize over and upper bounds in a
 * for_constexpr loop. Loops from the `Index`'s current loop value to `Upper`.
 *
 * \see for_constexpr for_bounds for_symm_lower
 */
template <size_t Index, size_t Upper>
struct for_symm_upper {};

namespace for_constexpr_detail {
// Provided for implementation to be self-contained
template <bool...>
struct bool_pack;
template <bool... Bs>
using all_true = std::is_same<bool_pack<Bs..., true>, bool_pack<true, Bs...>>;

// Base case
template <size_t Lower, size_t... Is, class F, class... IntegralConstants>
ALWAYS_INLINE constexpr void for_constexpr_impl(
    F&& f, std::index_sequence<Is...> /*meta*/, IntegralConstants&&... v) {
  (void)std::initializer_list<char>{
      ((void)f(v..., std::integral_constant<size_t, Is + Lower>{}), '0')...};
}

// Cases of second last loop
template <size_t Lower, size_t BoundsNextLower, size_t BoundsNextUpper,
          size_t... Is, class F, class... IntegralConstants>
ALWAYS_INLINE constexpr void for_constexpr_impl(
    F&& f, for_bounds<BoundsNextLower, BoundsNextUpper> /*meta*/,
    std::index_sequence<Is...> /*meta*/, IntegralConstants&&... v) {
  static_assert(static_cast<ssize_t>(BoundsNextUpper) -
                        static_cast<ssize_t>(BoundsNextLower) >=
                    0,
                "Cannot make index_sequence of negative size. The upper bound "
                "in for_bounds is smaller than the lower bound.");
  (void)std::initializer_list<char>{
      ((void)for_constexpr_impl<BoundsNextLower>(
           std::forward<F>(f),
           std::make_index_sequence<(  // Safeguard against generating
                                       // index_sequence of size ~ max size_t
               static_cast<ssize_t>(BoundsNextUpper) -
                           static_cast<ssize_t>(BoundsNextLower) <
                       0
                   ? 1
                   : BoundsNextUpper - BoundsNextLower)>{},
           v..., std::integral_constant<size_t, Is + Lower>{}),
       '0')...};
}

template <size_t Lower, size_t BoundsNextIndex, size_t BoundsNextUpper,
          size_t... Is, class F, class... IntegralConstants>
ALWAYS_INLINE constexpr void for_constexpr_impl(
    F&& f, for_symm_upper<BoundsNextIndex, BoundsNextUpper> /*meta*/,
    std::index_sequence<Is...> /*meta*/, IntegralConstants&&... v) {
  static_assert(all_true<(static_cast<ssize_t>(BoundsNextUpper) -
                              static_cast<ssize_t>(std::get<BoundsNextIndex>(
                                  std::make_tuple(IntegralConstants::value...,
                                                  Is + Lower))) >=
                          0)...>::value,
                "Cannot make index_sequence of negative size. You specified an "
                "upper bound in for_symm_upper that is less than the "
                "smallest lower bound in the loop being symmetrized over.");
  (void)std::initializer_list<char>{
      ((void)for_constexpr_impl<std::get<BoundsNextIndex>(
           std::make_tuple(IntegralConstants::value..., Is + Lower))>(
           std::forward<F>(f),
           std::make_index_sequence<(  // Safeguard against generating
                                       // index_sequence of size ~ max size_t
               static_cast<ssize_t>(BoundsNextUpper) -
                           static_cast<ssize_t>(
                               std::get<BoundsNextIndex>(std::make_tuple(
                                   IntegralConstants::value..., Is + Lower))) <
                       0
                   ? 1
                   : BoundsNextUpper -
                         std::get<BoundsNextIndex>(std::make_tuple(
                             IntegralConstants::value..., Is + Lower)))>{},
           v..., std::integral_constant<size_t, Is + Lower>{}),
       '0')...};
}

template <size_t Lower, size_t BoundsNextIndex, size_t BoundsNextLower,
          ssize_t BoundsNextOffset, size_t... Is, class F,
          class... IntegralConstants>
ALWAYS_INLINE constexpr void for_constexpr_impl(
    F&& f,
    for_symm_lower<BoundsNextIndex, BoundsNextLower, BoundsNextOffset> /*meta*/,
    std::index_sequence<Is...> /*meta*/, IntegralConstants&&... v) {
  static_assert(
      all_true<(static_cast<ssize_t>(std::get<BoundsNextIndex>(
                    std::make_tuple(IntegralConstants::value..., Is + Lower))) +
                    BoundsNextOffset - static_cast<ssize_t>(BoundsNextLower) >=
                0)...>::value,
      "Cannot make index_sequence of negative size. You specified a lower "
      "bound in for_symm_lower that is larger than the upper bounds of "
      "the loop being symmetrized over");
  (void)std::initializer_list<char>{
      ((void)for_constexpr_impl<BoundsNextLower>(
           std::forward<F>(f),
           std::make_index_sequence<(  // Safeguard against generating
                                       // index_sequence of size ~ max size_t
               static_cast<ssize_t>(std::get<BoundsNextIndex>(
                   std::make_tuple(IntegralConstants::value..., Is + Lower))) +
                           BoundsNextOffset -
                           static_cast<ssize_t>(BoundsNextLower) <
                       0
                   ? 1
                   : static_cast<size_t>(
                         static_cast<ssize_t>(
                             std::get<BoundsNextIndex>(std::make_tuple(
                                 IntegralConstants::value..., Is + Lower))) +
                         BoundsNextOffset -
                         static_cast<ssize_t>(BoundsNextLower)))>{},
           v..., std::integral_constant<size_t, Is + Lower>{}),
       '0')...};
}

// Handle cases of more than two nested loops
template <size_t Lower, class Bounds1, class... Bounds, size_t BoundsNextLower,
          size_t BoundsNextUpper, size_t... Is, class F,
          class... IntegralConstants>
ALWAYS_INLINE constexpr void for_constexpr_impl(
    F&& f, for_bounds<BoundsNextLower, BoundsNextUpper> /*meta*/,
    std::index_sequence<Is...> /*meta*/, IntegralConstants&&... v) {
  static_assert(static_cast<ssize_t>(BoundsNextUpper) -
                        static_cast<ssize_t>(BoundsNextLower) >=
                    0,
                "Cannot make index_sequence of negative size. The upper bound "
                "in for_bounds is smaller than the lower bound.");

  (void)std::initializer_list<char>{
      ((void)for_constexpr_impl<BoundsNextLower, Bounds...>(
           std::forward<F>(f), Bounds1{},
           std::make_index_sequence<(  // Safeguard against generating
                                       // index_sequence of size ~ max size_t
               static_cast<ssize_t>(BoundsNextUpper) -
                           static_cast<ssize_t>(BoundsNextLower) <
                       0
                   ? 1
                   : BoundsNextUpper - BoundsNextLower)>{},
           v..., std::integral_constant<size_t, Is + Lower>{}),
       '0')...};
}

template <size_t Lower, class Bounds1, class... Bounds, size_t BoundsNextIndex,
          size_t BoundsNextUpper, size_t... Is, class F,
          class... IntegralConstants>
ALWAYS_INLINE constexpr void for_constexpr_impl(
    F&& f, for_symm_upper<BoundsNextIndex, BoundsNextUpper> /*meta*/,
    std::index_sequence<Is...> /*meta*/, IntegralConstants&&... v) {
  static_assert(all_true<(static_cast<ssize_t>(BoundsNextUpper) -
                              static_cast<ssize_t>(std::get<BoundsNextIndex>(
                                  std::make_tuple(IntegralConstants::value...,
                                                  Is + Lower))) >=
                          0)...>::value,
                "Cannot make index_sequence of negative size. You specified an "
                "upper bound in for_symm_upper that is less than the "
                "smallest lower bound in the loop being symmetrized over.");
  (void)std::initializer_list<char>{
      ((void)for_constexpr_impl<std::get<BoundsNextIndex>(
           std::make_tuple(IntegralConstants::value..., Is + Lower))>(
           std::forward<F>(f), Bounds1{},
           std::make_index_sequence<(  // Safeguard against generating
                                       // index_sequence of size ~ max size_t
               static_cast<ssize_t>(BoundsNextUpper) -
                           static_cast<ssize_t>(
                               std::get<BoundsNextIndex>(std::make_tuple(
                                   IntegralConstants::value..., Is + Lower))) <
                       0
                   ? 1
                   : BoundsNextUpper -
                         std::get<BoundsNextIndex>(std::make_tuple(
                             IntegralConstants::value..., Is + Lower)))>{},
           v..., std::integral_constant<size_t, Is + Lower>{}),
       '0')...};
}

template <size_t Lower, class Bounds1, class... Bounds, size_t BoundsNextIndex,
          size_t BoundsNextLower, ssize_t BoundsNextOffset, size_t... Is,
          class F, class... IntegralConstants>
ALWAYS_INLINE constexpr void for_constexpr_impl(
    F&& f,
    for_symm_lower<BoundsNextIndex, BoundsNextLower, BoundsNextOffset> /*meta*/,
    std::index_sequence<Is...> /*meta*/, IntegralConstants&&... v) {
  static_assert(
      all_true<(static_cast<ssize_t>(std::get<BoundsNextIndex>(
                    std::make_tuple(IntegralConstants::value..., Is + Lower))) +
                    BoundsNextOffset - static_cast<ssize_t>(BoundsNextLower) >=
                0)...>::value,
      "Cannot make index_sequence of negative size. You specified a lower "
      "bound in for_symm_lower that is larger than the upper bounds of "
      "the loop being symmetrized over");
  (void)std::initializer_list<char>{
      ((void)for_constexpr_impl<BoundsNextLower>(
           std::forward<F>(f), Bounds1{},
           std::make_index_sequence<(  // Safeguard against generating
                                       // index_sequence of size ~ max size_t
               static_cast<ssize_t>(std::get<BoundsNextIndex>(
                   std::make_tuple(IntegralConstants::value..., Is + Lower))) +
                           BoundsNextOffset -
                           static_cast<ssize_t>(BoundsNextLower) <
                       0
                   ? 1
                   : static_cast<size_t>(
                         static_cast<ssize_t>(
                             std::get<BoundsNextIndex>(std::make_tuple(
                                 IntegralConstants::value..., Is + Lower))) +
                         BoundsNextOffset -
                         static_cast<ssize_t>(BoundsNextLower)))>{},
           v..., std::integral_constant<size_t, Is + Lower>{}),
       '0')...};
}
}  // namespace for_constexpr_detail

/// \cond
template <class Bounds0, class F>
ALWAYS_INLINE constexpr void for_constexpr(F&& f) {
  static_assert(static_cast<ssize_t>(Bounds0::upper) -
                        static_cast<ssize_t>(Bounds0::lower) >=
                    0,
                "Cannot make index_sequence of negative size. The upper bound "
                "in for_bounds is smaller than the lower bound.");
  for_constexpr_detail::for_constexpr_impl<Bounds0::lower>(
      std::forward<F>(f), std::make_index_sequence<(
                              static_cast<ssize_t>(Bounds0::upper) -
                                          static_cast<ssize_t>(Bounds0::lower) <
                                      0
                                  ? 1
                                  : Bounds0::upper - Bounds0::lower)>{});
}
/// \endcond

/*!
 * \ingroup UtilitiesGroup
 * \brief Allows nested constexpr for loops including symmetrizing over loops.
 *
 * \note All upper bounds are exclusive (the comparator is a strict less than,
 * `<`)
 * \note You must loop over non-negative numbers.
 *
 * The `for_bounds` class is used for specifying the lower (first template
 * parameter) and upper (second template parameter) bounds for a single loop.
 * Symmetrizing over loops is supported via the `for_symm_lower` and
 * `for_symm_upper` classes. `for_symm_lower` takes two template parameters (and
 * a third optional one): the integer corresponding to the outer loop to
 * symmetrize over, the lower bound of the loop, and optionally an integer to
 * add to the upper bound obtained from the loop being symmetrized over.
 * This is equivalent to loops of the form:
 *
 * \code
 * for (size_t i = 0; i < Dim; ++i) {
 *   for (size_t j = param1; j < i + param2; ++j) {
 *   }
 * }
 * \endcode
 *
 * `for_symm_upper` takes two template parameters. The first is the integer
 * corresponding to the outer loop being symmetrized over, and the second is the
 * upper bound. This is equivalent to loops of the form
 *
 * \code
 * for (size_t i = 0; i < Dim; ++i) {
 *   for (size_t j = j; j < param1; ++j) {
 *   }
 * }
 * \endcode
 *
 * \example
 * Here are various example use cases of different loop structures. The runtime
 * for loops are shown for comparison. Only the elements that are 1 were mutated
 * by the `for_constexpr`.
 *
 * #### Single loops
 * Single loops are hopefully straightforward.
 * \snippet Test_ForConstexpr.cpp single_loop
 *
 * #### Double loops
 * For double loops we should the four different options for loop symmetries.
 * \snippet Test_ForConstexpr.cpp double_loop
 * \snippet Test_ForConstexpr.cpp double_symm_lower_inclusive
 * \snippet Test_ForConstexpr.cpp double_symm_lower_exclusive
 * \snippet Test_ForConstexpr.cpp double_symm_upper
 *
 * #### Triple loops
 * For triple loops we only show the double symmetrized loops, since the others
 * are very similar to the double loop case.
 * \snippet Test_ForConstexpr.cpp triple_symm_lower_lower
 * \snippet Test_ForConstexpr.cpp triple_symm_upper_lower
 *
 * \see for_bounds for_symm_lower for_symm_upper
 */
template <class Bounds0, class Bounds1, class... Bounds, class F>
ALWAYS_INLINE constexpr void for_constexpr(F&& f) {
  static_assert(static_cast<ssize_t>(Bounds0::upper) -
                        static_cast<ssize_t>(Bounds0::lower) >=
                    0,
                "Cannot make index_sequence of negative size. The upper bound "
                "in for_bounds is smaller than the lower bound.");
  for_constexpr_detail::for_constexpr_impl<Bounds0::lower, Bounds...>(
      std::forward<F>(f), Bounds1{},
      std::make_index_sequence<(
          static_cast<ssize_t>(Bounds0::upper) -
                      static_cast<ssize_t>(Bounds0::lower) <
                  0
              ? 1
              : Bounds0::upper - Bounds0::lower)>{});
}


namespace {
void single_loop() {
  /// [single_loop]
  constexpr size_t array_size = 3;
  std::array<size_t, array_size> values{{0, 0, 0}};
  for_constexpr<for_bounds<0, array_size>>([&values](auto i) { values[i]++; });
  for (size_t i = 0; i < values.size(); ++i) {
    assert(values[i] == 1);
  }
  /// [single_loop]
}

void double_loop() {
  constexpr size_t array_size = 3;
  std::array<std::array<size_t, array_size>, array_size> zero{
      {{{0, 0, 0}}, {{0, 0, 0}}, {{0, 0, 0}}}};
  {  // Check double loop, no symmetry
    /// [double_loop]
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_bounds<0, array_size>>(
        [&values](auto i, auto j) { values[i][j]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        assert(values[i][j] == 1);
      }
    }
    /// [double_loop]
  }
  {  // Check double loop, lower symmetry
    /// [double_symm_lower_exclusive]
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0>>(
        [&values](auto i, auto j) { values[i][j]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        if (j < i) {
          assert(values[i][j] == 1);
        } else {
          assert(values[i][j] == 0);
        }
      }
    }
    /// [double_symm_lower_exclusive]
  }
  {  // Check double loop, lower symmetry, offset = 1
     /// [double_symm_lower_inclusive]
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 1>>(
        [&values](auto i, auto j) { values[i][j]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        if (j <= i) {
          assert(values[i][j] == 1);
        } else {
          assert(values[i][j] == 0);
        }
      }
    }
    /// [double_symm_lower_inclusive]
  }
  {  // Check double loop, upper symmetry
    /// [double_symm_upper]
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_upper<0, array_size>>(
        [&values](auto i, auto j) { values[i][j]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        if (j >= i) {
          assert(values[i][j] == 1);
        } else {
          assert(values[i][j] == 0);
        }
      }
    }
    /// [double_symm_upper]
  }
}

void triple_loop() {
  constexpr size_t array_size = 3;
  std::array<std::array<size_t, array_size>, array_size> zero_2d{
      {{{0, 0, 0}}, {{0, 0, 0}}, {{0, 0, 0}}}};
  std::array<std::array<std::array<size_t, array_size>, array_size>, array_size>
      zero{{zero_2d, zero_2d, zero_2d}};

  {  // no symmetry
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_bounds<0, array_size>,
                  for_bounds<0, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          assert(values[i][j][k] == 1);
        }
      }
    }
  }
  {  // upper symmetric last on first
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_bounds<0, array_size>,
                  for_symm_upper<0, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (k >= i) {
            assert(values[i][j][k] == 1);
          } else {
            assert(values[i][j][k] == 0);
          }
        }
      }
    }
  }
  {  // upper symmetric last on second
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_bounds<0, array_size>,
                  for_symm_upper<1, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (k >= j) {
            assert(values[i][j][k] == 1);
          } else {
            assert(values[i][j][k] == 0);
          }
        }
      }
    }
  }
  {  // upper symmetric second on first
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_upper<0, array_size>,
                  for_bounds<0, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j >= i) {
            assert(values[i][j][k] == 1);
          } else {
            assert(values[i][j][k] == 0);
          }
        }
      }
    }
  }
  {  // upper symmetric second on first, last on first
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_upper<0, array_size>,
                  for_symm_upper<0, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j >= i) {
            if (k >= i) {
              assert(values[i][j][k] == 1);
              continue;
            }
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // upper symmetric second on first, last on second
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_upper<0, array_size>,
                  for_symm_upper<1, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j >= i) {
            if (k >= j) {
              assert(values[i][j][k] == 1);
              continue;
            }
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
}

void triple_loop_lower_symmetric() {
  constexpr size_t array_size = 3;
  std::array<std::array<size_t, array_size>, array_size> zero_2d{
      {{{0, 0, 0}}, {{0, 0, 0}}, {{0, 0, 0}}}};
  std::array<std::array<std::array<size_t, array_size>, array_size>, array_size>
      zero{{zero_2d, zero_2d, zero_2d}};

  {  // lower symmetric last on first, exclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_bounds<0, array_size>,
                  for_symm_lower<0, 0>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (k < i) {
            assert(values[i][j][k] == 1);
          } else {
            assert(values[i][j][k] == 0);
          }
        }
      }
    }
  }
  {  // lower symmetric last on first, inclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_bounds<0, array_size>,
                  for_symm_lower<0, 0, 1>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (k <= i) {
            assert(values[i][j][k] == 1);
          } else {
            assert(values[i][j][k] == 0);
          }
        }
      }
    }
  }
  {  // lower symmetric last on second, exclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_bounds<0, array_size>,
                  for_symm_lower<1, 0>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (k < j) {
            assert(values[i][j][k] == 1);
          } else {
            assert(values[i][j][k] == 0);
          }
        }
      }
    }
  }
  {  // lower symmetric last on second, inclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_bounds<0, array_size>,
                  for_symm_lower<1, 0, 1>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (k <= j) {
            assert(values[i][j][k] == 1);
          } else {
            assert(values[i][j][k] == 0);
          }
        }
      }
    }
  }
  {  // lower symmetric second on first, exclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0>,
                  for_bounds<0, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j < i) {
            assert(values[i][j][k] == 1);
          } else {
            assert(values[i][j][k] == 0);
          }
        }
      }
    }
  }
  {  // lower symmetric second on first, inclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 1>,
                  for_bounds<0, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j <= i) {
            assert(values[i][j][k] == 1);
          } else {
            assert(values[i][j][k] == 0);
          }
        }
      }
    }
  }
  {  // lower symmetric second on first, exclusive, last on first, exclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 0>,
                  for_symm_lower<0, 0, 0>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j < i and k < i) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // lower symmetric second on first, exclusive, last on first, inclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 0>,
                  for_symm_lower<0, 0, 1>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j < i and k <= i) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // lower symmetric second on first, inclusive, last on first, exclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 1>,
                  for_symm_lower<0, 0, 0>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j <= i and k < i) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // lower symmetric second on first, inclusive, last on first, inclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 1>,
                  for_symm_lower<0, 0, 1>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j <= i and k <= i) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }

  {  // lower symmetric second on first, exclusive, last on second, exclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 0>,
                  for_symm_lower<1, 0, 0>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j < i and k < j) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // lower symmetric second on first, exclusive, last on second, inclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 0>,
                  for_symm_lower<1, 0, 1>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j < i and k <= j) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // lower symmetric second on first, inclusive, last on second, exclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 1>,
                  for_symm_lower<1, 0, 0>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j <= i and k < j) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // lower symmetric second on first, inclusive, last on second, inclusive
    /// [triple_symm_lower_lower]
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 1>,
                  for_symm_lower<1, 0, 1>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j <= i and k <= j) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
    /// [triple_symm_lower_lower]
  }
}

void triple_loop_mixed() {
  constexpr size_t array_size = 3;
  std::array<std::array<size_t, array_size>, array_size> zero_2d{
      {{{0, 0, 0}}, {{0, 0, 0}}, {{0, 0, 0}}}};
  std::array<std::array<std::array<size_t, array_size>, array_size>, array_size>
      zero{{zero_2d, zero_2d, zero_2d}};
  {  // lower/upper symmetric second on first, exclusive, last on first, upper
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0>,
                  for_symm_upper<0, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j < i and k >= i) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // lower/upper symmetric second on first, inclusive, last on first, upper
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 1>,
                  for_symm_upper<0, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j <= i and k >= i) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // lower/upper symmetric second on first, exclusive, last on second, upper
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0>,
                  for_symm_upper<1, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j < i and k >= j) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // lower/upper symmetric second on first, inclusive, last on second, upper
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_lower<0, 0, 1>,
                  for_symm_upper<1, array_size>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j <= i and k >= j) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // upper/lower symmetric second on first, upper, last on first, exclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_upper<0, array_size>,
                  for_symm_lower<0, 0>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j >= i and k < i) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // upper/lower symmetric second on first, upper, last on first, inclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_upper<0, array_size>,
                  for_symm_lower<0, 0, 1>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j >= i and k <= i) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // upper/lower symmetric second on first, upper, last on second, exclusive
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_upper<0, array_size>,
                  for_symm_lower<1, 0>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j >= i and k < j) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
  }
  {  // upper/lower symmetric second on first, upper, last on second, inclusive
    /// [triple_symm_upper_lower]
    auto values = zero;
    for_constexpr<for_bounds<0, array_size>, for_symm_upper<0, array_size>,
                  for_symm_lower<1, 0, 1>>(
        [&values](auto i, auto j, auto k) { values[i][j][k]++; });
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < values.size(); ++j) {
        for (size_t k = 0; k < values.size(); ++k) {
          if (j >= i and k <= j) {
            assert(values[i][j][k] == 1);
            continue;
          }
          assert(values[i][j][k] == 0);
        }
      }
    }
    /// [triple_symm_upper_lower]
  }
}
}  // namespace

int main() {
  single_loop();
  double_loop();
  triple_loop();
  triple_loop_mixed();
  triple_loop_lower_symmetric();
}
