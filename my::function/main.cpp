#include "my_function.hpp"
#include <iostream>

void free_function(int x) {
  std::cout << "Free fnction called with: " << x << "\n";
}

int main() {
  my::function<void(int)> f1 = free_function;
  f1(10);

  int multiplier = 5;
  my::function<int(int)> f2 = [multiplier](int x) { return x * multiplier; };

  std::cout << "Lambda returned: " << f2(10) << "\n";

  return 0;
}
