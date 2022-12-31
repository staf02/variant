#pragma once

#include "variant.h"
#include <exception>
#include <functional>

template <typename... Args>
class variant;

namespace variant_utils {
template <typename... Other>
union variant_union;
} // namespace variant_utils

// VARIANT SIZE
template<typename _Variant>
struct variant_size;

template<typename _Variant>
struct variant_size<const _Variant> : variant_size<_Variant> {};

template<typename _Variant>
struct variant_size<volatile _Variant> : variant_size<_Variant> {};

template<typename _Variant>
struct variant_size<const volatile _Variant> : variant_size<_Variant> {};

template<typename... _Types>
struct variant_size<variant<_Types...>>
    : std::integral_constant<size_t, sizeof...(_Types)> {};

template<typename _Variant>
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

template <size_t Index, typename _First, typename... _Rest>
struct variant_alternative<Index, variant<_First, _Rest...>> : variant_alternative<Index - 1, variant<_Rest...>> {};

template <typename _First, typename... _Rest>
struct variant_alternative<0, variant<_First, _Rest...>> {
  using type = _First;
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
    constexpr size_t size = variant_size_v<std::__remove_cvref_t<Variant>>;
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

/// Alias template index_sequence
template <size_t... _Idx>
using index_sequence = std::integer_sequence<size_t, _Idx...>;

/// Alias template make_index_sequence
template <size_t _Num>
using make_index_sequence = std::make_integer_sequence<size_t, _Num>;

// Helper used to check for valid conversions that don't involve narrowing.
template <typename _Ti>
struct _Arr {
  _Ti _M_x[1];
};

// "Build an imaginary function FUN(Ti) for each alternative type Ti"
template <size_t _Ind, typename _Tp, typename _Ti, typename = void>
struct _Build_FUN {
  // This function means 'using _Build_FUN<I, T, Ti>::_S_fun;' is valid,
  // but only static functions will be considered in the call below.
  void _S_fun();
};

// "... for which Ti x[] = {std::forward<T>(t)}; is well-formed."
template <size_t _Ind, typename _Tp, typename _Ti>
struct _Build_FUN<_Ind, _Tp, _Ti, std::void_t<decltype(_Arr<_Ti>{{std::declval<_Tp>()}})>> {
  // This is the FUN function for type _Ti, with index _Ind
  static std::integral_constant<size_t, _Ind> _S_fun(_Ti);
};

template <typename _Tp, typename Variant, typename = make_index_sequence<variant_size_v<Variant>>>
struct _Build_FUNs;

template <typename _Tp, typename... _Ti, size_t... _Ind>
struct _Build_FUNs<_Tp, variant<_Ti...>, index_sequence<_Ind...>> : _Build_FUN<_Ind, _Tp, _Ti>... {
  using _Build_FUN<_Ind, _Tp, _Ti>::_S_fun...;
};

// The index j of the overload FUN(Tj) selected by overload resolution
// for FUN(std::forward<_Tp>(t))
template <typename _Tp, typename Variant>
using _FUN_type =
    decltype(_Build_FUNs<_Tp, Variant>::_S_fun(std::declval<_Tp>())); // integral_constant<size_t, current_index>

// The index selected for FUN(std::forward<T>(t)), or variant_npos if none.
template <typename _Tp, typename Variant, typename = void>
struct __accepted_index : std::integral_constant<size_t, 0> {};

template <typename _Tp, typename Variant>
struct __accepted_index<_Tp, Variant, std::void_t<_FUN_type<_Tp, Variant>>> : _FUN_type<_Tp, Variant> {};

template <typename T, typename Variant>
static constexpr size_t index_chooser_v = __accepted_index<T, Variant>::value;
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
