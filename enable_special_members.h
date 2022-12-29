#pragma once

namespace variant_utils {
struct default_constructor_tag {
  explicit constexpr default_constructor_tag() = default;
};

template <bool Default, typename Tag = void>
struct default_constructor_base {
  constexpr default_constructor_base() noexcept = default;
  constexpr default_constructor_base(default_constructor_base const&) noexcept = default;
  constexpr default_constructor_base(default_constructor_base&&) noexcept = default;
  constexpr default_constructor_base& operator=(default_constructor_base const&) noexcept = default;
  constexpr default_constructor_base& operator=(default_constructor_base&&) noexcept = default;

  constexpr explicit default_constructor_base(default_constructor_tag) {}
};
template <typename Tag>
struct default_constructor_base<false, Tag> {
  constexpr default_constructor_base() noexcept = delete;
  constexpr default_constructor_base(default_constructor_base const&) noexcept = default;
  constexpr default_constructor_base(default_constructor_base&&) noexcept = default;
  constexpr default_constructor_base& operator=(default_constructor_base const&) noexcept = default;
  constexpr default_constructor_base& operator=(default_constructor_base&&) noexcept = default;

  constexpr explicit default_constructor_base(default_constructor_tag) {}
};

template <bool Copy, typename Tag = void>
struct copy_constructor_base {
  constexpr copy_constructor_base() noexcept = default;
  constexpr copy_constructor_base(copy_constructor_base const&) noexcept = default;
  constexpr copy_constructor_base(copy_constructor_base&&) noexcept = default;
  constexpr copy_constructor_base& operator=(copy_constructor_base const&) noexcept = default;
  constexpr copy_constructor_base& operator=(copy_constructor_base&&) noexcept = default;
};
template <typename Tag>
struct copy_constructor_base<false, Tag> {
  constexpr copy_constructor_base() noexcept = default;
  constexpr copy_constructor_base(copy_constructor_base const&) noexcept = delete;
  constexpr copy_constructor_base(copy_constructor_base&&) noexcept = default;
  constexpr copy_constructor_base& operator=(copy_constructor_base const&) noexcept = default;
  constexpr copy_constructor_base& operator=(copy_constructor_base&&) noexcept = default;
};

template <bool Move, typename Tag = void>
struct move_constructor_base {
  constexpr move_constructor_base() noexcept = default;
  constexpr move_constructor_base(move_constructor_base const&) noexcept = default;
  constexpr move_constructor_base(move_constructor_base&&) noexcept = default;
  constexpr move_constructor_base& operator=(move_constructor_base const&) noexcept = default;
  constexpr move_constructor_base& operator=(move_constructor_base&&) noexcept = default;
};
template <typename Tag>
struct move_constructor_base<false, Tag> {
  constexpr move_constructor_base() noexcept = default;
  constexpr move_constructor_base(move_constructor_base const&) noexcept = default;
  constexpr move_constructor_base(move_constructor_base&&) noexcept = delete;
  constexpr move_constructor_base& operator=(move_constructor_base const&) noexcept = default;
  constexpr move_constructor_base& operator=(move_constructor_base&&) noexcept = default;
};

template <bool CopyAssignment, typename Tag = void>
struct copy_assignment_base {
  constexpr copy_assignment_base() noexcept = default;
  constexpr copy_assignment_base(copy_assignment_base const&) noexcept = default;
  constexpr copy_assignment_base(copy_assignment_base&&) noexcept = default;
  constexpr copy_assignment_base& operator=(copy_assignment_base const&) noexcept = default;
  constexpr copy_assignment_base& operator=(copy_assignment_base&&) noexcept = default;
};
template <typename Tag>
struct copy_assignment_base<false, Tag> {
  constexpr copy_assignment_base() noexcept = default;
  constexpr copy_assignment_base(copy_assignment_base const&) noexcept = default;
  constexpr copy_assignment_base(copy_assignment_base&&) noexcept = default;
  constexpr copy_assignment_base& operator=(copy_assignment_base const&) noexcept = delete;
  constexpr copy_assignment_base& operator=(copy_assignment_base&&) noexcept = default;
};

template <bool MoveAssignment, typename Tag = void>
struct move_assignment_base {
  constexpr move_assignment_base() noexcept = default;
  constexpr move_assignment_base(move_assignment_base const&) noexcept = default;
  constexpr move_assignment_base(move_assignment_base&&) noexcept = default;
  constexpr move_assignment_base& operator=(move_assignment_base const&) noexcept = default;
  constexpr move_assignment_base& operator=(move_assignment_base&&) noexcept = default;
};
template <typename Tag>
struct move_assignment_base<false, Tag> {
  constexpr move_assignment_base() noexcept = default;
  constexpr move_assignment_base(move_assignment_base const&) noexcept = default;
  constexpr move_assignment_base(move_assignment_base&&) noexcept = default;
  constexpr move_assignment_base& operator=(move_assignment_base const&) noexcept = default;
  constexpr move_assignment_base& operator=(move_assignment_base&&) noexcept = delete;
};
}