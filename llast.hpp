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
  virtual llvm::Value *codeGen(int type = -1) = 0;
};

class DJProgram : public DJNode {
public:
  bool hasInstanceOf;
  bool runOptimizations;
  // ClassDeclList classes;
  // VarDeclList mainDecls;
  ExprList mainExprs;
  // DJProgram(ClassDeclList classes, VarDeclList mainDecls, ExprList mainExprs)
  //     : classes(classes), mainDecls(mainDecls), mainExprs(mainExprs) {}
  DJProgram(ExprList mainExprs)
      : hasInstanceOf(false), runOptimizations(false), mainExprs(mainExprs) {}
  // the value of type is only ever utilized in DJNull::codeGen()
  llvm::Function *codeGen(int type = -1) override;
  void print();
};

class DJExpression : public DJNode {
public:
  /*these variables are only assigned in and used from DJDotMethod, DJDotAssign,
   * DJAssign, DJEqual, and DJInstanceOf; these are the only expressions that
   * can have a child DJNull whose type is meaningful*/
  bool hasNullChild;
  int nullChildCount;
  virtual void print(int offset = 0) = 0;
  virtual std::string className() = 0;
};

class DJNat : public DJExpression {
public:
  unsigned int value;
  DJNat(unsigned int value) : value(value) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJFalse : public DJExpression {
public:
  // unsigned int value;
  // DJFalse(unsigned int value) : value(value) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJTrue : public DJExpression {
public:
  // unsigned int value;
  // DJTrue(unsigned int value) : value(value) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJPlus : public DJExpression {
public:
  DJExpression *lhs, *rhs;
  DJPlus(DJExpression *lhs, DJExpression *rhs) : lhs(lhs), rhs(rhs) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJMinus : public DJExpression {
public:
  DJExpression *lhs, *rhs;
  DJMinus(DJExpression *lhs, DJExpression *rhs) : lhs(lhs), rhs(rhs) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJTimes : public DJExpression {
public:
  DJExpression *lhs, *rhs;
  DJTimes(DJExpression *lhs, DJExpression *rhs) : lhs(lhs), rhs(rhs) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJPrint : public DJExpression {
public:
  DJExpression *printee;
  DJPrint(DJExpression *printee) : printee(printee) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJRead : public DJExpression {
public:
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJNot : public DJExpression {
public:
  DJExpression *negated;
  DJNot(DJExpression *negated) : negated(negated) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJEqual : public DJExpression {
public:
  DJExpression *lhs, *rhs;
  bool bothNull;
  bool leftNull;
  bool rightNull;
  int nonNullType;
  DJEqual(DJExpression *lhs, DJExpression *rhs) : lhs(lhs), rhs(rhs) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJGreater : public DJExpression {
public:
  DJExpression *lhs, *rhs;
  DJGreater(DJExpression *lhs, DJExpression *rhs) : lhs(lhs), rhs(rhs) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJAnd : public DJExpression {
public:
  DJExpression *lhs, *rhs;
  DJAnd(DJExpression *lhs, DJExpression *rhs) : lhs(lhs), rhs(rhs) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJIf : public DJExpression {
public:
  DJExpression *cond;
  ExprList thenBlock, elseBlock;
  DJIf(DJExpression *cond, ExprList thenBlock, ExprList elseBlock)
      : cond(cond), thenBlock(thenBlock), elseBlock(elseBlock) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJFor : public DJExpression {
public:
  DJExpression *init, *test, *update;
  ExprList body;
  DJFor(DJExpression *init, DJExpression *test, DJExpression *update,
        ExprList body)
      : init(init), test(test), update(update), body(body) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJId : public DJExpression {
public:
  std::string ID;
  DJId(char *ID) : ID(ID){};
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJAssign : public DJExpression {
public:
  std::string LHS;
  int LHSType;
  DJExpression *RHS;
  DJAssign(char *LHS, int LHSType, DJExpression *RHS)
      : LHS(LHS), LHSType(LHSType), RHS(RHS) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJNull : public DJExpression {
public:
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJNew : public DJExpression {
public:
  std::string assignee;
  int classID;
  DJNew(char *assignee, int classID) : assignee(assignee), classID(classID) {}
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJDotId : public DJExpression {
public:
  DJExpression *objectLike;
  int objectLikeType;
  std::string ID;
  DJDotId(DJExpression *objectLike, int objectLikeType, char *ID)
      : objectLike(objectLike), objectLikeType(objectLikeType), ID(ID){};
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJDotAssign : public DJExpression {
public:
  DJExpression *objectLike;
  int objectLikeType;
  std::string ID;
  DJExpression *assignVal;
  DJDotAssign(DJExpression *objectLike, int objectLikeType, char *ID,
              DJExpression *assignVal)
      : objectLike(objectLike), objectLikeType(objectLikeType), ID(ID),
        assignVal(assignVal){};
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

class DJInstanceOf : public DJExpression {
public:
  DJExpression *objectLike;
  int objectLikeType;
  int classID;
  DJInstanceOf(DJExpression *objectLike, int objectLikeType, int classID)
      : objectLike(objectLike), objectLikeType(objectLikeType),
        classID(classID){};
  llvm::Value *codeGen(int type = -1) override;
  void print(int offset = 0) override;
  std::string className() override;
};

#endif // __LLAST_H_
