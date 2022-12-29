#pragma once

#include "variant.h"
#include "variant_traits.h"
#include "variant_union.h"
#include <iostream>
#include <string>

namespace variant_utils {

template <bool is_trivially_destructible, typename... Types>
struct variant_storage_dtor_base {};

template <bool is_trivial, typename... Types>
constexpr variant<Types...>& variant_cast(variant_storage_dtor_base<is_trivial, Types...>& base) {
  return static_cast<variant<Types...>&>(base);
}

template <bool is_trivial, typename... Types>
constexpr variant<Types...>&& variant_cast(variant_storage_dtor_base<is_trivial, Types...>&& base) {
  return static_cast<variant<Types...>&&>(base);
}

template <bool is_trivial, typename... Types>
constexpr variant<Types...> const& variant_cast(variant_storage_dtor_base<is_trivial, Types...> const& base) {
  return static_cast<variant<Types...> const&>(base);
}

template <typename... Types>
struct variant_storage_dtor_base<false, Types...> {
public:
  constexpr variant_storage_dtor_base() = default;
  constexpr variant_storage_dtor_base(variant_storage_dtor_base const& other) = default;

  template <size_t Index, typename... ConstructorRest>
  explicit constexpr variant_storage_dtor_base(in_place_index_t<Index>, ConstructorRest&&... rest)
      : storage(in_place_index<Index>, std::forward<ConstructorRest>(rest)...), index_(Index) {}

  void reset() {
    if (index_ != variant_npos) {
      variant_utils::visit_index<void>(
          [this](auto this_index) {
            this->storage.template reset<this_index>();
          },
          variant_cast(*this));
      index_ = variant_npos;
    }
  }

  ~variant_storage_dtor_base() {
    this->reset();
  }

  variant_union<Types...> storage;
  size_t index_{0};
};

template <typename... Types>
struct variant_storage_dtor_base<true, Types...> {
public:
  constexpr variant_storage_dtor_base() = default;
  constexpr variant_storage_dtor_base(variant_storage_dtor_base const& other) = default;

  template <size_t Index, typename... ConstructorRest>
  explicit constexpr variant_storage_dtor_base(in_place_index_t<Index>, ConstructorRest&&... rest)
      : storage(in_place_index<Index>, std::forward<ConstructorRest>(rest)...), index_(Index) {}

  void reset() {
    if (index_ != variant_npos) {
      variant_utils::visit_index<void>(
          [this](auto this_index) {
            this->storage.template reset<this_index>();
          },
          variant_cast(*this));
      index_ = variant_npos;
    }
  }

  ~variant_storage_dtor_base() = default;

  variant_union<Types...> storage;
  size_t index_{0};
};

template <bool is_trivially_move_assignable, typename... Types>
struct variant_storage_move_assign_base : variant_storage_dtor_base<variant_traits<Types...>::trivial_dtor, Types...> {
};

template <typename... Types>
struct variant_storage_move_assign_base<false, Types...>
    : variant_storage_dtor_base<variant_traits<Types...>::trivial_dtor, Types...> {
public:
  using base = variant_storage_dtor_base<variant_traits<Types...>::trivial_dtor, Types...>;
  using base::base;

  constexpr variant_storage_move_assign_base(variant_storage_move_assign_base const& other) = default;
  constexpr variant_storage_move_assign_base(variant_storage_move_assign_base&& other) = default;
  constexpr variant_storage_move_assign_base& operator=(variant_storage_move_assign_base const& other) = default;

  variant_storage_move_assign_base&
  operator=(variant_storage_move_assign_base&& other) noexcept(variant_traits<Types...>::nothrow_move_assign) {
    variant<Types...>& this_variant = variant_cast(*this);
    variant<Types...>&& other_variant = variant_cast(std::forward<decltype(other)>(other));
    if (other_variant.valueless_by_exception()) {
      if (this_variant.valueless_by_exception()) {
        return *this;
      }
      this->reset();
      return *this;
    }
    variant_utils::visit_index<void>(
        [&this_variant, &other_variant](auto this_index, auto other_index) {
          if constexpr (this_index == other_index) {
            get<this_index>(this_variant) = std::move(get<other_index>(other_variant));
          } else {
            this_variant.template emplace<other_index>(std::move(get<other_index>(other_variant)));
          }
        },
        this_variant, std::move(other_variant));
    return *this;
  }
};

template <typename... Types>
struct variant_storage_move_assign_base<true, Types...>
    : variant_storage_dtor_base<variant_traits<Types...>::trivial_dtor, Types...> {
public:
  using base = variant_storage_dtor_base<variant_traits<Types...>::trivial_dtor, Types...>;
  using base::base;

  constexpr variant_storage_move_assign_base(variant_storage_move_assign_base const& other) = default;
  constexpr variant_storage_move_assign_base(variant_storage_move_assign_base&& other) = default;
  constexpr variant_storage_move_assign_base& operator=(variant_storage_move_assign_base const& other) = default;
  constexpr variant_storage_move_assign_base& operator=(variant_storage_move_assign_base&& other) = default;
};

template <bool is_trivially_move_constructible, typename... Types>
struct variant_storage_move_ctor_base
    : variant_storage_move_assign_base<variant_traits<Types...>::trivial_move_assign, Types...> {};

template <typename... Types>
struct variant_storage_move_ctor_base<false, Types...>
    : variant_storage_move_assign_base<variant_traits<Types...>::trivial_move_assign, Types...> {
public:
  using base = variant_storage_move_assign_base<variant_traits<Types...>::trivial_move_assign, Types...>;
  using base::base;

  constexpr variant_storage_move_ctor_base(variant_storage_move_ctor_base const& other) = default;

  variant_storage_move_ctor_base(variant_storage_move_ctor_base&& other) noexcept(
      variant_utils::variant_traits<Types...>::nothrow_move_ctor) {
    variant<Types...>& this_variant = variant_cast(*this);
    variant<Types...>&& other_variant = variant_cast(std::forward<decltype(other)>(other));
    if (!other_variant.valueless_by_exception()) {
      variant_utils::visit_index<void>(
          [this, &other](auto index) {
            this->storage.template construct<index>(std::move(other.storage));
          },
          std::move(other_variant));
    }
    this->index_ = other.index_;
  }

  constexpr variant_storage_move_ctor_base& operator=(variant_storage_move_ctor_base const& other) = default;
  constexpr variant_storage_move_ctor_base& operator=(variant_storage_move_ctor_base&& other) = default;
};

template <typename... Types>
struct variant_storage_move_ctor_base<true, Types...>
    : variant_storage_move_assign_base<variant_traits<Types...>::trivial_move_assign, Types...> {
public:
  using base = variant_storage_move_assign_base<variant_traits<Types...>::trivial_move_assign, Types...>;
  using base::base;

  constexpr variant_storage_move_ctor_base(variant_storage_move_ctor_base const& other) = default;
  constexpr variant_storage_move_ctor_base(variant_storage_move_ctor_base&& other) = default;
  constexpr variant_storage_move_ctor_base& operator=(variant_storage_move_ctor_base const& other) = default;
  constexpr variant_storage_move_ctor_base& operator=(variant_storage_move_ctor_base&& other) = default;
};

template <bool is_trivially_copy_assignable, typename... Types>
struct variant_storage_copy_assign_base
    : variant_storage_move_ctor_base<variant_traits<Types...>::trivial_move_ctor, Types...> {};

template <typename... Types>
struct variant_storage_copy_assign_base<false, Types...>
    : variant_storage_move_ctor_base<variant_traits<Types...>::trivial_move_ctor, Types...> {
public:
  using base = variant_storage_move_ctor_base<variant_traits<Types...>::trivial_move_ctor, Types...>;
  using base::base;

  constexpr variant_storage_copy_assign_base(variant_storage_copy_assign_base const& other) = default;
  variant_storage_copy_assign_base(variant_storage_copy_assign_base&& other) = default;
  variant_storage_copy_assign_base& operator=(variant_storage_copy_assign_base&& other) = default;

  variant_storage_copy_assign_base& operator=(variant_storage_copy_assign_base const& other) noexcept(
      variant_utils::variant_traits<Types...>::nothrow_copy_assign) {
    variant<Types...>& this_variant = variant_cast(*this);
    variant<Types...> const& other_variant = variant_cast(std::forward<decltype(other)>(other));
    if (other_variant.valueless_by_exception()) {
      if (this_variant.valueless_by_exception()) {
        return *this;
      }
      this->reset();
      return *this;
    }
    variant_utils::visit_index<void>(
        [&this_variant, &other_variant](auto this_index, auto other_index) {
          if constexpr (this_index == other_index) {
            get<this_index>(this_variant) = get<other_index>(other_variant);
          } else {
            this_variant.template emplace<other_index>(get<other_index>(other_variant));
          }
        },
        this_variant, other_variant);
    return *this;
  }
};

template <typename... Types>
struct variant_storage_copy_assign_base<true, Types...>
    : variant_storage_move_ctor_base<variant_traits<Types...>::trivial_move_ctor, Types...> {
public:
  using base = variant_storage_move_ctor_base<variant_traits<Types...>::trivial_move_ctor, Types...>;
  using base::base;

  constexpr variant_storage_copy_assign_base(variant_storage_copy_assign_base const& other) = default;
  constexpr variant_storage_copy_assign_base(variant_storage_copy_assign_base&& other) = default;
  constexpr variant_storage_copy_assign_base& operator=(variant_storage_copy_assign_base const& other) = default;
  constexpr variant_storage_copy_assign_base& operator=(variant_storage_copy_assign_base&& other) = default;
};

template <bool is_trivially_copy_constructible, typename... Types>
struct variant_storage_copy_ctor_base
    : variant_storage_copy_assign_base<variant_traits<Types...>::trivial_copy_assign, Types...> {};

template <typename... Types>
struct variant_storage_copy_ctor_base<false, Types...>
    : variant_storage_copy_assign_base<variant_traits<Types...>::trivial_copy_assign, Types...> {
public:
  using base = variant_storage_copy_assign_base<variant_traits<Types...>::trivial_copy_assign, Types...>;
  using base::base;

  constexpr variant_storage_copy_ctor_base()
      : variant_storage_copy_ctor_base(in_place_index<0>) {}

  variant_storage_copy_ctor_base(variant_storage_copy_ctor_base const& other) noexcept(
      variant_utils::variant_traits<Types...>::nothrow_copy_ctor) {
    variant<Types...>& this_variant = variant_cast(*this);
    variant<Types...> const& other_variant = variant_cast(std::forward<decltype(other)>(other));
    if (!other_variant.valueless_by_exception()) {
      variant_utils::visit_index<void>(
          [this, &other](auto index) {
            this->storage.template construct<index>(other.storage);
          },
          other_variant);
    }
    this->index_ = other.index_;
  }

  variant_storage_copy_ctor_base(variant_storage_copy_ctor_base&& other) = default;
  variant_storage_copy_ctor_base& operator=(variant_storage_copy_ctor_base&& other) = default;
  variant_storage_copy_ctor_base& operator=(variant_storage_copy_ctor_base const& other) = default;
};

template <typename... Types>
struct variant_storage_copy_ctor_base<true, Types...>
    : variant_storage_copy_assign_base<variant_traits<Types...>::trivial_copy_assign, Types...> {
public:
  using base = variant_storage_copy_assign_base<variant_traits<Types...>::trivial_copy_assign, Types...>;
  using base::base;

  constexpr variant_storage_copy_ctor_base()
      : variant_storage_copy_ctor_base(in_place_index<0>) {}

  constexpr variant_storage_copy_ctor_base(variant_storage_copy_ctor_base const& other) = default;
  constexpr variant_storage_copy_ctor_base(variant_storage_copy_ctor_base&& other) = default;
  constexpr variant_storage_copy_ctor_base& operator=(variant_storage_copy_ctor_base&& other) = default;
  constexpr variant_storage_copy_ctor_base& operator=(variant_storage_copy_ctor_base const& other) = default;
};
} // namespace variant_utils