#pragma once
#include "variant_utils.h"

namespace variant_utils {
template <typename... Rest>
union variant_union {};

template <typename First, typename... Rest>
union variant_union<First, Rest...> {

  constexpr variant_union() : rest() {}

  template <size_t Index, typename... Args>
  constexpr variant_union(in_place_index_t<Index>, Args&&... args)
      : rest(in_place_index<Index - 1>, std::forward<Args>(args)...) {}

  template <typename... Args>
  constexpr variant_union(in_place_index_t<0>, Args&&... args) : first(std::forward<Args>(args)...) {}

  template <size_t Index>
  void construct(variant_union const& other) {
    if constexpr (Index == 0) {
      std::construct_at(std::addressof(first), other.first);
    } else {
      rest.template construct<Index - 1>(other.rest);
    }
  }

  template <size_t Index>
  void construct(variant_union&& other) {
    if constexpr (Index == 0) {
      std::construct_at(std::addressof(first), std::move(other.first));
    } else {
      rest.template construct<Index - 1>(std::move(other.rest));
    }
  }

  template <size_t Index>
  void reset() {
    if constexpr (Index == 0) {
      first.~First();
    } else {
      rest.template reset<Index - 1>();
    }
  }

  template <size_t Index, typename... Args>
  decltype(auto) emplace(in_place_index_t<Index>, Args&&... args) {
    if constexpr (Index == 0) {
      std::construct_at(std::addressof(first), std::forward<Args>(args)...);
      return this->get(in_place_index<0>);
    }
    else {
      return rest.template emplace<Index - 1>(in_place_index<Index - 1>, std::forward<Args>(args)...);
    }
  }

  template <size_t Index>
  constexpr auto& get(in_place_index_t<Index>) {
    if constexpr (Index == 0) {
      return first;
    }
    else {
      return rest.get(in_place_index<Index - 1>);
    }
  }

  template <size_t Index>
  constexpr auto const& get(in_place_index_t<Index>) const {
    if constexpr (Index == 0) {
      return first;
    }
    else {
      return rest.get(in_place_index<Index - 1>);
    }
  }

  constexpr ~variant_union()
      requires(!variant_utils::trivial_dtor<First, Rest...>){};
  constexpr ~variant_union()
      requires(variant_utils::trivial_dtor<First, Rest...>) = default;

private:
  First first;
  variant_union<Rest...> rest;
};
} // namespace variant_utils