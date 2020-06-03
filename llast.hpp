#ifndef __LLAST_H_
#define __LLAST_H_
#include "llvm_includes.hpp"
#include "util.h"
#include <vector>

/* defines class and functions for the AST that will be used to perform code
 * generation*/

class CodeGenContext;
class DJClassDecl;
class DJVarDecl;
class DJExpression;

typedef std::vector<DJClassDecl *> ClassDeclList;
typedef std::vector<DJVarDecl *> VarDeclList;
typedef std::vector<DJExpression *> ExprList;

class DJNode {
public:
  virtual ~DJNode() {}
  virtual llvm::Value *codeGen() = 0;
};

class DJProgram : public DJNode {
public:
  // ClassDeclList classes;
  // VarDeclList mainDecls;
  ExprList mainExprs;
  // DJProgram(ClassDeclList classes, VarDeclList mainDecls, ExprList mainExprs)
  //     : classes(classes), mainDecls(mainDecls), mainExprs(mainExprs) {}
  DJProgram(ExprList mainExprs) : mainExprs(mainExprs) {}
  llvm::Function *codeGen() override;
  void print();
};

class DJExpression : public DJNode {
public:
  virtual void print(int offset) = 0;
};

class DJNat : public DJExpression {
public:
  unsigned int value;
  DJNat(unsigned int value) : value(value) {}
  llvm::Value *codeGen() override;
  void print(int offset) override;
};

// class DJBool : public DJExpression {
// public:
//   bool value;
//   DJBool(bool value) : value(value) {}
//   virtual llvm::Value *codeGen(CodeGenContext &context);
// };

class DJPlus : public DJExpression {
public:
  DJExpression *lhs;
  DJExpression *rhs;
  DJPlus(DJExpression *lhs, DJExpression *rhs) : lhs(lhs), rhs(rhs) {}
  llvm::Value *codeGen() override;
  void print(int offset) override;
};

class DJMinus : public DJExpression {
public:
  DJExpression *lhs;
  DJExpression *rhs;
  DJMinus(DJExpression *lhs, DJExpression *rhs) : lhs(lhs), rhs(rhs) {}
  llvm::Value *codeGen() override;
  void print(int offset) override;
};

class DJTimes : public DJExpression {
public:
  DJExpression *lhs;
  DJExpression *rhs;
  DJTimes(DJExpression *lhs, DJExpression *rhs) : lhs(lhs), rhs(rhs) {}
  llvm::Value *codeGen() override;
  void print(int offset) override;
};

class DJPrint : public DJExpression {
public:
  DJExpression *printee;
  DJPrint(DJExpression *printee) : printee(printee) {}
  llvm::Value *codeGen() override;
  void print(int offset) override;
};

#endif // __LLAST_H_
