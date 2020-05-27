#include "codegen.hpp"

#include "llast.hpp"
#include "llvm_includes.hpp"
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

llvm::Function *DJProgram::codeGen() {
  llvm::Value *last = nullptr;
  // for (auto c : classes) {
  //   last = c->codeGen(context);
  // }
  // for (auto m : mainDecls) {
  //   last = m->codeGen(context);
  // }
  FunctionType *FT = FunctionType::get(Type::getVoidTy(TheContext), false);
  Function *TheFunction =
      Function::Create(FT, Function::ExternalLinkage, "main", TheModule.get());
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
  Builder.SetInsertPoint(BB);
  for (auto e : mainExprs) {
    last = e->codeGen();
  }
  Builder.CreateRet(last);
  // verifyFunction(*TheFunction);
  return TheFunction;
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
