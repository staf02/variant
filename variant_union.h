#pragma once
#include "uninit_value.h"
#include "variant_utils.h"

namespace variant_utils {
template <typename... Rest>
union variant_union {};

template <typename First, typename... Rest>
union variant_union<First, Rest...> {

  constexpr variant_union() : rest() {}

  constexpr variant_union(variant_union const& other) = default;
  constexpr variant_union(variant_union&& other) = default;
  constexpr variant_union& operator=(variant_union const& other) = default;
  constexpr variant_union& operator=(variant_union&& other) = default;

  template <size_t Index, typename... Args>
  constexpr variant_union(in_place_index_t<Index>, Args&&... args)
      : rest(in_place_index<Index - 1>, std::forward<Args>(args)...) {}

  template <typename... Args>
  constexpr variant_union(in_place_index_t<0>, Args&&... args) : first(std::forward<Args>(args)...) {}

  template <size_t Index>
  void construct(variant_union const& other) {
    if constexpr (Index == 0) {
      first.construct(other.first);
    } else {
      rest.template construct<Index - 1>(other.rest);
    }
  }

  template <size_t Index>
  void construct(variant_union&& other) {
    if constexpr (Index == 0) {
      first.construct(std::move(other.first));
    } else {
      rest.template construct<Index - 1>(std::move(other.rest));
    }
  }

  template <size_t Index>
  void reset() {
    if constexpr (Index == 0) {
      first.reset();
    } else {
      rest.template reset<Index - 1>();
    }
  }

  template <size_t Index, typename... Args>
  decltype(auto) emplace(in_place_index_t<Index>, Args&&... args) {
    return rest.template emplace<Index - 1>(in_place_index<Index - 1>, std::forward<Args>(args)...);
  }

  template <size_t Index, typename... Args>
  decltype(auto) emplace(in_place_index_t<0>, Args&&... args) {
    std::construct_at(&first, std::forward<Args>(args)...);
    return first.get();
  }

  template <size_t Index>
  constexpr auto& get(in_place_index_t<Index>) {
    return rest.get(in_place_index<Index - 1>);
  }

  constexpr auto& get(in_place_index_t<0>) {
    return first.get();
  }

  template <size_t Index>
  constexpr auto const& get(in_place_index_t<Index>) const {
    return rest.get(in_place_index<Index - 1>);
  }

  constexpr auto const& get(in_place_index_t<0>) const {
    return first.get();
  }

  constexpr ~variant_union() = default;

private:
  uninit_value<First> first;
  variant_union<Rest...> rest;
};
} // namespace variant_utils