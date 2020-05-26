#include "codegen.hpp"

#include "llast.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <stack>
#include <string>

using namespace llvm;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;
;

llvm::Value *DJProgram::codeGen() {
  llvm::Value *last = nullptr;
  // for (auto c : classes) {
  //   last = c->codeGen(context);
  // }
  // for (auto m : mainDecls) {
  //   last = m->codeGen(context);
  // }
  for (auto e : mainExprs) {
    last = e->codeGen();
  }
  return last;
}

llvm::Value *DJPlus::codeGen() {
  Value *L = lhs->codeGen();
  Value *R = rhs->codeGen();
  if (!L || !R) {
    return nullptr;
  }
  return Builder.CreateAdd(L, R, "addtmp");
}

llvm::Value *DJNat::codeGen() {
  return llvm::ConstantInt::get(TheContext, APInt(64, value));
}
