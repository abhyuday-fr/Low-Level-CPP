#include <iostream>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <vector>

int main(int argc, char **argv){
  std::vector<std::string> args(argv + 1, argv + argc);
  std::cout << "Got Arguments: ";
  std::copy(args.begin(), args.end(), std::ostream_iterator<std::string>(std::cout, " "));
  return EXIT_SUCCESS;
}