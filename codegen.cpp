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
#include "codeGenClass.hpp"
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

using namespace llvm;
extern std::string inputFile;

static std::map<std::string, llvm::StructType *> allocatedClasses;
static std::map<std::string, std::vector<llvm::Type *>> classSizes;
static std::unique_ptr<llvm::Module> TheModule;

llvm::Function *createFunc(llvm::IRBuilder<> &Builder, std::string Name) {
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(Builder.getInt32Ty(), false);
  llvm::Function *fooFunc = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, Name, TheModule.get());
  return fooFunc;
}

llvm::BasicBlock *createBB(llvm::Function *fooFunc, std::string Name) {
  return llvm::BasicBlock::Create(TheContext, Name, fooFunc);
}

void emitITable() {
  // generate the instance-of jump table for all the classes in the program
  // basically a mess of chained if-then-else statements of this form:
  // {{let a = the first argument to ITable and b = the second argument}}
  // if(a == 0 and b == 0) return isSubtype(0,0)
  // if(a == 0 and b == 1) return isSubtype(0,1)
  // etc
  std::vector<Type *> ITableArgs = {Type::getInt32Ty(TheContext),
                                    Type::getInt32Ty(TheContext)};
  llvm::FunctionType *ITableType =
      llvm::FunctionType::get(Builder.getInt1Ty(), ITableArgs, false);
  llvm::Function *ITableFunc = llvm::Function::Create(
      ITableType, llvm::Function::ExternalLinkage, "ITable", TheModule.get());
  Builder.SetInsertPoint(createBB(ITableFunc, "entry"));
  auto aClass = ITableFunc->getArg(0);
  auto bClass = ITableFunc->getArg(1);

  for (int i = 0; i < numClasses; i++) {
    for (int j = 0; j < numClasses; j++) {
      auto condValue = Builder.CreateAnd(
          Builder.CreateICmpEQ(aClass,
                               ConstantInt::get(TheContext, APInt(32, i))),
          Builder.CreateICmpEQ(bClass,
                               ConstantInt::get(TheContext, APInt(32, j))));
      Function *TheFunction = Builder.GetInsertBlock()->getParent();

      BasicBlock *ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
      BasicBlock *ElseBB = BasicBlock::Create(TheContext, "else");

      Builder.CreateCondBr(condValue, ThenBB, ElseBB);
      // emit then value
      Builder.SetInsertPoint(ThenBB);

      Builder.CreateRet(
          ConstantInt::get(TheContext, APInt(1, isSubtype(i, j))));

      ThenBB = Builder.GetInsertBlock();
      TheFunction->getBasicBlockList().push_back(ElseBB);
      Builder.SetInsertPoint(ElseBB);
    }
  }
  Builder.CreateRet(ConstantInt::get(TheContext, APInt(1, 0)));
}

Function *DJProgram::codeGen(int type) {
  TheModule = std::make_unique<Module>(inputFile, TheContext);
  Value *last = nullptr;

  emitITable();
  for (int i = 0; i < numClasses; i++) {
    allocatedClasses[classesST[i].className] =
        llvm::StructType::create(TheContext, classesST[i].className);
  }
  classSizes = calculateClassStorageNeeds(allocatedClasses);
  for (int i = 0; i < numClasses; i++) {
    allocatedClasses[classesST[i].className]->setBody(
        classSizes[classesST[i].className]);
  }
  // emit static variable declarations
  // DJ treats static variables the way java does, as globals that are specific
  // to any object of that class, even if that object does not exist; (new
  // A()).a accesses the same object as allocatedA.a
  for (int i = 0; i < numClasses; i++) {
    for (int j = 0; j < classesST[i].numStaticVars; j++) {
      auto var = classesST[i].staticVarList[j];
      /*the class that declares this static variable*/
      auto declaredClass = std::string(classesST[i].className);
      auto name = declaredClass + "." + var.varName;
      switch (var.type) {
      case TYPE_NAT:
        GlobalValues[name] = new GlobalVariable(
            *TheModule.get(), Type::getInt32Ty(TheContext), false,
            llvm::GlobalValue::LinkageTypes::CommonLinkage, 0, name);
        GlobalValues[name]->setInitializer(
            ConstantInt::get(TheContext, APInt(32, 0)));
        break;
      case TYPE_BOOL:
        GlobalValues[name] = new GlobalVariable(
            *TheModule.get(), Type::getInt1Ty(TheContext), false,
            llvm::GlobalValue::LinkageTypes::CommonLinkage, 0, name);
        GlobalValues[name]->setInitializer(
            ConstantInt::get(TheContext, APInt(1, 0)));
        break;
      default:
        GlobalValues[name] = new GlobalVariable(
            *TheModule.get(),
            // allocatedClasses[declaredClass],
            PointerType::getUnqual(allocatedClasses[declaredClass]), false,
            GlobalValue::LinkageTypes::ExternalLinkage, 0, name);
      }
    }
  }
  // for (auto c : classes) {
  //   last = c->codeGen(context);
  // }
  // for (auto m : mainDecls) {
  //   last = m->codeGen(context);
  // }

  /*emit runtime function `printNat()`, which is really just the system
   * printf*/
  std::vector<Type *> args;
  args.push_back(Type::getInt8PtrTy(TheContext));
  FunctionType *printfType =
      FunctionType::get(Builder.getInt32Ty(), args, true);
  Function::Create(printfType, Function::ExternalLinkage, "printf",
                   TheModule.get());
  /*emit runtime function `readNat()`, which is really just the system scanf*/
  Function::Create(printfType, Function::ExternalLinkage, "scanf",
                   TheModule.get());
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
          PointerType::getUnqual(allocatedClasses[varType]),
          // PointerType::get(allocatedClasses[varType],0),
          NamedValues[varName]);
      break;
    }
  }
  for (auto e : mainExprs) {
    last = e->codeGen();
  }
  // adjust main's return type if needed so we don't get a type mismatch when
  // we verify the module
  if (last->getType() != Type::getInt32Ty(TheContext)) {
    last = ConstantInt::get(TheContext, APInt(32, 0));
  }
  Builder.CreateRet(last); /*done with code gen*/

  std::cout << "****************\n";
  TheModule->print(outs(), nullptr);
  std::cout << "****************\n";
  llvm::Module *test = TheModule.get();
  llvm::verifyModule(*test, &llvm::errs());
  if (runOptimizations) {
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
    TheFPM->run(*DJmain);
  }
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

Value *DJPlus::codeGen(int type) {
  return Builder.CreateAdd(lhs->codeGen(), rhs->codeGen(), "addtmp");
}

Value *DJMinus::codeGen(int type) {
  return Builder.CreateSub(lhs->codeGen(), rhs->codeGen(), "subtmp");
}

Value *DJTimes::codeGen(int type) {
  return Builder.CreateMul(lhs->codeGen(), rhs->codeGen(), "multmp");
}

Value *DJPrint::codeGen(int type) {
  Value *P = printee->codeGen();
  Value *formatStr = Builder.CreateGlobalStringPtr("%u\n");
  std::vector<Value *> PrintfArgs = {formatStr, P};
  Function *TheFunction = TheModule->getFunction("printf");
  Builder.CreateCall(TheFunction, PrintfArgs);
  return P;
}

Value *DJRead::codeGen(int type) {
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

Value *DJNat::codeGen(int type) {
  return ConstantInt::get(TheContext, APInt(32, value));
}

Value *DJNot::codeGen(int type) {
  return Builder.CreateNot(negated->codeGen());
}

Value *DJEqual::codeGen(int type) {
  if (bothNull || !hasNullChild) {
    return Builder.CreateICmpEQ(lhs->codeGen(), rhs->codeGen());
  }
  if (leftNull) {
    return Builder.CreateICmpEQ(lhs->codeGen(nonNullType), rhs->codeGen());
  }
  // right must be null, then
  return Builder.CreateICmpEQ(lhs->codeGen(), rhs->codeGen(nonNullType));
}

Value *DJGreater::codeGen(int type) {
  return Builder.CreateICmpUGT(lhs->codeGen(), rhs->codeGen());
}

Value *DJAnd::codeGen(int type) {
  return Builder.CreateAnd(lhs->codeGen(), rhs->codeGen());
}

Value *DJTrue::codeGen(int type) {
  return ConstantInt::get(TheContext, APInt(1, 1));
}

Value *DJFalse::codeGen(int type) {
  return ConstantInt::get(TheContext, APInt(1, 0));
}

Value *DJIf::codeGen(int type) {
  /*almost verbatim from LLVM kaleidescope tutorial; comments are not mine*/
  Value *condValue = cond->codeGen();
  condValue = Builder.CreateICmpNE(condValue,
                                   ConstantInt::get(TheContext, APInt(1, 0)));
  Function *TheFunction = Builder.GetInsertBlock()->getParent();

  // Create blocks for the then and else cases.  Insert the 'then' block at
  // the end of the function.
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
  // Codegen of 'Then' can change the current block, update ThenBB for the
  // PHI.
  ThenBB = Builder.GetInsertBlock();
  // Emit else block.
  TheFunction->getBasicBlockList().push_back(ElseBB);
  Builder.SetInsertPoint(ElseBB);

  Value *ElseV = nullptr;
  for (auto &e : elseBlock) {
    ElseV = e->codeGen();
  }

  Builder.CreateBr(MergeBB);
  // codegen of 'Else' can change the current block, update ElseBB for the
  // PHI.
  ElseBB = Builder.GetInsertBlock();
  // Emit merge block.
  TheFunction->getBasicBlockList().push_back(MergeBB);
  Builder.SetInsertPoint(MergeBB);
  PHINode *PN = Builder.CreatePHI(Type::getInt32Ty(TheContext), 2, "iftmp");

  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  return PN;
}

Value *DJFor::codeGen(int type) {
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

Value *DJId::codeGen(int type) {
  return Builder.CreateLoad(NamedValues[ID], ID);
}

Value *DJAssign::codeGen(int type) {
  if (hasNullChild) {
    return Builder.CreateStore(RHS->codeGen(LHSType), NamedValues[LHS]);
  }
  return Builder.CreateStore(RHS->codeGen(), NamedValues[LHS]);
}

Value *DJNull::codeGen(int type) {
  if (type == -1) {
    // case where null is not compared or assigned to a variable of object
    // type, we can simply return a null int. this also covers the case where
    // null is compared to null, etc
    return Constant::getNullValue(Type::getInt32Ty(TheContext));
  }
  return ConstantPointerNull::get(
      PointerType::getUnqual(allocatedClasses[typeString(type)]));
}

int getClassID(std::string name) {
  for (int i = 0; i < numClasses; i++) {
    if (classesST[i].className == name) {
      return i;
    }
  }
  return -1;
}

Value *DJNew::codeGen(int type) {
  /* allocate a DJ class using system malloc, setting the `this` pointer and the
   * class ID */
  auto typeSize = ConstantExpr::getSizeOf(allocatedClasses[assignee]);
  typeSize =
      ConstantExpr::getTruncOrBitCast(typeSize, Type::getInt64Ty(TheContext));

  auto I = CallInst::CreateMalloc(
      Builder.GetInsertBlock(), Type::getInt64Ty(TheContext),
      allocatedClasses[assignee], typeSize, nullptr, nullptr, "");
  NamedValues["temp"] = Builder.CreateAlloca(
      PointerType::getUnqual(allocatedClasses[assignee]), nullptr, "temp");
  Builder.CreateStore(Builder.Insert(I), NamedValues["temp"]);

  std::vector<Value *> elementIndex = {
      ConstantInt::get(TheContext, APInt(32, 0)),
      ConstantInt::get(TheContext, APInt(32, 0))};
  // get pointer to zeroth element in the struct, which is the `this` pointer
  auto thisPtr = GetElementPtrInst::Create(
      allocatedClasses[assignee], Builder.CreateLoad(NamedValues["temp"]),
      elementIndex);
  // store the result of malloc as `this`
  Builder.CreateStore(Builder.CreateLoad(NamedValues["temp"]),
                      Builder.Insert(thisPtr));
  // get pointer to the 1th element in the struct, which holds its class ID
  elementIndex[1] = ConstantInt::get(TheContext, APInt(32, 1));
  auto classID = GetElementPtrInst::Create(
      allocatedClasses[assignee], Builder.CreateLoad(NamedValues["temp"]),
      elementIndex);
  Builder.CreateStore(ConstantInt::get(TheContext, APInt(32, this->classID)),
                      Builder.Insert(classID));

  return I;
}

Value *DJDotId::codeGen(int type) {
  auto varInfo = varIsStaticInAnySuperClass(ID, objectLikeType);
  if (varInfo.first) {
    // because of subtyping, the program may be talking about A.b (where A
    // extends B) and b is actually a static field of class B. varIsStatic...
    // checks the entire superclass hierarchy of the class called in the program
    // (in this example, class A) to determine which class actually declared the
    // static variable
    auto actualID = varInfo.second + "." + ID;
    return Builder.CreateLoad(GlobalValues[actualID]);
  } else {
    std::vector<Value *> elementIndex = {
        ConstantInt::get(TheContext, APInt(32, 0)),
        ConstantInt::get(
            TheContext,
            /*add 1 to offset from the `this` pointer*/
            APInt(32, getIndexOfRegularOrInheritedField(ID, objectLikeType)))};
    auto I =
        GetElementPtrInst::Create(allocatedClasses[typeString(objectLikeType)],
                                  objectLike->codeGen(), elementIndex);
    Builder.Insert(I);
    return Builder.CreateLoad(I);
  }
}

Value *DJDotAssign::codeGen(int type) {
  auto varInfo = varIsStaticInAnySuperClass(ID, objectLikeType);
  auto ret = assignVal->codeGen();
  if (varInfo.first) {
    // because of subtyping, the program may be talking about A.b (where A
    // extends B) and b is actually a static field of class B. varIsStatic...
    // checks the entire superclass hierarchy of the class called in the program
    // (in this example, class A) to determine which class actually declared the
    // static variable
    auto actualID = varInfo.second + "." + ID;
    Builder.CreateStore(ret, GlobalValues[actualID]);
  } else {
    std::vector<Value *> elementIndex = {
        ConstantInt::get(TheContext, APInt(32, 0)),
        ConstantInt::get(
            TheContext,
            APInt(32, getIndexOfRegularOrInheritedField(ID, objectLikeType)))};
    if (hasNullChild) {
      auto I = GetElementPtrInst::Create(
          allocatedClasses[typeString(objectLikeType)], objectLike->codeGen(),
          elementIndex);
      Builder.Insert(I);
      auto ret = assignVal->codeGen(objectLikeType);
      Builder.CreateStore(ret, I);
      return ret;
    }
    auto I =
        GetElementPtrInst::Create(allocatedClasses[typeString(objectLikeType)],
                                  objectLike->codeGen(), elementIndex);
    Builder.Insert(I);
    Builder.CreateStore(ret, I);
  }
  return ret;
}

Value *DJInstanceOf::codeGen(int type) {
  Value *testee = objectLike->codeGen();
  std::vector<Value *> elementIndex = {
      ConstantInt::get(TheContext, APInt(32, 0)),
      ConstantInt::get(TheContext, APInt(32, 1))};
  auto I = GetElementPtrInst::Create(
      allocatedClasses[typeString(objectLikeType)], testee, elementIndex);
  Builder.Insert(I);
  Builder.CreateLoad(I);
  std::vector<Value *> ITableArgs = {
      Builder.CreateLoad(I), ConstantInt::get(TheContext, APInt(32, classID))};
  Function *TheFunction = TheModule->getFunction("ITable");
  return Builder.CreateCall(TheFunction, ITableArgs);
}
