#include "codegen.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <string>

using namespace llvm;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

void generateLL(std::ofstream &outputFile) {}

/* Generate DISM code for an expression list, which appears in the given class
and method (or main block).If classNumber < 0 then methodNumber may be
anything and we assume we are generating code for the program's main block.
*/
void codeGenExprs(ASTree *expList, int classNumber, int methodNumber) {
  NULL_CHECK(expList);
  ASTList *expr_list_iter = expList->children;
  NULL_CHECK(expr_list_iter);
  while (expr_list_iter != NULL) {
    codeGenExpr(expr_list_iter->data, classNumber, methodNumber);
    if (expr_list_iter->next != NULL) {
    }
    expr_list_iter = expr_list_iter->next;
  }
}

// Value *codeGenValueExpr(ASTree *t, int classNumber, int methodNumber) {}

void codeGenExpr(ASTree *t, int classNumber, int methodNumber) {
  NULL_CHECK(t);
  int CN = classNumber;
  int MN = methodNumber;

  int childCount = 0; // how many children this node has
  ASTList *thisChild = t->children;
  struct astnode *children[5];
  if (t->typ != NULL_EXPR && t->typ != NAT_LITERAL_EXPR &&
      t->typ != TRUE_LITERAL_EXPR && t->typ != FALSE_LITERAL_EXPR &&
      t->typ != READ_EXPR && t->typ != THIS_EXPR) {
    // skip codegen of t's children when t doesn't have any children
    while (thisChild != NULL) {
      children[childCount] = thisChild->data;
      childCount++;
      thisChild = thisChild->next;
    }
    int i = 0;
    for (; i < childCount; i++) {
      NULL_CHECK(children[i]);
      // skip codegen of these children; code must be generated for them in a
      // specific way (because of short-circuiting, control flow, variable
      // lookup, etc)
      if (t->typ != AND_EXPR && t->typ != IF_THEN_ELSE_EXPR &&
          t->typ != FOR_EXPR && t->typ != ID_EXPR && t->typ != ASSIGN_EXPR &&
          t->typ != NEW_EXPR && t->typ != INSTANCEOF_EXPR &&
          t->typ != DOT_ID_EXPR && t->typ != DOT_ASSIGN_EXPR &&
          t->typ != DOT_METHOD_CALL_EXPR && t->typ != METHOD_CALL_EXPR) {
        codeGenExpr(children[i], CN, MN);
      }
    }
  }
  switch (t->typ) {
  case DOT_METHOD_CALL_EXPR: { /*E.ID(A)*/
    return;
  }
  case METHOD_CALL_EXPR: { /*ID(A)*/
    return;
  }
  case DOT_ID_EXPR: { /*E.ID*/
    return;
  }
  case ID_EXPR: {
    return;
  }
  case DOT_ASSIGN_EXPR: { /*E.ID = E'*/
    return;
  }
  case ASSIGN_EXPR: {
    return;
  }
  case PLUS_EXPR: {
    return;
  }
  case MINUS_EXPR: {
    return;
  }
  case TIMES_EXPR: {
    return;
  }
  case EQUALITY_EXPR: {
    return;
  }
  case GREATER_THAN_EXPR: {
    return;
  }
  case NOT_EXPR: {
    return;
  }
  case INSTANCEOF_EXPR: {
    return;
  }
  case IF_THEN_ELSE_EXPR: {
    return;
  }
  case FOR_EXPR: {
    return;
  }
  case READ_EXPR: {
    return;
  }
  case THIS_EXPR: {
    return;
  }
  case NEW_EXPR: {
    return;
  }
  case NULL_EXPR: {
    return;
  }
  case NAT_LITERAL_EXPR: {
    APInt natVal = APInt();
    natVal = t->natVal;
    return; // ConstantInt::get(TheContext, natVal) ;
  }
  case TRUE_LITERAL_EXPR: {
    return;
  }
  case FALSE_LITERAL_EXPR: {
    return;
  }
  default: {
    exit(-1);
  }
  }
}
