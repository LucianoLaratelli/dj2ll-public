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
  Builder.CreateRet(last); /*done with code gen*/

  llvm::Module *test = TheModule.get();
  llvm::verifyModule(*test, &llvm::errs());
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
  return Builder.CreateAdd(lhs->codeGen(), rhs->codeGen(), "addtmp");
}

Value *DJMinus::codeGen() {
  return Builder.CreateSub(lhs->codeGen(), rhs->codeGen(), "subtmp");
}

Value *DJTimes::codeGen() {
  return Builder.CreateMul(lhs->codeGen(), rhs->codeGen(), "multmp");
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

Value *DJNot::codeGen() { return Builder.CreateNot(negated->codeGen()); }

Value *DJEqual::codeGen() {
  return Builder.CreateICmpEQ(lhs->codeGen(), rhs->codeGen());
}

Value *DJGreater::codeGen() {
  return Builder.CreateICmpUGT(lhs->codeGen(), rhs->codeGen());
}

Value *DJAnd::codeGen() {
  return Builder.CreateAnd(lhs->codeGen(), rhs->codeGen());
}

Value *DJTrue::codeGen() { return ConstantInt::get(TheContext, APInt(1, 1)); }

Value *DJFalse::codeGen() { return ConstantInt::get(TheContext, APInt(1, 0)); }

Value *DJIf::codeGen() {
  /*almost verbatim from LLVM kaleidescope tutorial; comments are not mine*/
  Value *condValue = cond->codeGen();
  if (!condValue) {
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

Value *DJFor::codeGen() // {
                        // Value *StartVal = init->codeGen();
                        // if (!StartVal)
                        //   return nullptr;

    // // Make the new basic block for the loop header, inserting after current
    // // block.
    // Function *TheFunction = Builder.GetInsertBlock()->getParent();
    // BasicBlock *PreheaderBB = Builder.GetInsertBlock();
    // BasicBlock *LoopBB = BasicBlock::Create(TheContext, "loop", TheFunction);

    // // Insert an explicit fall through from the current block to the LoopBB.
    // Builder.CreateBr(LoopBB);

    // // Start insertion in LoopBB.
    // Builder.SetInsertPoint(LoopBB);

    // // Start the PHI node with an entry for Start.
    // PHINode *Variable =
    //     Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, VarName);
    // Variable->addIncoming(StartVal, PreheaderBB);

    // // Within the loop, the variable is defined equal to the PHI node.  If it
    // // shadows an existing variable, we have to restore it, so save it now.
    // Value *OldVal = NamedValues[VarName];
    // NamedValues[VarName] = Variable;

    // // Emit the body of the loop.  This, like any other expr, can change the
    // // current BB.  Note that we ignore the value computed by the body, but
    // // don't allow an error.
    // if (!body->codeGen())
    //   return nullptr;

    // // Emit the step value.
    // Value *StepVal = nullptr;
    // if (Step) {
    //   StepVal = Step->codeGen();
    //   if (!StepVal)
    //     return nullptr;
    // } else {
    //   // If not specified, use 1.0.
    //   StepVal = ConstantFP::get(TheContext, APFloat(1.0));
    // }

    // Value *NextVar = Builder.CreateFAdd(Variable, StepVal, "nextvar");

    // // Compute the end condition.
    // Value *EndCond = End->codeGen();
    // if (!EndCond)
    //   return nullptr;

    // // Convert condition to a bool by comparing non-equal to 0.0.
    // EndCond = Builder.CreateFCmpONE(
    //     EndCond, ConstantFP::get(TheContext, APFloat(0.0)), "loopcond");

    // // Create the "after loop" block and insert it.
    // BasicBlock *LoopEndBB = Builder.GetInsertBlock();
    // BasicBlock *AfterBB =
    //     BasicBlock::Create(TheContext, "afterloop", TheFunction);

    // // Insert the conditional branch into the end of LoopEndBB.
    // Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

    // // Any new code will be inserted in AfterBB.
    // Builder.SetInsertPoint(AfterBB);

    // // Add a new entry to the PHI node for the backedge.
    // Variable->addIncoming(NextVar, LoopEndBB);

    // // Restore the unshadowed variable.
    // if (OldVal)
    //   NamedValues[VarName] = OldVal;
    // else
    //   NamedValues.erase(VarName);

    // // for expr always returns 0.0.
    return Constant::getNullValue(Type::getDoubleTy(TheContext));
}
