#include "llast.hpp"
#include <iostream>

void DJProgram::print() {
  std::cout << 0 << ":"
            << "DJ PROGRAM\n";
  for (auto e : mainExprs) {
    e->print(1);
  }
}

void DJNat::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ NAT("
            << value << ")\n";
}

void DJPlus::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ PLUS\n";
  lhs->print(offset + 1);
  rhs->print(offset + 1);
}
