#ifndef __LLAST_H_
#define __LLAST_H_

/* defines class and functions for the AST that will be used to perform code
 * generation -- essentially a C++-ized version of the original AST defined
 * ast.h */

#include "util.h"
#include <memory>
#include <vector>

class ExprAST {
  std::vector<std::unique_ptr<ExprAST>> children;
  unsigned int lineNumber;

public:
  virtual ~ExprAST() = default;
  void setLineNo(unsigned int lineNumber) { this->lineNumber = lineNumber; }
  void setChildren(std::vector<std::unique_ptr<ExprAST>> c) {
    this->children = c;
  }
};

std::unique_ptr<ExprAST> translateAST(ASTree *t);

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
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  plusExprAST(std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
      : LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class minusExprAST : public ExprAST {
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  minusExprAST(std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
      : LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class timesExprAST : public ExprAST {
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  timesExprAST(std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
      : LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

#endif // __LLAST_H_
