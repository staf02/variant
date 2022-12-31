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
  auto t = variant_utils::index_chooser_v<int, variant<int, const int, int const, volatile int>>;
  std::cout << t;
  return 0;
};