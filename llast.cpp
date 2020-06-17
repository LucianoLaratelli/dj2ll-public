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

void DJFalse::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ FALSE\n";
}

void DJTrue::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ TRUE\n";
}

void DJPlus::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ PLUS\n";
  lhs->print(offset + 1);
  rhs->print(offset + 1);
}

void DJMinus::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ MINUS\n";
  lhs->print(offset + 1);
  rhs->print(offset + 1);
}

void DJTimes::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ TIMES\n";
  lhs->print(offset + 1);
  rhs->print(offset + 1);
}

void DJEqual::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ EQUAL\n";
  lhs->print(offset + 1);
  rhs->print(offset + 1);
}

void DJGreater::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ GREATER\n";
  lhs->print(offset + 1);
  rhs->print(offset + 1);
}

void DJAnd::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ AND\n";
  lhs->print(offset + 1);
  rhs->print(offset + 1);
}

void DJPrint::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ PRINT\n";
  printee->print(offset + 1);
}

void DJRead::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ READ\n";
}

void DJNot::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ NOT\n";
}

void DJIf::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ IF\n";
  cond->print(offset + 1);
  for (auto &e : thenBlock) {
    e->print(offset + 1);
  }
  for (auto &e : elseBlock) {
    e->print(offset + 1);
  }
}

void DJFor::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ FOR\n";
  init->print(offset + 1);
  test->print(offset + 1);
  update->print(offset + 1);
  for (auto &e : body) {
    e->print(offset + 1);
  }
}

void DJId::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ ID(" << ID
            << ")\n";
}
