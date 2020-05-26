#ifndef __LLAST_H_
#define __LLAST_H_

/* defines class and functions for the AST that will be used to perform code
 * generation -- essentially a C++-ized version of the original AST defined
 * ast.h */

#include "util.h"
#include <vector>

class ExprAST {
  std::vector<ExprAST> children;
  unsigned int lineNumber;

public:
  virtual ~ExprAST() = default;
  void setLineNo(unsigned int lineNumber) { this->lineNumber = lineNumber; }
  void setChildren(std::vector<ExprAST> c) { this->children = c; }
};

ExprAST translateAST(ASTree *t);

class NatLiteralExprAST : public ExprAST {
  unsigned int natVal;

public:
  NatLiteralExprAST(unsigned int natVal) : natVal(natVal) {}
};

class BoolLiteralExprAST : public ExprAST {
  bool boolVal;

public:
  BoolLiteralExprAST(bool boolVal) : boolVal(boolVal) {}
};

class plusExprAST : public ExprAST {
  ExprAST LHS, RHS;

public:
  plusExprAST(ExprAST LHS, ExprAST RHS)
      : LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class minusExprAST : public ExprAST {
  ExprAST LHS, RHS;

public:
  minusExprAST(ExprAST LHS, ExprAST RHS)
      : LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class timesExprAST : public ExprAST {
  ExprAST LHS, RHS;

public:
  timesExprAST(ExprAST LHS, ExprAST RHS)
      : LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

#endif // __LLAST_H_
