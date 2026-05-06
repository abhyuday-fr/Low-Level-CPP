#include "my_function.hpp"
#include <iostream>

int main() {

  int x{0};
  my::function<void(int)> f = [&](int x) { x += 1; };

  std::cout << x << "\n";

  return 0;
}
