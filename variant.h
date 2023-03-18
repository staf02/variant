#pragma once
#include "variant_traits.h"
#include "variant_union.h"
#include "variant_utils.h"
#include <algorithm>

template <typename... Types>
class variant {
public:
  constexpr variant() requires(!variant_utils::default_ctor<Types...>) = delete;
  constexpr variant() noexcept(variant_utils::nothrow_default_ctor<Types...>)
      requires(variant_utils::default_ctor<Types...>)
      : storage(in_place_index<0>) {}

  constexpr variant(variant const& other) noexcept(variant_utils::nothrow_copy_ctor<Types...>) requires
      variant_utils::copy_ctor<Types...> {
    if (!other.valueless_by_exception()) {
      variant_utils::visit_index<void>(
          [this, &other](auto index) { this->storage.template construct<index>(other.storage); }, other);
    }
    this->index_ = other.index_;
  }
  constexpr variant(variant&& other) noexcept(variant_utils::nothrow_move_ctor<Types...>)
      requires(variant_utils::move_ctor<Types...>) {
    if (!other.valueless_by_exception()) {
      variant_utils::visit_index<void>(
          [this, &other](auto index) { this->storage.template construct<index>(std::move(other.storage)); },
          std::move(other));
    }
    this->index_ = other.index_;
  }

  variant& operator=(variant const& other) noexcept(variant_utils::nothrow_copy_assign<Types...>)
      requires(variant_utils::copy_assign<Types...>) {
    if (other.valueless_by_exception()) {
      if (this->valueless_by_exception()) {
        return *this;
      }
      this->reset();
      return *this;
    }
    variant_utils::visit_index<void>(
        [this, other](auto this_index, auto other_index) {
          if constexpr (this_index == other_index) {
            ::get<this_index>(*this) = ::get<other_index>(other);
          } else {
            this->template emplace<other_index>(::get<other_index>(other));
          }
        },
        *this, other);
    return *this;
  }

  variant& operator=(variant&& other) noexcept(variant_utils::nothrow_move_assign<Types...>)
      requires(variant_utils::move_assign<Types...>) {
    if (other.valueless_by_exception()) {
      if (this->valueless_by_exception()) {
        return *this;
      }
      this->reset();
      return *this;
    }
    variant_utils::visit_index<void>(
        [this, &other](auto this_index, auto other_index) {
          if constexpr (this_index == other_index) {
            ::get<this_index>(*this) = ::get<other_index>(std::move(other));
          } else {
            this->template emplace<other_index>(::get<other_index>(std::move(other)));
          }
        },
        *this, std::move(other));
    return *this;
  }

  constexpr variant(variant const& other)
      requires(variant_utils::copy_ctor<Types...>&& variant_utils::trivial_copy_ctor<Types...>) = default;
  constexpr variant(variant&& other)
      requires(variant_utils::move_ctor<Types...>&& variant_utils::trivial_move_ctor<Types...>) = default;
  variant& operator=(variant const& other)
      requires(variant_utils::copy_assign<Types...>&& variant_utils::trivial_copy_assign<Types...>) = default;
  variant& operator=(variant&& other)
      requires(variant_utils::move_assign<Types...>&& variant_utils::trivial_move_assign<Types...>) = default;

  template <typename T>
  requires(
      (sizeof...(Types) > 0) && !std::is_same_v<std::decay_t<T>, variant> &&
      std::is_constructible_v<variant_utils::find_overload_t<T, Types...>,
                              T>) constexpr variant(T&& t) noexcept(variant_utils::nothrow_convert_ctor<T, Types...>)
      : variant(in_place_type_t<variant_utils::find_overload_t<T, Types...>>(), std::forward<T>(t)) {}

  template <typename T>
  requires((sizeof...(Types) > 0) && !std::is_same_v<std::decay_t<T>, variant> &&
           std::is_assignable_v<variant_utils::find_overload_t<T, Types...>&, T> &&
           std::is_constructible_v<variant_utils::find_overload_t<T, Types...>, T>) constexpr variant&
  operator=(T&& t) noexcept(variant_utils::nothrow_convert_assign<T, Types...>) {
    using Target = variant_utils::find_overload_t<T, Types...>;
    if (this->index() == variant_utils::index_chooser_v<Target, Types...>) {
      ::get<variant_utils::index_chooser_v<Target, Types...>>(*this) = std::forward<T>(t);
    } else {
      if constexpr (std::is_nothrow_constructible_v<Target, T> || !std::is_nothrow_move_constructible_v<Target>) {
        this->template emplace<variant_utils::index_chooser_v<Target, Types...>>(std::forward<T>(t));
      } else {
        this->operator=(variant(std::forward<T>(t)));
      }
    }
    return *this;
  }

  constexpr ~variant() requires(!(std::is_trivially_destructible_v<Types> && ...)) {
    this->reset();
  }

  constexpr ~variant() requires(std::is_trivially_destructible_v<Types>&&...) = default;

  template <size_t Index, typename... Args>
  requires(Index < sizeof...(Types) &&
           std::is_constructible_v<variant_alternative_t<Index, variant<Types...>>,
                                   Args...>) explicit constexpr variant(in_place_index_t<Index>, Args&&... args)
      : storage(in_place_index<Index>, std::forward<Args>(args)...), index_(Index) {}

  template <typename T, typename... Args>
  requires(variant_utils::exactly_once_v<T, Types...>&& std::is_constructible_v<T, Args...>) constexpr explicit variant(
      in_place_type_t<T>, Args&&... args)
      : variant(in_place_index<variant_utils::index_chooser_v<T, Types...>>, std::forward<Args>(args)...) {}

  template <class T, class... Args>
  T& emplace(Args&&... args) {
    return emplace<variant_utils::index_chooser_v<T, Types...>>(std::forward<Args>(args)...);
  }

  template <size_t Index, class... Args>
  variant_alternative_t<Index, variant>& emplace(Args&&... args) {
    this->reset();
    auto& res = this->storage.template emplace<Index>(in_place_index<Index>, std::forward<Args>(args)...);
    this->index_ = Index;
    return res;
  }

  void swap(variant& other) noexcept(((std::is_nothrow_move_constructible_v<Types> &&
                                       std::is_nothrow_swappable_v<Types>)&&...)) {
    if (valueless_by_exception() && other.valueless_by_exception()) {
      return;
    } else if (valueless_by_exception()) {
      variant_utils::visit_index<void>(
          [this, &other](auto other_index) {
            this->template emplace<other_index>(::get<other_index>(std::move(other)));
          },
          std::forward<variant>(other));
      other.reset();
      return;
    } else if (other.valueless_by_exception()) {
      other.swap(*this);
      return;
    } else {
      variant_utils::visit_index<void>(
          [this, &other](auto this_index, auto other_index) {
            if constexpr (this_index == other_index) {
              using std::swap;
              swap(::get<this_index>(*this), ::get<this_index>(other));
            } else {
              std::swap(*this, other);
            }
          },
          *this, other);
    }
  }

  constexpr size_t index() const noexcept {
    return this->index_;
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

  void reset() {
    if (index_ != variant_npos) {
      variant_utils::visit_index<void>([this](auto this_index) { this->storage.template reset<this_index>(); }, *this);
      index_ = variant_npos;
    }
  }

  variant_utils::variant_union<Types...> storage;
  size_t index_{0};
};

template <class T, class... Types>
constexpr bool holds_alternative(const variant<Types...>& v) noexcept {
  return v.index() == variant_utils::index_chooser_v<T, Types...>;
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

template <class T, class... Types>
constexpr std::add_pointer_t<T> get_if(variant<Types...>* pv) noexcept {
  if (pv->index() == variant_utils::index_chooser_v<T, Types...>) {
    return std::addressof(get<variant_utils::index_chooser_v<T, Types...>>(*pv));
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
  if (w.valueless_by_exception()) {
    return false;
  } else if (v.valueless_by_exception()) {
    return true;
  }
  return variant_utils::visit_index<bool>(
      [&v, &w](auto index1, auto index2) -> bool {
        if constexpr (index1 < index2) {
          return true;
        } else if constexpr (index1 > index2) {
          return false;
        } else {
          return get<index1>(v) < get<index2>(w);
        }
      },
      v, w);
}

template <class... Types>
constexpr bool operator>(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception()) {
    return false;
  } else if (w.valueless_by_exception()) {
    return true;
  }
  return variant_utils::visit_index<bool>(
      [&v, &w](auto index1, auto index2) -> bool {
        if constexpr (index1 > index2) {
          return true;
        } else if constexpr (index1 < index2) {
          return false;
        } else {
          return get<index1>(v) > get<index2>(w);
        }
      },
      v, w);
}

template <class... Types>
constexpr bool operator<=(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception()) {
    return true;
  } else if (w.valueless_by_exception()) {
    return false;
  }
  return variant_utils::visit_index<bool>(
      [&v, &w](auto index1, auto index2) -> bool {
        if constexpr (index1 < index2) {
          return true;
        } else if constexpr (index1 > index2) {
          return false;
        } else {
          return get<index1>(v) <= get<index2>(w);
        }
      },
      v, w);
}

template <class... Types>
constexpr bool operator>=(const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception()) {
    return true;
  } else if (v.valueless_by_exception()) {
    return false;
  }
  return variant_utils::visit_index<bool>(
      [&v, &w](auto index1, auto index2) -> bool {
        if constexpr (index1 > index2) {
          return true;
        } else if constexpr (index1 < index2) {
          return false;
        } else {
          return get<index1>(v) >= get<index2>(w);
        }
      },
      v, w);
}