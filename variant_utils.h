#pragma once

#include "variant.h"
#include <array>
#include <exception>
#include <functional>

template <typename... Args>
class variant;

namespace variant_utils {
template <typename... Other>
union variant_union;
} // namespace variant_utils

// VARIANT SIZE
template <typename Variant>
struct variant_size;

template <typename Variant>
struct variant_size<const Variant> : variant_size<Variant> {};

template <typename Variant>
struct variant_size<volatile Variant> : variant_size<Variant> {};

template <typename Variant>
struct variant_size<const volatile Variant> : variant_size<Variant> {};

template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};

template <typename Variant>
inline constexpr size_t variant_size_v = variant_size<Variant>::value;

// bad_variant_access

class bad_variant_access : public std::exception {
public:
  bad_variant_access() noexcept {}

  const char* what() const noexcept override {
    return reason;
  }

private:
  const char* reason = "bad variant access";
};

// indexes

inline constexpr size_t variant_npos = -1;

template <size_t T>
class in_place_index_t {
public:
  explicit constexpr in_place_index_t() = default;
};

template <size_t Index>
inline constexpr in_place_index_t<Index> in_place_index;

template <class T>
struct in_place_type_t {
  explicit in_place_type_t() = default;
};

template <class T>
inline constexpr in_place_type_t<T> in_place_type;

struct monostate {};

// VARIANT ALTERNATIVE

template <size_t Index, typename Variant>
struct variant_alternative;

template <size_t Index, typename First, typename... Rest>
struct variant_alternative<Index, variant<First, Rest...>> : variant_alternative<Index - 1, variant<Rest...>> {};

template <typename First, typename... Rest>
struct variant_alternative<0, variant<First, Rest...>> {
  using type = First;
};

template <size_t Index, typename Variant>
using variant_alternative_t = typename variant_alternative<Index, Variant>::type;

template <size_t Index, typename Variant>
struct variant_alternative<Index, const Variant> {
  using type = std::add_const_t<variant_alternative_t<Index, Variant>>;
};

template <size_t Index, typename Variant>
struct variant_alternative<Index, volatile Variant> {
  using type = std::add_volatile_t<variant_alternative_t<Index, Variant>>;
};

template <size_t Index, typename Variant>
struct variant_alternative<Index, const volatile Variant> {
  using type = std::add_cv_t<variant_alternative_t<Index, Variant>>;
};

namespace variant_utils {

template <typename T>
struct arr {
  T x[1];
};

template <typename V, typename T, size_t Index, typename = void>
struct fun {
  T operator()();
};

template <typename V, typename T, size_t Index>
struct fun<V, T, Index,
           std::enable_if_t<(!std::is_same_v<std::decay_t<T>, bool> || std::is_same_v<std::decay_t<V>, bool>)&&std::
                                is_same_v<void, std::void_t<decltype(arr<T>{{std::declval<V>()}})>>>> {
  T operator()(T);
};

template <typename T, typename IndexSequence, typename... Types>
struct find_overload;

template <typename T, size_t... Indexes, typename... Types>
struct find_overload<T, std::index_sequence<Indexes...>, Types...> : fun<T, Types, Indexes>... {
  using fun<T, Types, Indexes>::operator()...;
};

template <typename T, typename... Types>
using find_overload_t = typename std::invoke_result_t<find_overload<T, std::index_sequence_for<Types...>, Types...>, T>;

template <size_t Index, bool less, typename... Types>
struct type_at {};

template <size_t Index, typename... Types>
struct type_at<Index, true, Types...> {
  using type = std::tuple_element_t<Index, std::tuple<Types...>>;
};

template <size_t Index, typename... Types>
using types_at_t = typename type_at < Index,
      Index<sizeof...(Types), Types...>::type;

template <bool is_same, typename CurrentType, typename... Types>
struct index_chooser {
  static constexpr size_t Index = 0;
};

template <typename CurrentType, typename T, typename... Types>
struct index_chooser<false, CurrentType, T, Types...> {
  static constexpr size_t Index = index_chooser<std::is_same_v<T, CurrentType>, CurrentType, Types...>::Index + 1;
};

template <typename CurrentType, typename T, typename... Types>
inline constexpr size_t index_chooser_v = index_chooser<std::is_same_v<T, CurrentType>, CurrentType, Types...>::Index;

} // namespace variant_utils

template <size_t Index, class... Types>
constexpr variant_alternative_t<Index, variant<Types...>>& get(variant<Types...>& v) {
  if (Index != v.index()) {
    throw bad_variant_access();
  }
  return v.get(in_place_index<Index>);
}

template <std::size_t Index, class... Types>
constexpr variant_alternative_t<Index, variant<Types...>>&& get(variant<Types...>&& v) {
  return std::move(get<Index>(v));
}

template <std::size_t Index, class... Types>
constexpr const variant_alternative_t<Index, variant<Types...>>& get(const variant<Types...>& v) {
  if (Index != v.index()) {
    throw bad_variant_access();
  }
  return v.get(in_place_index<Index>);
}

template <std::size_t Index, class... Types>
constexpr const variant_alternative_t<Index, variant<Types...>>&& get(const variant<Types...>&& v) {
  return std::move(get<Index>(v));
}

template <class T, class... Types>
constexpr T& get(variant<Types...>& v) {
  return get<variant_utils::index_chooser_v<T, Types...>>(v);
}

template <class T, class... Types>
constexpr T&& get(variant<Types...>&& v) {
  return std::move(get<variant_utils::index_chooser_v<T, Types...>>(v));
}

template <class T, class... Types>
constexpr const T& get(const variant<Types...>& v) {
  return get<variant_utils::index_chooser_v<T, Types...>>(v);
}

template <class T, class... Types>
constexpr const T&& get(const variant<Types...>&& v) {
  return std::move(get<variant_utils::index_chooser_v<T, Types...>>(v));
}

namespace variant_utils {

// VISIT

template <size_t I>
struct index_wrapper : std::integral_constant<size_t, I> {};

template <size_t Index, typename... Variants>
struct get_next_indexes;

template <size_t Index, typename Variant, typename... Variants>
struct get_next_indexes<Index, Variant, Variants...> {
  using type = typename get_next_indexes<Index - 1, Variants...>::type;
};

template <typename Variant, typename... Variants>
struct get_next_indexes<0, Variant, Variants...> {
  using type = std::make_index_sequence<variant_size_v<std::decay_t<Variant>>>;
};

template <>
struct get_next_indexes<0> {
  using type = std::index_sequence<>;
};

template <size_t Index, typename... Variants>
using get_next_indexes_t = typename get_next_indexes<Index, Variants...>::type;

template <bool indexed, typename R, typename Visitor, typename Indexes, typename... Variants>
struct runner;

template <typename R, typename Visitor, size_t... Indexes, typename... Variants>
struct runner<false, R, Visitor, std::index_sequence<Indexes...>, Variants...> {
  static constexpr R run_func(Visitor vis, Variants... vars) {
    return std::forward<Visitor>(vis)(::get<Indexes>(std::forward<Variants>(vars))...);
  }
};

template <typename R, typename Visitor, size_t... Indexes, typename... Variants>
struct runner<true, R, Visitor, std::index_sequence<Indexes...>, Variants...> {
  static constexpr R run_func(Visitor vis, Variants... vars) {
    return std::forward<Visitor>(vis)(index_wrapper<Indexes>{}...);
  }
};

template <typename... Args>
constexpr std::array<std::common_type_t<Args...>, sizeof...(Args)> make_array(Args&&... args) {
  return {std::forward<Args>(args)...};
}

template <bool indexed, typename R, typename Visitor, size_t Index, typename PrefixIndexesSeq, typename VariantSeq,
          typename... Variants>
struct visit_table;

template <typename R, typename Visitor, size_t Index, size_t... Indexes, typename... Variants>
struct visit_table<false, R, Visitor, Index, std::index_sequence<Indexes...>, std::index_sequence<>, Variants...> {

  static constexpr auto build_next() {
    return &runner<false, R, Visitor, std::index_sequence<Indexes...>, Variants...>::run_func;
  }
};

template <typename R, typename Visitor, size_t Index, size_t... Indexes, typename... Variants>
struct visit_table<true, R, Visitor, Index, std::index_sequence<Indexes...>, std::index_sequence<>, Variants...> {
  static constexpr auto build_next() {
    return &runner<true, R, Visitor, std::index_sequence<Indexes...>, Variants...>::run_func;
  }
};

template <bool indexed, typename R, typename Visitor, size_t Index, size_t... PrefixIndexes, size_t... NextIndexes,
          typename... Variants>
struct visit_table<indexed, R, Visitor, Index, std::index_sequence<PrefixIndexes...>,
                   std::index_sequence<NextIndexes...>, Variants...> {
  static constexpr auto build_next() {
    return make_array(
        visit_table<indexed, R, Visitor, Index + 1, std::index_sequence<PrefixIndexes..., NextIndexes>,
                    variant_utils::get_next_indexes_t<Index + 1, Variants...>, Variants...>::build_next()...);
  }
};

template <bool indexed, typename R, typename Visitor, typename... Variants>
using visit_table_t = visit_table<indexed, R, Visitor, 0, std::index_sequence<>,
                                  variant_utils::get_next_indexes_t<0, Variants...>, Variants...>;

template <typename Overload>
constexpr auto const& get_overload(Overload const& overload) {
  return overload;
}

template <typename Overload, typename... Is>
constexpr auto const& get_overload(Overload const& overload, size_t index, Is... indexes) {
  return get_overload(overload[index], indexes...);
}

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit_index(Visitor&& vis, Variants&&... vars) {
  using R = decltype(std::invoke(std::forward<Visitor>(vis), get<0>(std::forward<Variants>(vars))...));
  return variant_utils::get_overload(visit_table_t<true, R, Visitor&&, Variants&&...>::build_next(),
                                     vars.index()...)(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <typename R, typename Visitor, typename... Variants>
constexpr R visit_index(Visitor&& vis, Variants&&... vars) {
  return variant_utils::get_overload(visit_table_t<true, R, Visitor&&, Variants&&...>::build_next(),
                                     vars.index()...)(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

} // namespace variant_utils

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }

  using R = decltype(std::invoke(std::forward<Visitor>(vis), get<0>(std::forward<Variants>(vars))...));
  return variant_utils::get_overload(variant_utils::visit_table_t<false, R, Visitor&&, Variants&&...>::build_next(),
                                     vars.index()...)(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <typename R, typename Visitor, typename... Variants>
constexpr R visit(Visitor&& vis, Variants&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }

  return variant_utils::get_overload(variant_utils::visit_table_t<false, R, Visitor&&, Variants&&...>::build_next(),
                                     vars.index()...)(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}