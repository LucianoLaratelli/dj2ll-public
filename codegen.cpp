#include "codegen.hpp"

#include "llast.hpp"
#include "llvm_includes.hpp"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace llvm;
extern std::string inputFile;

static std::unique_ptr<Module> TheModule;
Function *createFunc(IRBuilder<> &Builder, std::string Name) {
  FunctionType *funcType = FunctionType::get(Builder.getInt32Ty(), false);
  Function *fooFunc = Function::Create(funcType, Function::ExternalLinkage,
                                       Name, TheModule.get());
  return fooFunc;
}
BasicBlock *createBB(Function *fooFunc, std::string Name) {
  return BasicBlock::Create(TheContext, Name, fooFunc);
}

Function *DJProgram::codeGen() {
  TheModule = std::make_unique<Module>(inputFile, TheContext);
  Value *last = nullptr;
  // for (auto c : classes) {
  //   last = c->codeGen(context);
  // }
  // for (auto m : mainDecls) {
  //   last = m->codeGen(context);
  // }

  /*emit runtime function `printNat()`, which is really just the system printf*/
  std::vector<Type *> args;
  args.push_back(Type::getInt8PtrTy(TheContext));
  FunctionType *printfType =
      FunctionType::get(Builder.getInt32Ty(), args, true);
  Function::Create(printfType, Function::ExternalLinkage, "printf",
                   TheModule.get());
  /*begin codegen for `main`*/
  Function *main = createFunc(Builder, "main");
  BasicBlock *entry = createBB(main, "entry");
  Builder.SetInsertPoint(entry);
  for (auto e : mainExprs) {
    last = e->codeGen();
  }
  Builder.CreateRet(last);
  std::cout << "****************\n";
  TheModule->print(outs(), nullptr);
  std::cout << "****************\n";
  /*begin emitting object file -- copied mostly verbatim from the kaleidoscope
   * tutorial*/
  auto TargetTriple = sys::getDefaultTargetTriple();
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();
  std::string Error;
  auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!Target) {
    errs() << Error;
    exit(-1);
  }
  auto CPU = sys::getHostCPUName();
  std::string Features = "";
  StringMap<bool> HostFeatures;
  if (!sys::getHostCPUFeatures(HostFeatures)) {
    std::cerr << LRED "Could not determine host CPU features.\n";

  } else {
    SubtargetFeatures TheFeatures;
    for (auto i : HostFeatures.keys()) {
      if (HostFeatures[i]) {
        TheFeatures.AddFeature(i.str());
      }
    }
    Features = TheFeatures.getString();
  }

  TargetOptions opt;
  auto RM = Reloc::Model::DynamicNoPIC;
  auto TargetMachine =
      Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
  TheModule->setDataLayout(TargetMachine->createDataLayout());
  TheModule->setTargetTriple(TargetTriple);
  auto Filename = inputFile + ".o";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    exit(-1);
  }
  legacy::PassManager pass;
  auto FileType = CGFT_ObjectFile;

  if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    errs() << "TargetMachine can't emit a file of this type";
    exit(-1);
  }

  pass.run(*TheModule);
  dest.flush();
  return main;
}

Value *DJPlus::codeGen() {
  Value *L = lhs->codeGen();
  Value *R = rhs->codeGen();
  if (!L || !R) {
    return nullptr;
  }
  return Builder.CreateAdd(L, R, "addtmp");
}

Value *DJMinus::codeGen() {
  Value *L = lhs->codeGen();
  Value *R = rhs->codeGen();
  if (!L || !R) {
    return nullptr;
  }
  return Builder.CreateSub(L, R, "subtmp");
}

Value *DJTimes::codeGen() {
  Value *L = lhs->codeGen();
  Value *R = rhs->codeGen();
  if (!L || !R) {
    return nullptr;
  }
  return Builder.CreateMul(L, R, "multmp");
}

Value *DJPrint::codeGen() {
  Value *P = printee->codeGen();
  if (!P) {
    std::cout << LRED "codegen failure in DJPrint::codeGen()\n";
    return nullptr;
  }
  std::vector<Value *> ArgsV;
  Value *formatStr = Builder.CreateGlobalStringPtr("%d\n");
  ArgsV.push_back(formatStr);
  ArgsV.push_back(P);
  Function *TheFunction = TheModule->getFunction("printf");
  Builder.CreateCall(TheFunction, ArgsV);
  return P;
}

Value *DJNat::codeGen() {
  return ConstantInt::get(TheContext, APInt(32, value));
}

Value *DJNot::codeGen() {
  Value *N = negated->codeGen();
  if (!N) {
    std::cout << LRED "codegen failure in DJNot::codeGen()\n Child tree:";
    negated->print(0);
    exit(-1);
  }
  return Builder.CreateNot(N);
}

Value *DJEqual::codeGen() {
  Value *L = lhs->codeGen();
  Value *R = rhs->codeGen();
  if (!L || !R) {
    return nullptr;
  }
  return Builder.CreateICmpEQ(L, R);
}

Value *DJGreater::codeGen() {
  Value *L = lhs->codeGen();
  Value *R = rhs->codeGen();
  if (!L || !R) {
    return nullptr;
  }
  return Builder.CreateICmpUGT(L, R);
}

Value *DJAnd::codeGen() {
  Value *L = lhs->codeGen();
  Value *R = rhs->codeGen();
  if (!L || !R) {
    return nullptr;
  }
  return Builder.CreateAnd(L, R);
}

Value *DJTrue::codeGen() { return ConstantInt::get(TheContext, APInt(1, 1)); }

Value *DJFalse::codeGen() { return ConstantInt::get(TheContext, APInt(1, 0)); }

Value *DJIf::codeGen() {
  /*almost verbatim from LLVM kaleidescope tutorial; comments are not mine*/
  Value *condValue = cond->codeGen();
  if (!cond) {
    std::cerr << LRED "Failure in DJIf::codeGen() for condValue\n";
    exit(-1);
  }
  condValue = Builder.CreateICmpNE(condValue,
                                   ConstantInt::get(TheContext, APInt(1, 0)));
  Function *TheFunction = Builder.GetInsertBlock()->getParent();

  // Create blocks for the then and else cases.  Insert the 'then' block at the
  // end of the function.
  BasicBlock *ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
  BasicBlock *ElseBB = BasicBlock::Create(TheContext, "else");
  BasicBlock *MergeBB = BasicBlock::Create(TheContext, "ifcont");

  Builder.CreateCondBr(condValue, ThenBB, ElseBB);
  // Emit then value.
  Builder.SetInsertPoint(ThenBB);

  Value *ThenV;
  for (auto &e : thenBlock) {
    ThenV = e->codeGen();
  }

  Builder.CreateBr(MergeBB);
  // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
  ThenBB = Builder.GetInsertBlock();
  // Emit else block.
  TheFunction->getBasicBlockList().push_back(ElseBB);
  Builder.SetInsertPoint(ElseBB);

  Value *ElseV;
  for (auto &e : elseBlock) {
    ElseV = e->codeGen();
  }

  Builder.CreateBr(MergeBB);
  // codegen of 'Else' can change the current block, update ElseBB for the PHI.
  ElseBB = Builder.GetInsertBlock();
  // Emit merge block.
  TheFunction->getBasicBlockList().push_back(MergeBB);
  Builder.SetInsertPoint(MergeBB);
  PHINode *PN = Builder.CreatePHI(Type::getInt32Ty(TheContext), 2, "iftmp");

  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  return PN;
}
