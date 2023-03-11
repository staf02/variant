#pragma once
#include "variant.h"
#include "variant_utils.h"
#include <concepts>
#include <type_traits>

namespace variant_utils {

template <size_t Index, typename... Types>
struct nth_type;

template <size_t Index, typename First, typename... Rest>
struct nth_type<Index, First, Rest...> : nth_type<Index - 1, Rest...> {};

template <typename First, typename... Rest>
struct nth_type<0, First, Rest...> {
  using type = First;
};

template <typename... Types>
concept default_ctor = std::is_default_constructible_v<typename nth_type<0, Types...>::type>;

template <typename... Types>
concept copy_ctor = (std::is_copy_constructible_v<Types> && ...);

template <typename... Types>
concept move_ctor = (std::is_move_constructible_v<Types> && ...);

template <typename... Types>
concept copy_assign = copy_ctor<Types...> && (std::is_copy_assignable_v<Types> && ...);

template <typename... Types>
concept move_assign = move_ctor<Types...> && (std::is_move_assignable_v<Types> && ...);

template <typename... Types>
concept trivial_dtor = (std::is_trivially_destructible_v<Types> && ...);

template <typename... Types>
concept trivial_copy_ctor = (std::is_trivially_copy_constructible_v<Types> && ...);

template <typename... Types>
concept trivial_move_ctor = (std::is_trivially_move_constructible_v<Types> && ...);

template <typename... Types>
concept trivial_copy_assign =
    trivial_dtor<Types...> && trivial_copy_ctor<Types...> && (std::is_trivially_copy_assignable_v<Types> && ...);

template <typename... Types>
concept trivial_move_assign =
    trivial_dtor<Types...> && trivial_move_ctor<Types...> && (std::is_trivially_move_assignable_v<Types> && ...);

template <typename... Types>
concept nothrow_default_ctor = std::is_nothrow_default_constructible_v<typename nth_type<0, Types...>::type>;

template <typename... Types>
concept nothrow_copy_ctor = false;

template <typename... Types>
concept nothrow_move_ctor = (std::is_nothrow_move_constructible_v<Types> && ...);

template <typename... Types>
concept nothrow_copy_assign = false;

template <typename... Types>
concept nothrow_move_assign = nothrow_move_ctor<Types...> && (std::is_nothrow_move_assignable_v<Types> && ...);

template <typename T, typename... Types>
concept nothrow_convert_ctor =
    std::is_nothrow_constructible_v<variant_alternative_t<index_chooser_v<T, Types...>, variant<Types...>>, T>;

template <typename T, typename... Types>
concept nothrow_convert_assign =
    nothrow_convert_ctor<T, Types...> &&
    std::is_nothrow_assignable_v<variant_alternative_t<index_chooser_v<T, Types...>, variant<Types...>>, T>;

template <typename T, typename... Types>
inline constexpr bool exactly_once_v = (std::is_same_v<T, Types> + ...) == 1;

} // namespace variant_utils