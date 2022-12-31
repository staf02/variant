//
// Created by staf02 on 15/12/22.

#include "variant.h"
//#include <variant>
#include "test-classes.h"
#include <iostream>
#include <type_traits>
#include <vector>

// static_assert(variant_utils::variant_traits<int, int const, double>::default_ctor == true);


int main() {
  std::cout << variant_utils::index_chooser_v<std::vector<int>, variant<std::vector<int>, throwing_move_operator_t>>;
  using V = variant<std::vector<int>, throwing_move_operator_t>;
  std::vector<int> v1 = std::vector{1, 2, 3};
  V v = v1;
  return 0;
};