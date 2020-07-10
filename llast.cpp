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

void DJAssign::print(int offset) {
  // FIXME: look at printing behavior for a = new A()
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ ASSIGN\n";
  std::cout << std::string(4 * (offset + 1), ' ') << LHS << "\n";
  RHS->print(offset + 1);
}

void DJNull::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ NULL\n";
}

void DJNew::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ NEW("
            << assignee << ")\n";
}

void DJDotId::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ DOT ID ("
            << typeString(objectLikeType) << ", " << ID << "):\n";
  objectLike->print(offset + 1);
}

void DJDotAssign::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ')
            << "DJ DOT ASSIGN (" << typeString(objectLikeType) << ", " << ID
            << "):\n";
  objectLike->print(offset + 1);
  assignVal->print(offset + 1);
}

void DJInstanceOf::print(int offset) {
  std::cout << offset << ":" << std::string(4 * offset, ' ') << "DJ INSTANCEOF"
            << typeString(classID) << ":\n";
  objectLike->print(offset + 1);
}

std::string DJNat::className() { return "DJNat"; }

std::string DJFalse::className() { return "DJFalse"; }

std::string DJTrue::className() { return "DJTrue"; }

std::string DJPlus::className() { return "DJPlus"; }

std::string DJMinus::className() { return "DJMinus"; }

std::string DJTimes::className() { return "DJTimes"; }

std::string DJEqual::className() { return "DJEqual"; }

std::string DJGreater::className() { return "DJGreater"; }

std::string DJAnd::className() { return "DJAnd"; }

std::string DJPrint::className() { return "DJPrint"; }

std::string DJRead::className() { return "DJRead"; }

std::string DJNot::className() { return "DJNot"; }

std::string DJIf::className() { return "DJIf"; }

std::string DJFor::className() { return "DJFor"; }

std::string DJId::className() { return "DJId"; }

std::string DJAssign::className() { return "DJAssign"; }

std::string DJNull::className() { return "DJNull"; }

std::string DJNew::className() { return "DJNew"; }

std::string DJDotId::className() { return "DJDotId"; }

std::string DJDotAssign::className() { return "DJDotAssign"; }

std::string DJInstanceOf::className() { return "DJInstanceOf"; }
