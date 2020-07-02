/*
** codegen.cpp
**
** This file implements all of the codeGen methods for the expression nodes of
** the LLAST. It also handles codeGen for an entire DJProgram:
**     * setting up runtime functions, vtables, itables
**     * verifying the generated IR Module
**     * emitting object code to a target file whose name is the same as the
**       source file but with a .o extension
** In contrast to the LLVM Kaleidoscope tutorial, I have opted to not perform
** null checks on any of the expressions. This is a deliberate choice to keep
** the code cleaner; I don't believe any safety is sacrificed because all of the
** base cases for the recursion (DOT_IDs, IDs, INSTANCEOFs, READs, THISs, NEWs,
** NULLs, NAT_LITERALs, TRUE_LITERALs, and FALSE_LITERALs) depend on directly
** utilizing LLVM API calls or the symbol table. Errors in the symbol table are
** caught during type checking; I expect that a failure that would cause a null
** pointer to arise during code generation for one of the base cases would
** actually be caught by the LLVM API call itself.
*/

#include "codegen.hpp"

#include "llast.hpp"
#include "llvm_includes.hpp"
#include "util.h"
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

int getNonStaticClassFieldIndex(std::string desired, int classIndex) {
  auto ST = classesST[classIndex].varList;
  for (int i = 0; i < classesST[classIndex].numVars; i++) {
    if (std::string(ST[i].varName) == desired) {
      return i;
    }
  }
  return -1;
}

using namespace llvm;
extern std::string inputFile;

std::map<std::string, std::vector<llvm::Type *>> calculateClassStorageNeeds() {
  std::map<std::string, std::vector<llvm::Type *>> ret;
  for (int i = 1; i < numClasses; i++) {
    std::vector<llvm::Type *> members;
    for (int j = 0; j < classesST[i].numVars; j++) {
      switch (classesST[i].varList[j].type) {
      case BAD_TYPE:
      case NO_OBJECT:
      case ANY_OBJECT:
        std::cerr
            << "bad regular var encountered in calculateClassStorageNeeds\n";
        exit(-1);
      case TYPE_BOOL: {
        members.push_back(Type::getInt1Ty(TheContext));
        break;
      }
      case TYPE_NAT: {
        members.push_back(Type::getInt32Ty(TheContext));
        break;
      }
      default: { // all objects
        members.push_back(Type::getInt32PtrTy(TheContext));
        break;
      }
      }
    }
    ret[classesST[i].className] = members;
  }
  return ret;
}

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

std::map<std::string, llvm::StructType *> allocatedClasses;
std::map<std::string, std::vector<llvm::Type *>> classSizes;

Function *DJProgram::codeGen() {
  TheModule = std::make_unique<Module>(inputFile, TheContext);
  // Create a new pass manager attached to it.
  TheFPM = std::make_unique<legacy::FunctionPassManager>(TheModule.get());

  // Promote allocas to registers.
  TheFPM->add(createPromoteMemoryToRegisterPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->add(createInstructionCombiningPass());
  // Reassociate expressions.
  TheFPM->add(createReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->add(createCFGSimplificationPass());

  TheFPM->doInitialization();
  Value *last = nullptr;

  classSizes = calculateClassStorageNeeds();
  for (int i = 1; i < numClasses; i++) {
    allocatedClasses[classesST[i].className] =
        llvm::StructType::create(TheContext, classesST[i].className);
    allocatedClasses[classesST[i].className]->setBody(
        classSizes[classesST[i].className]);
  }
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
  /*emit runtime function `readNat()`, which is really just the system scanf*/
  Function::Create(printfType, Function::ExternalLinkage, "scanf",
                   TheModule.get());
  // std::vector<Type *> mallocArgs = {Type::getInt64Ty(TheContext)};
  // FunctionType *mallocType =
  //     FunctionType::get(Builder.getInt8Ty(), mallocArgs, false);
  // Function::Create(mallocType, Function::ExternalLinkage, "malloc",
  //                  TheModule.get());
  /*begin codegen for `main`*/
  Function *DJmain = createFunc(Builder, "main");
  BasicBlock *entry = createBB(DJmain, "entry");
  Builder.SetInsertPoint(entry);
  for (int i = 0; i < numMainBlockLocals; i++) {
    char *varName = mainBlockST[i].varName;
    switch (mainBlockST[i].type) {
    case TYPE_NAT:
      NamedValues[varName] =
          Builder.CreateAlloca(Type::getInt32Ty(TheContext), nullptr, varName);
      Builder.CreateStore(ConstantInt::get(TheContext, APInt(32, 0)),
                          NamedValues[varName]);
      break;
    case TYPE_BOOL:
      NamedValues[varName] =
          Builder.CreateAlloca(Type::getInt1Ty(TheContext), nullptr, varName);
      Builder.CreateStore(ConstantInt::get(TheContext, APInt(1, 0)),
                          NamedValues[varName]);
      break;
    default:
      char *varType = typeString(mainBlockST[i].type);
      NamedValues[varName] = Builder.CreateAlloca(
          PointerType::get(allocatedClasses[varType], 0), NamedValues[varName]);
      break;
    }
  }
  for (auto e : mainExprs) {
    last = e->codeGen();
  }
  // adjust main's return type if needed so we don't get a type mismatch when we
  // verify the module
  if (last->getType() != Type::getInt32Ty(TheContext)) {
    last = ConstantInt::get(TheContext, APInt(32, 0));
  }
  Builder.CreateRet(last); /*done with code gen*/
  // TheFPM->run(*DJmain);
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
  return DJmain;
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
  Value *formatStr = Builder.CreateGlobalStringPtr("%u\n");
  std::vector<Value *> PrintfArgs = {formatStr, P};
  Function *TheFunction = TheModule->getFunction("printf");
  Builder.CreateCall(TheFunction, PrintfArgs);
  return P;
}

Value *DJRead::codeGen() {
  Builder.GetInsertBlock()->getParent();
  Value *requestStr = Builder.CreateGlobalStringPtr("Enter a natural number: ");
  std::vector<Value *> requestArgs = {requestStr};
  Builder.CreateCall(TheModule->getFunction("printf"), requestArgs);
  Value *formatStr = Builder.CreateGlobalStringPtr("%u");
  AllocaInst *Alloca =
      Builder.CreateAlloca(Type::getInt32Ty(TheContext), nullptr, "temp");
  std::vector<Value *> scanfArgs = {formatStr, Alloca};
  Function *theScanf = TheModule->getFunction("scanf");
  Builder.CreateCall(theScanf, scanfArgs);
  return Builder.CreateLoad(Alloca);
}

Value *DJNat::codeGen() {
  return ConstantInt::get(TheContext, APInt(32, value));
}

Value *DJNot::codeGen() { return Builder.CreateNot(negated->codeGen()); }

Value *DJEqual::codeGen() {
  // TODO: may have to implement some hackery to get null working
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

  Value *ThenV = nullptr;
  for (auto &e : thenBlock) {
    ThenV = e->codeGen();
  }

  Builder.CreateBr(MergeBB);
  // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
  ThenBB = Builder.GetInsertBlock();
  // Emit else block.
  TheFunction->getBasicBlockList().push_back(ElseBB);
  Builder.SetInsertPoint(ElseBB);

  Value *ElseV = nullptr;
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

Value *DJFor::codeGen() {
  /*pretty similar to kaleidescope example; some modification to actually work
   * like a for loop should, unlike the one in the tutorial*/

  init->codeGen();

  // Make the new basic block for the loop header, inserting after current
  // block.
  Function *TheFunction = Builder.GetInsertBlock()->getParent();
  Builder.GetInsertBlock();
  BasicBlock *LoopBB = BasicBlock::Create(TheContext, "loop", TheFunction);

  // Insert an explicit fall through from the current block to the LoopBB.
  Builder.CreateBr(LoopBB);

  // Start insertion in LoopBB.
  Builder.SetInsertPoint(LoopBB);

  //    Compute the end condition.
  Value *testVal = test->codeGen();

  BasicBlock *BodyBB = BasicBlock::Create(TheContext, "loopbody", TheFunction);
  BasicBlock *AfterBB =
      BasicBlock::Create(TheContext, "afterloop", TheFunction);

  // Convert condition to a bool by comparing non-equal to 0.0.
  testVal = Builder.CreateICmpNE(
      testVal, ConstantInt::get(TheContext, APInt(1, 0)), "loopcond");

  // Insert the conditional branch
  Builder.CreateCondBr(testVal, BodyBB, AfterBB);

  // Emit the body of the loop.  This, like any other expr, can change the
  // current BB.  Note that we ignore the value computed by the body
  Builder.SetInsertPoint(BodyBB);
  for (auto &e : body) {
    e->codeGen();
  }

  update->codeGen();
  testVal = test->codeGen();
  // Convert condition to a bool by comparing non-equal to 0.0.
  testVal = Builder.CreateICmpNE(
      testVal, ConstantInt::get(TheContext, APInt(1, 0)), "loopcond");

  // insert the "after loop" block
  Builder.GetInsertBlock();

  // Insert the conditional branch into the end of LoopEndBB.
  Builder.CreateCondBr(testVal, BodyBB, AfterBB);

  // Any new code will be inserted in AfterBB.
  Builder.SetInsertPoint(AfterBB);

  // for expr always returns 0.
  return Constant::getNullValue(Type::getInt32Ty(TheContext));
}

Value *DJId::codeGen() { return Builder.CreateLoad(NamedValues[ID], ID); }

Value *DJAssign::codeGen() {
  return Builder.CreateStore(RHS->codeGen(), NamedValues[LHS]);
}

Value *DJNull::codeGen() {
  // TODO: doesn't work, something like null == C for some class C fails because
  // of null having a different type
  return ConstantPointerNull::get(Builder.getVoidTy()->getPointerTo());
}

Value *DJNew::codeGen() {
  auto typeSize = ConstantExpr::getSizeOf(allocatedClasses[assignee]);
  typeSize =
      ConstantExpr::getTruncOrBitCast(typeSize, Type::getInt64Ty(TheContext));

  auto I = CallInst::CreateMalloc(
      Builder.GetInsertBlock(), Type::getInt64Ty(TheContext),
      allocatedClasses[assignee], typeSize, nullptr, nullptr, "");
  return Builder.Insert(I);
}

Value *DJDotId::codeGen() {
  // TODO: implement for superclass vars
  // TODO: implement for static vars
  std::vector<Value *> elementIndex = {ConstantInt::get(
      TheContext, APInt(32, getNonStaticClassFieldIndex(ID, objectLikeType)))};
  auto I =
      GetElementPtrInst::Create(allocatedClasses[typeString(objectLikeType)],
                                objectLike->codeGen(), elementIndex);
  return Builder.Insert(I);
}
