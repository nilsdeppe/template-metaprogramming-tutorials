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
 * clang++ -std=c++14 ./overloaded-lambda.cpp
 * g++ -std=c++14 ./overloaded-lambda.cpp
 */

#include <iostream>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <vector>

template <class... Fs>
struct overloader;

template <class F1, class F2, class F3, class F4, class F5, class F6, class F7,
          class F8, class... Fs>
struct overloader<F1, F2, F3, F4, F5, F6, F7, F8, Fs...>
    : F1, F2, F3, F4, F5, F6, F7, F8, overloader<Fs...> {
  constexpr overloader(F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, F6 f6, F7 f7, F8 f8,
                       Fs... fs)
      : F1(std::move(f1)),
        F2(std::move(f2)),
        F3(std::move(f3)),
        F4(std::move(f4)),
        F5(std::move(f5)),
        F6(std::move(f6)),
        F7(std::move(f7)),
        F8(std::move(f8)),
        overloader<Fs...>(std::move(fs)...) {}

  using F1::operator();
  using F2::operator();
  using F3::operator();
  using F4::operator();
  using F5::operator();
  using F6::operator();
  using F7::operator();
  using F8::operator();
  using overloader<Fs...>::operator();
};

template <class F1, class F2, class F3, class F4, class... Fs>
struct overloader<F1, F2, F3, F4, Fs...> : F1, F2, F3, F4, overloader<Fs...> {
  constexpr overloader(F1 f1, F2 f2, F3 f3, F4 f4, Fs... fs)
      : F1(std::move(f1)),
        F2(std::move(f2)),
        F3(std::move(f3)),
        F4(std::move(f4)),
        overloader<Fs...>(std::move(fs)...) {}

  using F1::operator();
  using F2::operator();
  using F3::operator();
  using F4::operator();
  using overloader<Fs...>::operator();
};

template <class F1, class F2, class... Fs>
struct overloader<F1, F2, Fs...> : F1, F2, overloader<Fs...> {
  constexpr overloader(F1 f1, F2 f2, Fs... fs)
      : F1(std::move(f1)),
        F2(std::move(f2)),
        overloader<Fs...>(std::move(fs)...) {}

  using F1::operator();
  using F2::operator();
  using overloader<Fs...>::operator();
};

template <class F>
struct overloader<F> : F {
  constexpr explicit overloader(F f) : F(std::move(f)) {}

  using F::operator();
};

namespace overloader_details {
struct no_such_type;
}  // namespace overloader_details

template <>
struct overloader<> {
  void operator()(overloader_details::no_such_type const* const /*unused*/) {}
};

template <class... Fs>
constexpr overloader<Fs...> make_overloader(Fs... fs) {
  return overloader<Fs...>{std::move(fs)...};
}

struct my_type1 {
  int func(int a) { return 2 * a; }
};

struct my_type2 {};

template <class...>
using void_t = void;

template <class T, class = void_t<>>
struct has_func : std::false_type {};

template <class T>
struct has_func<T,
                void_t<decltype(std::declval<T>().func(std::declval<int>()))>>
    : std::true_type {};

template <class T>
constexpr bool has_func_v = has_func<T>::value;

template <class T>
void check_for_func_member(T t) {
  constexpr auto my_lambdas = make_overloader(
      [](auto s) -> std::enable_if_t<has_func_v<decltype(s)>> {
        std::cout << "Has func(int) member using SFINAE\n";
      },
      [](auto s) -> std::enable_if_t<not has_func_v<decltype(s)>> {
        std::cout << "Has no func(int) member using SFINAE\n";
      });
  my_lambdas(t);
}

template <class T>
void check_for_func_member_overload(T t) {
  constexpr auto my_lambdas = make_overloader(
      [](auto s, std::true_type /*meta*/) {
        std::cout << "Has func(int) member using pseudo-SFINAE\n";
      },
      [](auto s, std::false_type /*meta*/) {
        std::cout << "Has no func(int) member using pseudo-SFINAE\n";
      });
  my_lambdas(t, typename has_func<T>::type{});
}

template <class Trait, class... Type>
constexpr bool local_trait_v =
    decltype(std::declval<Trait>()(std::declval<Type>()..., 0))::value;

template <class T>
void local_type_trait_example1(T /*t*/) {
  constexpr auto has_func_impl = make_overloader(
      [](auto t, int) -> decltype(
          (void)std::declval<decltype(t)>().func(std::declval<int>()),
          std::true_type{}) { return std::true_type{}; },
      [](auto...) { return std::false_type{}; });
  using has_func_member = decltype(has_func_impl);

  std::cout << "Has func(int) member function: " << std::boolalpha
            << local_trait_v<has_func_member, T> << "\n";
}

void local_type_trait_example2() {
  constexpr auto is_std_map = make_overloader(
      [](auto t, int) -> std::enable_if_t<
          std::is_same<decltype(t),
                       std::map<typename decltype(t)::key_type,
                                typename decltype(t)::mapped_type,
                                typename decltype(t)::key_compare,
                                typename decltype(t)::allocator_type>>::value,
          std::true_type> { return std::true_type{}; },
      [](auto...) { return std::false_type{}; });

  std::map<int, double> b;
  std::unordered_map<int, double> c;
  std::vector<int> d;
  std::cout << "Is a map: " << decltype(is_std_map(b, 0))::value << "\n";
  std::cout << "Is a map: " << decltype(is_std_map(c, 0))::value << "\n";
  std::cout << "Is a map: " << decltype(is_std_map(d, 0))::value << "\n";
}

int main() {
  std::cout << "\n";
  const auto lambdas = make_overloader(
      [](int a) { std::cout << "int: " << a << '\n'; },
      [](std::string a) { std::cout << "string: " << a << '\n'; });
  lambdas(1);
  lambdas("this is a string");

  std::cout << "\n";
  check_for_func_member(my_type1{});
  check_for_func_member(my_type2{});

  std::cout << "\n";
  check_for_func_member_overload(my_type1{});
  check_for_func_member_overload(my_type2{});

  std::cout << "\n";
  local_type_trait_example1(my_type1{});
  local_type_trait_example1(my_type2{});

  std::cout << "\n";
  local_type_trait_example2();
}
