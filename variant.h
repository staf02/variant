#pragma once
#include "enable_special_members.h"
#include "variant_storage.h"
#include "variant_union.h"
#include "variant_utils.h"

template <typename... Types>
struct variant
    : variant_utils::variant_storage_copy_ctor_base<variant_utils::variant_traits<Types...>::trivial_copy_ctor,
                                                    Types...>,
      private variant_utils::default_constructor_base<variant_utils::variant_traits<Types...>::default_ctor>,
      private variant_utils::copy_constructor_base<variant_utils::variant_traits<Types...>::copy_ctor>,
      private variant_utils::move_constructor_base<variant_utils::variant_traits<Types...>::move_ctor>,
      private variant_utils::copy_assignment_base<variant_utils::variant_traits<Types...>::copy_assign>,
      private variant_utils::move_assignment_base<variant_utils::variant_traits<Types...>::move_assign> {
public:
  using base = variant_utils::variant_storage_copy_ctor_base<variant_utils::variant_traits<Types...>::trivial_copy_ctor,
                                                             Types...>;
  using base::base;

  constexpr variant() noexcept = default;
  constexpr variant(variant const& other) = default;
  constexpr variant(variant&& other) = default;
  variant& operator=(variant const& other) = default;
  variant& operator=(variant&& other) = default;

  template <typename T,
            std::enable_if_t<variant_utils::variant_traits<Types...>::template converting_constructible<T>, int> = 0>
  constexpr variant(T&& t)
      : variant(in_place_index<variant_utils::index_chooser_v<T, variant<Types...>>>, std::forward<T>(t)) {}

  template <typename T,
            std::enable_if_t<variant_utils::variant_traits<Types...>::template converting_constructible<T>, int> = 0>
  constexpr variant& operator=(T&& t) {
    constexpr size_t j = variant_utils::index_chooser_v<T, variant<Types...>>;
    if (j == index()) {
      this->get<j>(in_place_index<j>) = std::forward<T>(t);
    } else {
      using T_j = variant_alternative<j, variant<Types...>>;
      if (std::is_nothrow_constructible_v<T_j, T> || !std::is_nothrow_move_constructible_v<T_j>) {
        this->emplace<j>(std::forward<T>(t));
      } else {
        operator=(variant(std::forward<T>(t)));
      }
    }
    return *this;
  }

  constexpr ~variant() = default;

  template <size_t Index, class... Args>
  explicit constexpr variant(in_place_index_t<Index>, Args&&... args)
      : base(in_place_index<Index>, std::forward<Args>(args)...),
        variant_utils::default_constructor_base<variant_utils::variant_traits<Types...>::default_ctor>(
            variant_utils::default_constructor_tag()) {}

  template <class T, class... Args>
  constexpr explicit variant(in_place_type_t<T>, Args&&... args)
      : base(in_place_index<variant_utils::index_chooser_v<T, variant<Types...>>>, std::forward<Args>(args)...),
        variant_utils::default_constructor_base<variant_utils::variant_traits<Types...>::default_ctor>(
            variant_utils::default_constructor_tag()) {}

  constexpr size_t index() const noexcept {
    return this->index_;
  }

  template <class T, class... Args>
  T& emplace(Args&&... args) {
    return emplace<variant_utils::index_chooser_v<T, variant<Types...>>>(std::forward<Args>(args)...);
  }

  template <size_t Index, class... Args>
  variant_alternative_t<Index, variant>& emplace(Args&&... args) {
    this->reset();
    auto& res = this->storage.template emplace<Index>(in_place_index<Index>, std::forward<Args>(args)...);
    this->index_ = Index;
    return res;
  }

  constexpr bool valueless_by_exception() const noexcept {
    return this->index_ == variant_npos;
  }

private:
  template <size_t Index, class... Args>
  friend constexpr variant_alternative_t<Index, variant<Args...>>& get(variant<Args...>& v);
  template <std::size_t Index, class... Args>
  friend constexpr variant_alternative_t<Index, variant<Args...>>&& get(variant<Args...>&& v);
  template <std::size_t Index, class... Args>
  friend constexpr const variant_alternative_t<Index, variant<Args...>>& get(const variant<Args...>& v);
  template <std::size_t Index, class... Args>
  friend constexpr const variant_alternative_t<Index, variant<Args...>>&& get(const variant<Args...>&& v);

  template <size_t Index>
  constexpr auto& get(in_place_index_t<Index>) {
    return this->storage.get(in_place_index<Index>);
  }

  template <size_t Index>
  constexpr auto const& get(in_place_index_t<Index>) const noexcept {
    return this->storage.get(in_place_index<Index>);
  }
};

template <class T, class... Args>
constexpr bool holds_alternative(const variant<Args...>& v) noexcept {
  return v.index() == variant_utils::index_chooser_v<T, variant<Args...>>;
}

template <size_t I, class... Args>
constexpr std::add_pointer_t<variant_alternative_t<I, variant<Args...>>> get_if(variant<Args...>* pv) noexcept {
  if (pv->index() == I) {
    return std::addressof(get<I>(*pv));
  }
  return nullptr;
}

template <std::size_t I, class... Args>
constexpr std::add_pointer_t<const variant_alternative_t<I, variant<Args...>>>
get_if(const variant<Args...>* pv) noexcept {
  return get_if(pv);
}

template <class T, class... Args>
constexpr std::add_pointer_t<T> get_if(variant<Args...>* pv) noexcept {
  if (pv->index() == variant_utils::index_chooser_v<T, variant<Args...>>) {
    return std::addressof(get<variant_utils::index_chooser_v<T, variant<Args...>>>(*pv));
  }
  return nullptr;
}

template <class T, class... Args>
constexpr std::add_pointer_t<const T> get_if(const variant<Args...>* pv) noexcept {
  return get_if(pv);
}

template <class... Types>
constexpr bool operator==(const variant<Types...>& v, const variant<Types...>& w) {
  return variant_utils::visit_index<bool>(
      [&v, &w](auto index1, auto index2) {
        if constexpr (index1 != index2) {
          return false;
        } else {
          if (v.valueless_by_exception()) {
            return true;
          } else {
            return get<index1>(v) == get<index2>(w);
          }
        }
      },
      v, w);
}

template <class... Types>
constexpr bool operator!=(const variant<Types...>& v, const variant<Types...>& w) {
  return variant_utils::visit_index<bool>(
      [&v, &w](auto index1, auto index2) {
        if constexpr (index1 != index2) {
          return true;
        } else {
          if (v.valueless_by_exception()) {
            return false;
          } else {
            return get<index1>(v) != get<index2>(w);
          }
        }
      },
      v, w);
}

template <class... Types>
constexpr bool operator<(const variant<Types...>& v, const variant<Types...>& w) {
  return variant_utils::visit_index<bool>(
      [&v, &w](auto index1, auto index2) -> bool {
        if (w.valueless_by_exception()) {
          return false;
        } else if (v.valueless_by_exception()) {
          return true;
        } else {
          if constexpr (index1 < index2) {
            return true;
          } else if constexpr (index1 > index2) {
            return false;
          } else {
            return get<index1>(v) < get<index2>(w);
          }
        }
      },
      v, w);
}

template <class... Types>
constexpr bool operator>(const variant<Types...>& v, const variant<Types...>& w) {
  return variant_utils::visit_index<bool>(
      [&v, &w](auto index1, auto index2) -> bool {
        if (v.valueless_by_exception()) {
          return false;
        } else if (w.valueless_by_exception()) {
          return true;
        } else {
          if constexpr (index1 > index2) {
            return true;
          } else if constexpr (index1 < index2) {
            return false;
          } else {
            return get<index1>(v) > get<index2>(w);
          }
        }
      },
      v, w);
}

template <class... Types>
constexpr bool operator<=(const variant<Types...>& v, const variant<Types...>& w) {
  return variant_utils::visit_index(
      [&v, &w](auto index1, auto index2) -> bool {
        if (w.valueless_by_exception()) {
          return false;
        } else if (v.valueless_by_exception()) {
          return true;
        } else {
          if constexpr (index1 < index2) {
            return true;
          } else if constexpr (index1 > index2) {
            return false;
          } else {
            return get<index1>(v) <= get<index2>(w);
          }
        }
      },
      v, w);
}

template <class... Types>
constexpr bool operator>=(const variant<Types...>& v, const variant<Types...>& w) {
  return variant_utils::visit_index<bool>(
      [&v, &w](auto index1, auto index2) -> bool {
        if (v.valueless_by_exception()) {
          return false;
        } else if (w.valueless_by_exception()) {
          return true;
        } else {
          if constexpr (index1 > index2) {
            return true;
          } else if constexpr (index1 < index2) {
            return false;
          } else {
            return get<index1>(v) >= get<index2>(w);
          }
        }
      },
      v, w);
}