#pragma once
#include "variant_utils.h"
#include "variant.h"
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
struct variant_traits {
  static constexpr bool default_ctor = std::is_default_constructible_v<typename nth_type<0, Types...>::type>;
  static constexpr bool copy_ctor = (std::is_copy_constructible_v<Types> && ...);
  static constexpr bool move_ctor = (std::is_move_constructible_v<Types> && ...);
  static constexpr bool copy_assign = copy_ctor && (std::is_copy_assignable_v<Types> && ...);
  static constexpr bool move_assign = move_ctor && (std::is_move_assignable_v<Types> && ...);

  static constexpr bool trivial_dtor = (std::is_trivially_destructible_v<Types> && ...);
  static constexpr bool trivial_copy_ctor = (std::is_trivially_copy_constructible_v<Types> && ...);
  static constexpr bool trivial_move_ctor = (std::is_trivially_move_constructible_v<Types> && ...);
  static constexpr bool trivial_copy_assign =
      trivial_dtor && trivial_copy_ctor && (std::is_trivially_copy_assignable_v<Types> && ...);
  static constexpr bool trivial_move_assign =
      trivial_dtor && trivial_move_ctor && (std::is_trivially_move_assignable_v<Types> && ...);

  template <typename T>
  struct is_in_place : std::integral_constant<bool, false> {};

  template <typename T>
  struct is_in_place<in_place_type_t<T>> : std::integral_constant<bool, true> {};

  template <size_t I>
  struct is_in_place<in_place_index_t<I>> : std::integral_constant<bool, true> {};

  template <typename T>
  static constexpr bool is_in_place_v = !is_in_place<T>::value;

  template <typename T,
            std::enable_if_t<index_chooser_v<T, variant<Types...>><sizeof...(Types), int> = 0> static constexpr bool
                converting_constructible =
                    (sizeof...(Types) > 0) &&
                    (!std::is_same_v<variant<Types...>, std::decay_t<T>>)&&(is_in_place_v<T>)&&(
                        std::is_constructible_v<
                            variant_alternative_t<index_chooser_v<T, variant<Types...>>, variant<Types...>>, T>);

  template<size_t Index, typename = std::enable_if_t<(Index < sizeof...(Types))>>
  using to_type = variant_alternative_t<Index, variant<Types...>>;

  template <size_t I, std::enable_if_t<(I < sizeof...(Types)), int> = 0>
  struct in_place_index_constructible_base {
    template <typename ...F_Types>
    static constexpr bool in_place_index_constructible = std::is_constructible_v<variant_alternative_t<I, variant<Types...>>, F_Types...>;
  };



  // The following nothrow traits are for non-trivial SMFs. Trivial SMFs
  // are always nothrow.
  static constexpr bool nothrow_default_ctor =
      std::is_nothrow_default_constructible_v<typename nth_type<0, Types...>::type>;
  static constexpr bool nothrow_copy_ctor = false;
  static constexpr bool nothrow_move_ctor = (std::is_nothrow_move_constructible_v<Types> && ...);
  static constexpr bool nothrow_copy_assign = false;
  static constexpr bool nothrow_move_assign = nothrow_move_ctor && (std::is_nothrow_move_assignable_v<Types> && ...);

  template <typename T, std::enable_if_t<index_chooser_v<T, variant<Types...>> < sizeof...(Types), int> = 0>
                        static constexpr bool nothrow_converting_constructible = std::is_nothrow_constructible_v<variant_alternative_t<index_chooser_v<T, variant<Types...>>, variant<Types...>>, T>;
  template <typename T, std::enable_if_t<index_chooser_v<T, variant<Types...>> < sizeof...(Types), int> = 0>
                        static constexpr bool nothrow_converting_assignable = nothrow_converting_constructible<T> &&
                            std::is_nothrow_assignable_v<variant_alternative_t<index_chooser_v<T, variant<Types...>>, variant<Types...>>, T>;

};
} // namespace variant_utils