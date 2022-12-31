#pragma once

#include "variant.h"
#include <exception>
#include <functional>

template <typename... Args>
struct variant;

namespace variant_utils {
template <typename... Other>
union variant_union;
} // namespace variant_utils

// VARIANT SIZE
template <typename _Variant>
struct variant_size;

template <typename _Variant>
struct variant_size<const _Variant> : variant_size<_Variant> {};

template <typename _Variant>
struct variant_size<volatile _Variant> : variant_size<_Variant> {};

template <typename _Variant>
struct variant_size<const volatile _Variant> : variant_size<_Variant> {};

template <typename... _Types>
struct variant_size<variant<_Types...>> : std::integral_constant<size_t, sizeof...(_Types)> {};

template <typename _Variant>
inline constexpr size_t variant_size_v = variant_size<_Variant>::value;

// bad_variant_access

class bad_variant_access : public std::exception {
public:
  bad_variant_access() noexcept {}

  const char* what() const noexcept override {
    return reason;
  }

private:
  bad_variant_access(const char* __reason) noexcept : reason(__reason) {}

  const char* reason = "bad variant access";

  friend void __throw_bad_variant_access(const char* __what);
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

// VISIT

template <size_t I>
struct index_wrapper : std::integral_constant<size_t, I> {};

template <bool IsValidIndex, bool IndexChooser, typename ReturnType>
struct visit_chooser {};

template <bool IndexChooser, typename ReturnType>
struct visit_chooser<false, IndexChooser, ReturnType> {
  template <size_t Index, size_t... Indexes, typename Visitor, typename Variant, typename... Variants>
  static ReturnType run_func(Visitor&& vis, Variant&& var, Variants&&... vars) {
    throw bad_variant_access();
  }

  template <size_t CurrentIndex, size_t... Indexes, typename Visitor, typename Variant, typename... Variants>
  static ReturnType choose(Visitor&& vis, Variant&& var, Variants&&... vars) {
    throw bad_variant_access();
  }
};

template <bool IndexChooser, typename ReturnType>
struct visit_chooser<true, IndexChooser, ReturnType> {
  template <size_t Index, size_t... Indexes, typename Visitor, typename Variant, typename... Variants>
  static constexpr ReturnType run_func(Visitor&& vis, Variant&& var, Variants&&... vars) {
    if constexpr (sizeof...(Indexes) == sizeof...(vars)) {
      if constexpr (!IndexChooser) {
        return std::forward<Visitor>(vis)(get<Indexes>(std::forward<Variants>(vars))...,
                                          get<Index>(std::forward<Variant>(var)));
      } else {
        return std::forward<Visitor>(vis)(index_wrapper<Indexes>{}..., index_wrapper<Index>{});
      }
    } else {
      return choose<0, Indexes..., Index>(std::forward<Visitor>(vis), std::forward<Variants>(vars)...,
                                          std::forward<Variant>(var));
    }
  }

  template <size_t CurrentIndex, size_t... Indexes, typename Visitor, typename Variant, typename... Variants>
  static constexpr ReturnType choose(Visitor&& vis, Variant&& var, Variants&&... vars) {
    constexpr size_t size = variant_size_v<std::remove_cvref_t<Variant>>;
    if (var.index() == CurrentIndex) {
      return visit_chooser<CurrentIndex + 0 < size, IndexChooser, ReturnType>::template run_func<CurrentIndex + 0,
                                                                                                 Indexes...>(
          std::forward<Visitor>(vis), std::forward<Variant>(var), std::forward<Variants>(vars)...);
    }
    return visit_chooser<CurrentIndex + 1 < size, IndexChooser, ReturnType>::template choose<CurrentIndex + 1,
                                                                                             Indexes...>(
        std::forward<Visitor>(vis), std::forward<Variant>(var), std::forward<Variants>(vars)...);
  }
};
} // namespace variant_utils

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }

  using R = decltype(std::invoke(std::forward<Visitor>(vis), get<0>(std::forward<Variants>(vars))...));
  return variant_utils::visit_chooser<true, false, R>::template choose<0>(std::forward<Visitor>(vis),
                                                                          std::forward<Variants>(vars)...);
}

template <typename R, typename Visitor, typename... Variants>
constexpr R visit(Visitor&& vis, Variants&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }

  return variant_utils::visit_chooser<true, false, R>::template choose<0>(std::forward<Visitor>(vis),
                                                                          std::forward<Variants>(vars)...);
}

namespace variant_utils {
template <typename F, typename... V>
constexpr decltype(auto) visit_index(F&& f, V&&... v) {
  using R = decltype(std::invoke(std::forward<F>(f), get<0>(std::forward<V>(v))...));
  return visit_index<R>(std::forward<F>(f), std::forward<V>(v)...);
}

template <typename R, typename Visitor, typename... Variants>
constexpr R visit_index(Visitor&& vis, Variants&&... vars) {
  return visit_chooser<true, true, R>::template choose<0>(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <size_t ind, bool less, typename... Ts>
struct types_at_impl {};

template <size_t ind, typename... Ts>
struct types_at_impl<ind, true, Ts...> {
  using type = std::tuple_element_t<ind, std::tuple<Ts...>>;
};

template <size_t ind, typename... Ts>
using types_at_t = typename types_at_impl < ind,
      ind<sizeof...(Ts), Ts...>::type;

template <bool is_same, typename Target, typename... Ts>
struct type_index_impl {
  static constexpr size_t ind = 0;
};

template <typename Target, typename T, typename... Ts>
struct type_index_impl<false, Target, T, Ts...> {
  static constexpr size_t ind = type_index_impl<std::is_same_v<T, Target>, Target, Ts...>::ind + 1;
};

template <typename Target, typename T, typename... Ts>
inline constexpr size_t type_index_v = type_index_impl<std::is_same_v<T, Target>, Target, Ts...>::ind;

template <typename T, typename Base>
struct variant_type_index;

template <typename T, template <typename...> typename base, typename... Ts>
struct variant_type_index<T, base<Ts...>> {
  static constexpr size_t value = type_index_v<T, Ts...>;
};

template <typename Target, typename Variant>
inline constexpr size_t index_chooser_v = variant_type_index<Target, Variant>::value;

template <size_t I, typename T>
struct alternative;

template <size_t I, template <typename...> typename base, typename... Ts>
struct alternative<I, base<Ts...>> {
  using type = types_at_t<I, Ts...>;
};

template <size_t I, template <typename...> typename base, typename... Ts>
struct alternative<I, const base<Ts...>> {
  using type = const types_at_t<I, Ts...>;
};

template <size_t I, template <typename...> typename base, typename... Ts>
struct alternative<I, base<Ts...>&&> {
  using type = types_at_t<I, Ts...>&&;
};

template <size_t I, template <typename...> typename base, typename... Ts>
struct alternative<I, base<Ts...> const&&> {
  using type = const types_at_t<I, Ts...>&&;
};

template <size_t I, template <typename...> typename base, typename... Ts>
struct alternative<I, base<Ts...>&> {
  using type = types_at_t<I, Ts...>&;
};

template <size_t I, template <typename...> typename base, typename... Ts>
struct alternative<I, base<Ts...> const&> {
  using type = types_at_t<I, Ts...> const&;
};

template <size_t I, template <bool, typename...> typename base, bool flag, typename... Ts>
struct alternative<I, base<flag, Ts...>> {
  using type = types_at_t<I, Ts...>;
};

template <size_t I, template <bool, typename...> typename base, bool flag, typename... Ts>
struct alternative<I, base<flag, Ts...> const> {
  using type = const types_at_t<I, Ts...>;
};

template <size_t I, template <bool, typename...> typename base, bool flag, typename... Ts>
struct alternative<I, base<flag, Ts...>&&> {
  using type = types_at_t<I, Ts...>&&;
};

template <size_t I, template <bool, typename...> typename base, bool flag, typename... Ts>
struct alternative<I, base<flag, Ts...> const&&> {
  using type = types_at_t<I, Ts...> const&&;
};

template <size_t I, template <bool, typename...> typename base, bool flag, typename... Ts>
struct alternative<I, base<flag, Ts...>&> {
  using type = types_at_t<I, Ts...>&;
};

template <size_t I, template <bool, typename...> typename base, bool flag, typename... Ts>
struct alternative<I, base<flag, Ts...> const&> {
  using type = types_at_t<I, Ts...> const&;
};

template <size_t I, typename T>
using alternative_t = typename alternative<I, T>::type;

template <class T>
struct alt_indexes;

template <template <typename...> typename base, typename... Ts>
struct alt_indexes<base<Ts...>> {
  using type = std::index_sequence_for<Ts...>;
};

template <template <bool, typename...> typename base, bool flag, typename... Ts>
struct alt_indexes<base<flag, Ts...>> {
  using type = std::index_sequence_for<Ts...>;
};

template <class T>
using alt_indexes_t = typename alt_indexes<std::decay_t<T>>::type;

template <size_t index, bool empty, typename... Ts>
struct alt_indexes_by_ind {
  using type = std::index_sequence<>;
};

template <size_t index, typename... Ts>
struct alt_indexes_by_ind<index, true, Ts...> {
  using type = alt_indexes_t<std::decay_t<types_at_t<index, Ts...>>>;
};

template <size_t index, typename... Ts>
using alt_indexes_by_ind_t = typename alt_indexes_by_ind < index,
      index<sizeof...(Ts), Ts...>::type;

template <typename T>
struct find_overload_array {
  T x[1];
};

template <typename U, typename T, size_t ind, typename = void>
struct fun {
  T operator()();
};

template <typename U, typename T, size_t ind>
struct fun<U, T, ind,
           std::enable_if_t<(!std::is_same_v<std::decay_t<T>, bool> || std::is_same_v<std::decay_t<U>, bool>)&&std::
                                is_same_v<void, std::void_t<decltype(find_overload_array<T>{{std::declval<U>()}})>>>> {
  T operator()(T);
};

template <typename U, typename IndexSequence, typename... Ts>
struct find_overload;

template <typename U, size_t... Is, typename... Ts>
struct find_overload<U, std::index_sequence<Is...>, Ts...> : fun<U, Ts, Is>... {
  using fun<U, Ts, Is>::operator()...;
};

template <typename U, typename... Ts>
using find_overload_t = typename std::invoke_result_t<find_overload<U, std::index_sequence_for<Ts...>, Ts...>, U>;

template <typename T, typename... Ts>
inline constexpr bool exactly_once_v = (std::is_same_v<T, Ts> + ...) == 1;

template <typename T, template <typename...> typename Template>
struct is_type_spec : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_type_spec<Template<Args...>, Template> : std::true_type {};

template <typename T, template <typename...> typename Template>
inline constexpr bool is_type_spec_v = is_type_spec<T, Template>::value;

template <typename T, template <size_t...> typename Template>
struct is_size_spec : std::false_type {};

template <template <size_t...> typename Template, size_t... Args>
struct is_size_spec<Template<Args...>, Template> : std::true_type {};

template <typename T, template <size_t...> typename Template>
inline constexpr bool is_size_spec_v = is_size_spec<T, Template>::value;
} // namespace variant_utils

template <size_t Index, class... Args>
constexpr variant_alternative_t<Index, variant<Args...>>& get(variant<Args...>& v) {
  if (Index != v.index()) {
    throw bad_variant_access();
  }
  return v.get(in_place_index<Index>);
}

template <std::size_t Index, class... Args>
constexpr variant_alternative_t<Index, variant<Args...>>&& get(variant<Args...>&& v) {
  return std::move(get<Index>(v));
}

template <std::size_t Index, class... Args>
constexpr const variant_alternative_t<Index, variant<Args...>>& get(const variant<Args...>& v) {
  if (Index != v.index()) {
    throw bad_variant_access();
  }
  return v.get(in_place_index<Index>);
}

template <std::size_t Index, class... Args>
constexpr const variant_alternative_t<Index, variant<Args...>>&& get(const variant<Args...>&& v) {
  return std::move(get<Index>(v));
}

template <class T, class... Args>
constexpr T& get(variant<Args...>& v) {
  return get<variant_utils::index_chooser_v<T, variant<Args...>>>(v);
}

template <class T, class... Args>
constexpr T&& get(variant<Args...>&& v) {
  return std::move(get<variant_utils::index_chooser_v<T, variant<Args...>>>(v));
}

template <class T, class... Args>
constexpr const T& get(const variant<Args...>& v) {
  return get<variant_utils::index_chooser_v<T, variant<Args...>>>(v);
}

template <class T, class... Args>
constexpr const T&& get(const variant<Args...>&& v) {
  return std::move(get<variant_utils::index_chooser_v<T, variant<Args...>>>(v));
}