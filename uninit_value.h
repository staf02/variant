#include "variant_utils.h"
#include <type_traits>
#include <iostream>

namespace variant_utils {

template <typename Type, bool = std::is_trivially_destructible_v<Type>>
struct uninit_value;

template <typename Type>
struct uninit_value<Type, true> {

  constexpr uninit_value() = default;
  constexpr uninit_value(uninit_value const&) = default;
  constexpr uninit_value(uninit_value&&) = default;
  constexpr uninit_value& operator=(uninit_value const&) = default;
  constexpr uninit_value& operator=(uninit_value&&) = default;

  template <typename... Args>
  constexpr uninit_value(Args&&... args) : storage(std::forward<Args>(args)...) {}

  template<typename OtherHolder>
  static constexpr void construct_uninit_value(uninit_value* holder, OtherHolder&& other) {
    new (holder) uninit_value(std::forward<OtherHolder>(other));
  }

  constexpr Type const& get() const& noexcept {
    return storage;
  }

  constexpr Type const&& get() const&& noexcept {
    return std::move(storage);
  }

  constexpr Type& get()& noexcept {
    return storage;
  }

  constexpr Type&& get()&& noexcept {
    return std::move(storage);
  }

  void construct(uninit_value const& other) {
    std::construct_at(&storage, other.get());
  }

  void construct(uninit_value&& other) {
    std::construct_at(&storage, std::move(other.get()));
  }

  void reset() {
    this->~uninit_value();
  }

  ~uninit_value() = default;

  Type storage;
};

template <typename Type>
struct uninit_value<Type, false> {

  constexpr uninit_value() noexcept(std::is_nothrow_default_constructible_v<Type>) {
    new(reinterpret_cast<Type*>(&storage)) Type();
  }

  constexpr uninit_value(uninit_value const&) = default;
  constexpr uninit_value(uninit_value&&) noexcept(std::is_nothrow_move_constructible_v<Type>) = default;
  constexpr uninit_value& operator=(uninit_value const&) = default;
  constexpr uninit_value& operator=(uninit_value&&) noexcept(std::is_nothrow_move_assignable_v<Type>) = default;

  template <typename... Args>
  uninit_value(Args&&... args) {
    std::construct_at(reinterpret_cast<Type*>(&storage), std::forward<Args>(args)...);
  }

  template<typename OtherHolder>
  static constexpr void construct_uninit_value(uninit_value* holder, OtherHolder&& other) {
    new (holder) uninit_value(std::forward<OtherHolder>(other).get());
  }

  const Type& get() const& noexcept {
    return *reinterpret_cast<Type const*>(&storage);
  }

  Type& get() & noexcept {
    return *reinterpret_cast<Type*>(&storage);
  }

  const Type&& get() const&& noexcept {
    return std::move(*reinterpret_cast<Type const*>(&storage));
  }

  Type&& get() && noexcept {
    return std::move(*reinterpret_cast<Type*>(&storage));
  }

  void construct(uninit_value const& other) {
    std::construct_at(reinterpret_cast<Type*>(&storage), other.get());
  }

  void construct(uninit_value&& other) {
    std::construct_at(reinterpret_cast<Type*>(&storage), std::move(other.get()));
  }

  void reset() {
    std::destroy_at(reinterpret_cast<Type*>(&storage));
  }

  std::aligned_storage_t<sizeof(Type), alignof(Type)> storage;
};
} // namespace variant_utils