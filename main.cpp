//
// Created by staf02 on 15/12/22.

#include "variant.h"
//#include <variant>
#include "test-classes.h"
#include <iostream>
#include <type_traits>
#include <vector>

// static_assert(variant_utils::variant_traits<int, int const, double>::default_ctor == true);

struct print_in_constr {
  print_in_constr() {
    std::cout << "Hello";
  }
};

int main() {
  using variant1 = variant<int, float, std::string, trivial_t, std::vector<int>, no_default_t>;
  std::cout << (std::is_constructible_v<variant1, in_place_index_t<1337>>);
  return 0;
};