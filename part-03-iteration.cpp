
#include <iostream>
#include <type_traits>
#include <tuple>

template <typename... Elements, size_t... Is>
void print_helper(std::ostream& os, const std::tuple<Elements...>& t, std::index_sequence<Is...> /*meta*/) {
  static_cast<void>(std::initializer_list<char>{
      (static_cast<void>(os << std::get<Is>(t) << ", "), '0')...});
}

template <typename... Elements>
std::ostream& operator<<(std::ostream& os, const std::tuple<Elements...>& t) {
  os << "(";
  print_helper(os, t, std::make_index_sequence<sizeof...(Elements) - 1>{});
  os << std::get<sizeof...(Elements) - 1>(t);
  os << ")";
  return os;
}

// template <typename...>
// struct typelist {};

// using my_fund_list =
//     typelist<double, int, char, double, bool, bool, short, char>;

// namespace detail {
// template <typename List, template <typename...> class F>
// struct transform {};

// template <typename First, typename... Rest, template <typename...> class F>
// struct transform<typelist<First, Rest...>, F> : {};

// }  // namespace

// template <typename List, template <typename...> class F>
// using transform = detail::transform<List, F>;

// template <template <typename...> class, typename...>
// struct bind {};

// template <typename T>
// struct xu {
//   using type = T;
// };

// template <template <typename...> class F, typename... Ts>
// struct unbind<bind<F, Ts...>> {
//   using type = F<typename unbox<Ts>::type...>;
// };

// template <bool>
// struct conditional;

// template <>
// struct conditional<true> {
//   template <typename T, typename F>
//   using type = typename unbind<T>::type;
// };

// template <>
// struct conditional<false> {
//   template <typename T, typename F>
//   using type = typename unbind<F>::type;
// };

// template <bool B, typename T, typename F>
// using conditional_t = typename conditional<B>::template type<T, F>;


// template <typename T>
// struct enable_if {
//   using dummy = T;
// };

// static_assert(
//     conditional_t<
//         false,
//         bind<std::is_same, double, double>,
//         bind<enable_if, std::integral_constant<bool, true>>>::type::value,
//     "wat");

template <size_t Index, typename... Elements, typename Func, typename... Args,
          std::enable_if_t<Index == 0>* = nullptr>
constexpr inline auto tuple_transform(const std::tuple<Elements...>& tuple,
                                      Func&& func, Args&&... args) -> void {
  func(std::get<Index>(tuple), std::integral_constant<size_t, Index>{},
       args...);
}

template <size_t Index, typename... Elements, typename Func, typename... Args,
          std::enable_if_t<Index != 0>* = nullptr>
constexpr inline auto tuple_transform(const std::tuple<Elements...>& tuple,
                                      Func&& func, Args&&... args) -> void {
  tuple_transform<Index - 1>(tuple, func, args...);
  func(std::get<Index>(tuple), std::integral_constant<size_t, Index>{},
       args...);
}

void tuple_fold_and_counted_fold_example() {
  const auto my_tupull = std::make_tuple(2, 7, -3.8, 20.9);
  double sum_value = 0.0;
  tuple_transform<3>(my_tupull, [](const auto& element, auto /*index*/,
                                   double& state) { state += element; },
                     sum_value);
  std::cout << "Expected: 26.1   Computed: " << sum_value << "\n";

  sum_value = 0.0;
  tuple_transform<3>(my_tupull,
                     [](const auto& element, auto index, double& state) {
                       if (index.value != 1) {
                         state += element;
                       }
                     },
                     sum_value);
  std::cout << "Expected: 19.1   Computed: " << sum_value << "\n";
}

void tuple_transform_example() {
  const auto my_tupull = std::make_tuple(2, 7, -3.8, 20.9);
  std::decay_t<decltype(my_tupull)> out_tupull;
  tuple_transform<3>(my_tupull,
                     [](const auto& element, auto index, auto& out_tuple) {
                       constexpr size_t index_v = decltype(index)::value;
                       std::get<index_v>(out_tuple) = -element;
                     },
                     out_tupull);
  std::cout << "Expected: (-2, -7, 3.8, -20.9)   Computed: " << out_tupull
            << "\n";
}

int main() {
  tuple_fold_and_counted_fold_example();
  tuple_transform_example();
}
