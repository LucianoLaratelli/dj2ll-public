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
#include "translateAST.hpp"
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

Type *getLLVMTypeFromDJType(int djType) {
  switch (djType) {
  case TYPE_BOOL: {
    return Type::getInt1Ty(TheContext);
  }
  case TYPE_NAT: {
    return Type::getInt32Ty(TheContext);
  }
  default: {
    return PointerType::getUnqual(allocatedClasses[typeString(djType)]);
  }
  }
}

Type *getLLVMTypeFromDJType(std::string djType) {
  if (djType == "bool") {
    return Type::getInt1Ty(TheContext);
  } else if (djType == "nat") {
    return Type::getInt32Ty(TheContext);
  } else {
    return PointerType::getUnqual(allocatedClasses[djType]);
  }
}

std::vector<Value *> getGEPIndex(std::string variable, int classID) {
  std::vector<Value *> ret = {
      ConstantInt::get(TheContext, APInt(32, 0)),
      ConstantInt::get(TheContext, APInt(32, getIndexOfRegularOrInheritedField(
                                                 variable, classID)))};
  return ret;
}

std::vector<Value *> getThisIndex() {
  std::vector<Value *> ret = {ConstantInt::get(TheContext, APInt(32, 0)),
                              ConstantInt::get(TheContext, APInt(32, 0))};
  return ret;
}

std::vector<Type *> calculateInheritedStorageNeeds(
    int classNum, std::map<std::string, llvm::StructType *> &allocatedClasses) {
  // given a class ID, iterate inclusively from that class through all its
  // superclasses, adding LLVM types to its declaration
  int count = 0;
  std::vector<Type *> members;
  while (count < numClasses && classNum != 0 && classNum != -4) {
    for (int j = 0; j < classesST[classNum].numVars; j++) {
      switch (classesST[classNum].varList[j].type) {
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
        members.push_back(PointerType::getUnqual(
            allocatedClasses[typeString(classesST[classNum].varList[j].type)]));
        break;
      }
      }
    }
    classNum = classesST[classNum].superclass;
  }
  return members;
}

std::map<std::string, std::vector<llvm::Type *>> calculateClassStorageNeeds(
    std::map<std::string, llvm::StructType *> &allocatedClasses) {
  // determine the storage needs of every class declared by the program
  std::map<std::string, std::vector<llvm::Type *>> ret;
  std::vector<llvm::Type *> members;
  symbolTable genericST;
  for (int i = 0; i < numClasses; i++) {
    /*allocate `this` pointer*/
    members.push_back(
        PointerType::getUnqual(allocatedClasses[classesST[i].className]));
    /*make space for class ID*/
    members.push_back(Type::getInt32Ty(TheContext));
    for (int j = 0; j < classesST[i].numVars; j++) {
      genericST[classesST[i].varList[j].varName] = nullptr;
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
        members.push_back(PointerType::getUnqual(
            allocatedClasses[typeString(classesST[i].varList[j].type)]));
        break;
      }
      }
    }
    auto inherited = calculateInheritedStorageNeeds(classesST[i].superclass,
                                                    allocatedClasses);
    members.insert(members.end(), inherited.begin(), inherited.end());
    ret[classesST[i].className] = members;
    members.clear();
    NamedValues[typeString(i)] = genericST;
    genericST.clear();
  }
  return ret;
}

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

typedef int classID;
typedef int methodNum;
std::map<std::string,
         std::map<std::string,
                  std::vector<std::tuple<std::string, classID, methodNum>>>>
    VTable;
// std::map<std::string, std::map<std::string, std::vector<std::string>>>
// VTable;
std::vector<std::string> availableVTables;

void emitVTable() {
  // generate the virtual method call table. this method generates nine
  // different functions, which act as the jump tables.

  // nat natVTablenat();
  // nat natVTablebool();
  // nat natVTableObject();
  // bool boolVTablenat();
  // bool boolVTablebool();
  // bool boolVTableObject();
  // Object ObjectVTablenat();
  // Object ObjectVTablebool();
  // Object ObjectVTableObject();

  //  These will be called selectively from undot and dot method call. I opted
  //  to implement the VTable in this manner because of the LLVM IR type system,
  //  which will give many warnings for return type mismatch, parameter
  //  mismatch, etc. This made something like the ITable undesirable, because we
  //  would have methods that return nat, Object, and bool all as the return
  //  value of some master VTable function. You might ask how I'm gettng away
  //  with having all Object functions / parameters. well, I'm just bitcasting
  //  everywhere. I tested that we can freely bitcast a class to and from Object
  //  and still access fields, call methods with the same pointer, etc.

  // declare VTable data structures that will be used by this function and
  // codegen methods of dot/undot method call expressions.
  std::map<std::string,
           std::vector<std::tuple<std::string, classID, methodNum>>>
      thisOne;
  std::vector<std::tuple<std::string, classID, methodNum>> empty;
  for (auto i : {"nat", "bool", "Object"}) {
    for (auto j : {"nat", "bool", "Object"}) {
      thisOne[j] = empty;
      availableVTables.push_back(i + std::string("VTable") + j);
    }
    VTable[i] = thisOne;
    thisOne.clear();
  }

  for (int i = 0; i < numClasses; i++) {
    auto thisClass = classesST[i];
    std::string className = thisClass.className;
    for (int j = 0; j < thisClass.numMethods; j++) {
      auto method = thisClass.methodList[j];
      std::string methodName = method.methodName;
      std::string retTypeStr = typeString(method.returnType);
      std::string paramTyStr = typeString(method.paramType);
      if (method.returnType > OBJECT_TYPE) {
        retTypeStr = "Object";
      }
      if (method.paramType > OBJECT_TYPE) {
        paramTyStr = "Object";
      }
      auto LLMethodName = className + "_method_" + methodName;
      VTable[retTypeStr][paramTyStr].push_back(
          std::make_tuple(LLMethodName, i, j));
    }
  }

  // at this point we have a mapping of return type to parameter type to the
  // method name as is stored in NamedValues. Now we need to generate the nine
  // VTable functions as above.
  std::vector<Type *> functionArgs;
  llvm::FunctionType *methodType;
  for (const auto &[returnType, params] : VTable) {
    for (const auto &[paramType, methods] : params) {
      functionArgs = {PointerType::getUnqual(allocatedClasses["Object"]),
                      getLLVMTypeFromDJType(paramType)};
      methodType = FunctionType::get(getLLVMTypeFromDJType(returnType),
                                     functionArgs, false);
      Builder.SetInsertPoint(createBB(
          Function::Create(methodType, llvm::Function::ExternalLinkage,
                           returnType + "VTable" + paramType, TheModule.get()),
          "entry"));
      // TODO: get `this` set up as in further method declarations

      // TODO: check for value of ID

      // TODO: need to store which methods can be virtually called, maybe using
      // getDynamicMethodInfo from the old DISM codegen routines. those stored
      // methods will be referenced here

      // TODO: maybe need to pass in static method number to vtable functions?
      for (const auto &m : methods) {
        // TODO: once we have the data structure that stores which methods can
        // come from which static class, we need to call them here in a chain of
        // if-statements as in the ITable I guess we could use a tuple of
        // LLMethodName, staticClassID, and staticMethodID to determine this?
      }
    }
  }
}

void generateMethodST(int classNum, int methodNum, int inheritedFrom = 0) {
  // generate symbol tables of LLVM types from the old symbol tables generated
  // in symbtbl.c for the requested method.

  // methods are stored as "<declaring_class>_method_<method_name>" because a
  // class and its superclass can declare methods with the same name. you may
  // also notice that the global NamedValues symbol table allows for collisions
  // between class and method names; the naming scheme designed above prevents
  // this
  std::map<std::string, llvm::AllocaInst *> genericSymbolTable;
  auto classDecl = classesST[classNum];
  auto className = std::string(classDecl.className);
  MethodDecl method;
  if (inheritedFrom == 0) {
    method = classDecl.methodList[methodNum];
  } else {
    method = classesST[inheritedFrom].methodList[methodNum];
  }
  auto methodName = className + "_method_" + method.methodName;
  Function *LLMethod = TheModule->getFunction(methodName);
  genericSymbolTable["this"] =
      Builder.CreateAlloca(LLMethod->getArg(0)->getType(), nullptr, "this");
  Builder.CreateStore(LLMethod->getArg(0), genericSymbolTable["this"]);
  // set parameter value to whatever is passed in
  genericSymbolTable[method.paramName] = Builder.CreateAlloca(
      LLMethod->getArg(1)->getType(), nullptr, method.paramName);
  Builder.CreateStore(LLMethod->getArg(1),
                      genericSymbolTable[method.paramName]);
  for (int k = 0; k < method.numLocals; k++) {
    auto var = method.localST[k];
    auto name = var.varName;
    switch (var.type) {
    case TYPE_NAT:
      genericSymbolTable[name] =
          Builder.CreateAlloca(Type::getInt32Ty(TheContext), nullptr, name);
      Builder.CreateStore(ConstantInt::get(TheContext, APInt(32, 0)),
                          genericSymbolTable[name]);
      break;
    case TYPE_BOOL:
      genericSymbolTable[name] =
          Builder.CreateAlloca(Type::getInt1Ty(TheContext), nullptr, name);
      Builder.CreateStore(ConstantInt::get(TheContext, APInt(1, 0)),
                          genericSymbolTable[name]);
      break;
    default:
      char *varTypeString = typeString(var.type);
      genericSymbolTable[name] = Builder.CreateAlloca(
          PointerType::getUnqual(allocatedClasses[varTypeString]),
          genericSymbolTable[name]);
      break;
    }
  }
  NamedValues[methodName] = genericSymbolTable;
}

Function *DJProgram::codeGen(symbolTable ST, int type) {
  emitVTable();
  TheModule = std::make_unique<Module>(inputFile, TheContext);

  if (hasPrintNat || hasReadNat) {
    std::vector<Type *> args;
    args.push_back(Type::getInt8PtrTy(TheContext));
    FunctionType *IOType = FunctionType::get(Builder.getInt32Ty(), args, true);
    if (hasPrintNat) {
      // emit runtime function `printNat()`, which is just system printf
      Function::Create(IOType, Function::ExternalLinkage, "printf",
                       TheModule.get());
    }
    if (hasReadNat) {
      // emit runtime function `readNat()`, which is just system scanf
      Function::Create(IOType, Function::ExternalLinkage, "scanf",
                       TheModule.get());
    }
  }
  for (int i = 0; i < numClasses; i++) {
    allocatedClasses[classesST[i].className] =
        llvm::StructType::create(TheContext, classesST[i].className);
  }
  classSizes = calculateClassStorageNeeds(allocatedClasses);
  for (int i = 0; i < numClasses; i++) {
    allocatedClasses[classesST[i].className]->setBody(
        classSizes[classesST[i].className]);
  }
  if (hasInstanceOf) {
    emitITable();
  }
  for (int i = 0; i < numClasses; i++) {
    // emit static variable declarations. DJ treats static variables the way
    // java does, as globals that are specific to any object of that class, even
    // if that object does not exist; (new A()).a accesses the same object as
    // allocatedA.a)
    auto declaredClass = std::string(classesST[i].className);
    for (int j = 0; j < classesST[i].numStaticVars; j++) {
      auto var = classesST[i].staticVarList[j];
      /*the class that declares this static variable*/
      auto name = declaredClass + "." + var.varName;
      switch (var.type) {
      case TYPE_NAT:
        GlobalValues[name] = new GlobalVariable(
            *TheModule.get(), Type::getInt32Ty(TheContext), false,
            llvm::GlobalValue::LinkageTypes::CommonLinkage, nullptr, name);
        GlobalValues[name]->setInitializer(
            ConstantInt::get(TheContext, APInt(32, 0)));
        break;
      case TYPE_BOOL:
        GlobalValues[name] = new GlobalVariable(
            *TheModule.get(), Type::getInt1Ty(TheContext), false,
            llvm::GlobalValue::LinkageTypes::CommonLinkage, nullptr, name);
        GlobalValues[name]->setInitializer(
            ConstantInt::get(TheContext, APInt(1, 0)));
        break;
      default:
        GlobalValues[name] = new GlobalVariable(
            *TheModule.get(),
            PointerType::getUnqual(allocatedClasses[declaredClass]), false,
            GlobalValue::LinkageTypes::ExternalLinkage, nullptr, name);
      }
    }
  }
  for (int i = 0; i < numClasses; i++) {
    // emit method declarations
    std::vector<Type *> functionArgs;
    llvm::FunctionType *methodType;
    auto declaredClass = std::string(classesST[i].className);
    for (int j = 0; j < classesST[i].numMethods; j++) {
      auto methodST = classesST[i].methodList[j];
      auto methodName = declaredClass + "_method_" + methodST.methodName;
      functionArgs = {PointerType::getUnqual(allocatedClasses[declaredClass]),
                      getLLVMTypeFromDJType(methodST.paramType)};
      methodType = FunctionType::get(getLLVMTypeFromDJType(methodST.returnType),
                                     functionArgs, false);
      Function::Create(methodType, llvm::Function::ExternalLinkage, methodName,
                       TheModule.get());
    }
    // declare methods inherited from super classes, unless the current class
    // overrides them. we have to declare all methods first because any method
    // is free to call any other method; if we codegen the body of a method
    // definition before all class declarations have been created, we'll seg
    // fault when we go get the function from the module.
    auto superClass = classesST[i].superclass;
    while (superClass != NO_OBJECT) {
      for (int j = 0; j < classesST[superClass].numMethods; j++) {
        auto methodST = classesST[superClass].methodList[j];
        auto methodName = declaredClass + "_method_" + methodST.methodName;
        if (TheModule->getFunction(methodName) == nullptr) {
          functionArgs = {
              PointerType::getUnqual(allocatedClasses[declaredClass]),
              getLLVMTypeFromDJType(methodST.paramType)};
          methodType = FunctionType::get(
              getLLVMTypeFromDJType(methodST.returnType), functionArgs, false);
          Function::Create(methodType, llvm::Function::ExternalLinkage,
                           methodName, TheModule.get());
        }
      }
      superClass = classesST[superClass].superclass;
    }
  }
  // define methods declared in this class
  for (int i = 0; i < numClasses; i++) {
    auto declaredClass = std::string(classesST[i].className);
    for (int j = 0; j < classesST[i].numMethods; j++) {
      auto methodST = classesST[i].methodList[j];
      auto methodName = declaredClass + "_method_" + methodST.methodName;
      auto method = TheModule->getFunction(methodName);
      Builder.SetInsertPoint(createBB(method, "entry"));
      generateMethodST(i, j);
      Value *last = nullptr;
      for (const auto &e : translateExprList(methodST.bodyExprs)) {
        last = e->codeGen(NamedValues[methodName]);
      }
      if (methodST.returnType >= OBJECT_TYPE) {
        Builder.CreateRet(Builder.CreatePointerCast(
            last, getLLVMTypeFromDJType(methodST.returnType)));
      } else {
        Builder.CreateRet(last);
      }
    }
    auto superClass = classesST[i].superclass;
    while (superClass != NO_OBJECT) {
      // define methods we inherited from superclasses
      for (int j = 0; j < classesST[superClass].numMethods; j++) {
        auto methodST = classesST[superClass].methodList[j];
        auto methodName = declaredClass + "_method_" + methodST.methodName;
        auto method = TheModule->getFunction(methodName);
        if (method == nullptr) {
          Builder.SetInsertPoint(createBB(method, "entry"));
          generateMethodST(i, j, superClass);
          Value *last = nullptr;
          for (const auto &e : translateExprList(methodST.bodyExprs)) {
            last = e->codeGen(NamedValues[methodName]);
          }
          if (methodST.returnType >= OBJECT_TYPE) {
            Builder.CreateRet(Builder.CreatePointerCast(
                last, getLLVMTypeFromDJType(methodST.returnType)));
          } else {
            Builder.CreateRet(last);
          }
        }
      }
      superClass = classesST[superClass].superclass;
    }
  }
  /*begin codegen for `main`*/
  Function *DJmain = createFunc(Builder, "main");
  BasicBlock *entry = createBB(DJmain, "entry");

  std::map<std::string, llvm::AllocaInst *> MainSymbolTable;
  Builder.SetInsertPoint(entry);
  for (int i = 0; i < numMainBlockLocals; i++) {
    char *varName = mainBlockST[i].varName;
    switch (mainBlockST[i].type) {
    case TYPE_NAT:
      MainSymbolTable[varName] =
          Builder.CreateAlloca(Type::getInt32Ty(TheContext), nullptr, varName);
      Builder.CreateStore(ConstantInt::get(TheContext, APInt(32, 0)),
                          MainSymbolTable[varName]);
      break;
    case TYPE_BOOL:
      MainSymbolTable[varName] =
          Builder.CreateAlloca(Type::getInt1Ty(TheContext), nullptr, varName);
      Builder.CreateStore(ConstantInt::get(TheContext, APInt(1, 0)),
                          MainSymbolTable[varName]);
      break;
    default:
      char *varType = typeString(mainBlockST[i].type);
      MainSymbolTable[varName] = Builder.CreateAlloca(
          PointerType::getUnqual(allocatedClasses[varType]),
          MainSymbolTable[varName]);
      break;
    }
  }
  NamedValues["main"] = MainSymbolTable;
  Value *last = nullptr;
  for (auto e : mainExprs) {
    last = e->codeGen(NamedValues["main"]);
  }
  // adjust main's return type if needed so we don't get a type mismatch when
  // we verify the module
  if (last->getType() != Type::getInt32Ty(TheContext)) {
    last = ConstantInt::get(TheContext, APInt(32, 0));
  }
  Builder.CreateRet(last); /*done with code gen*/
  if (emitLLVM) {
    std::cout << "\n\n";
    TheModule->print(outs(), nullptr);
  }
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

Value *DJPlus::codeGen(symbolTable ST, int type) {
  return Builder.CreateAdd(lhs->codeGen(ST), rhs->codeGen(ST), "addtmp");
}

Value *DJMinus::codeGen(symbolTable ST, int type) {
  return Builder.CreateSub(lhs->codeGen(ST), rhs->codeGen(ST), "subtmp");
}

Value *DJTimes::codeGen(symbolTable ST, int type) {
  return Builder.CreateMul(lhs->codeGen(ST), rhs->codeGen(ST), "multmp");
}

Value *DJPrint::codeGen(symbolTable ST, int type) {
  Value *P = printee->codeGen(ST);
  Value *formatStr = Builder.CreateGlobalStringPtr("%u\n");
  std::vector<Value *> PrintfArgs = {formatStr, P};
  Function *TheFunction = TheModule->getFunction("printf");
  Builder.CreateCall(TheFunction, PrintfArgs);
  return P;
}

Value *DJRead::codeGen(symbolTable ST, int type) {
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

Value *DJNat::codeGen(symbolTable ST, int type) {
  return ConstantInt::get(TheContext, APInt(32, value));
}

Value *DJNot::codeGen(symbolTable ST, int type) {
  return Builder.CreateNot(negated->codeGen(ST));
}

Value *DJEqual::codeGen(symbolTable ST, int type) {
  if (bothNull || !hasNullChild) {
    return Builder.CreateICmpEQ(lhs->codeGen(ST), rhs->codeGen(ST));
  }
  if (leftNull) {
    return Builder.CreateICmpEQ(lhs->codeGen(ST, nonNullType),
                                rhs->codeGen(ST));
  }
  // right must be null, then
  return Builder.CreateICmpEQ(lhs->codeGen(ST), rhs->codeGen(ST, nonNullType));
}

Value *DJGreater::codeGen(symbolTable ST, int type) {
  return Builder.CreateICmpUGT(lhs->codeGen(ST), rhs->codeGen(ST));
}

Value *DJAnd::codeGen(symbolTable ST, int type) {
  return Builder.CreateAnd(lhs->codeGen(ST), rhs->codeGen(ST));
}

Value *DJTrue::codeGen(symbolTable ST, int type) {
  return ConstantInt::get(TheContext, APInt(1, 1));
}

Value *DJFalse::codeGen(symbolTable ST, int type) {
  return ConstantInt::get(TheContext, APInt(1, 0));
}

Value *DJIf::codeGen(symbolTable ST, int type) {
  /*almost verbatim from LLVM kaleidescope tutorial; comments are not mine*/
  Value *condValue = cond->codeGen(ST);
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
    ThenV = e->codeGen(ST);
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
    ElseV = e->codeGen(ST);
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

Value *DJFor::codeGen(symbolTable ST, int type) {
  /*pretty similar to kaleidescope example; some modification to actually work
   * like a for loop should, unlike the one in the tutorial*/

  init->codeGen(ST);

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
  Value *testVal = test->codeGen(ST);

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
    e->codeGen(ST);
  }

  update->codeGen(ST);
  testVal = test->codeGen(ST);
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

Value *DJId::codeGen(symbolTable ST, int type) {
  // TODO:staticClassNum is wrong here
  Value *valToLoad = nullptr;
  if (ST.find(ID) == ST.end()) {
    // not found inthe local ST, so it must be global or a class variable.
    auto varInfo = varIsStaticInAnySuperClass(ID, staticClassNum);
    if (varInfo.first) { // global variable.
      auto actualID = varInfo.second + "." + ID;
      valToLoad = GlobalValues[actualID];
    } else {
      // if the variable isn't in the symbol table and it isn't a global
      // variable, it must be a class variable. we use `this` to get at it.
      auto IDIndex = getGEPIndex(ID, staticClassNum);
      valToLoad = Builder.CreateGEP(Builder.CreateLoad(ST["this"]), IDIndex);
    }
  } else {
    valToLoad = ST[ID];
  }
  return Builder.CreateLoad(valToLoad, ID);
}

Value *DJAssign::codeGen(symbolTable ST, int type) {
  Value *V = nullptr;
  if (hasNullChild) {
    V = RHS->codeGen(ST, LHSType);
  } else {
    V = RHS->codeGen(ST);
    if (!V->getType()->isIntegerTy()) {
      V = Builder.CreatePointerCast(V, getLLVMTypeFromDJType(LHSType));
    }
    if (ST.find(LHS) == ST.end()) { // var is a class variable or static
      auto varInfo = varIsStaticInAnySuperClass(LHS, staticClassNum);
      if (varInfo.first) { // static var AKA global
        auto actualID = varInfo.second + "." + LHS;
        Builder.CreateStore(V, GlobalValues[actualID]);
        return V;
      }
      if (staticClassNum > 0) {
        // we are in a method and the requested variable is not a static
        // variable nor is in in the method-local symbol table; this means it
        // must be a class variable so we look at `this`
        auto IDIndex = getGEPIndex(LHS, staticClassNum);
        Builder.CreateStore(
            V, Builder.CreateGEP(Builder.CreateLoad(ST["this"]), IDIndex));
        return V;
      }
    }
  }
  Builder.CreateStore(V, ST[LHS]);
  return V;
}

Value *DJNull::codeGen(symbolTable ST, int type) {
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

Value *DJNew::codeGen(symbolTable ST, int type) {
  /* allocate a DJ class using system malloc, setting the `this` pointer and
   * the class ID */
  auto typeSize = ConstantExpr::getSizeOf(allocatedClasses[assignee]);
  typeSize =
      ConstantExpr::getTruncOrBitCast(typeSize, Type::getInt64Ty(TheContext));

  auto I = CallInst::CreateMalloc(
      Builder.GetInsertBlock(), Type::getInt64Ty(TheContext),
      allocatedClasses[assignee], typeSize, nullptr, nullptr, "");
  ST["temp"] = Builder.CreateAlloca(
      PointerType::getUnqual(allocatedClasses[assignee]), nullptr, "temp");
  Builder.CreateStore(Builder.Insert(I), ST["temp"]);

  std::vector<Value *> elementIndex = {
      ConstantInt::get(TheContext, APInt(32, 0)),
      ConstantInt::get(TheContext, APInt(32, 0))};
  // get pointer to zeroth element in the struct, which is the `this` pointer
  auto thisPtr = GetElementPtrInst::Create(
      allocatedClasses[assignee], Builder.CreateLoad(ST["temp"]), elementIndex);
  // store the result of malloc as `this`
  Builder.CreateStore(Builder.CreateLoad(ST["temp"]), Builder.Insert(thisPtr));
  // get pointer to the 1th element in the struct, which holds its class ID
  elementIndex[1] = ConstantInt::get(TheContext, APInt(32, 1));
  auto classID = GetElementPtrInst::Create(
      allocatedClasses[assignee], Builder.CreateLoad(ST["temp"]), elementIndex);
  Builder.CreateStore(ConstantInt::get(TheContext, APInt(32, this->classID)),
                      Builder.Insert(classID));

  return I;
}

Value *DJDotId::codeGen(symbolTable ST, int type) {
  auto varInfo = varIsStaticInAnySuperClass(ID, staticClassNum);
  if (varInfo.first) {
    // because of subtyping, the program may be talking about A.b (where A
    // extends B) and b is actually a static field of class B. varIsStatic...
    // checks the entire superclass hierarchy of the class called in the
    // program (in this example, class A) to determine which class actually
    // declared the static variable
    auto actualID = varInfo.second + "." + ID;
    return Builder.CreateLoad(GlobalValues[actualID]);
  } else {
    auto IDIndex = getGEPIndex(ID, staticClassNum);
    auto I =
        GetElementPtrInst::Create(allocatedClasses[typeString(staticClassNum)],
                                  objectLike->codeGen(ST), IDIndex);
    Builder.Insert(I);
    return Builder.CreateLoad(I);
  }
}

Value *DJDotAssign::codeGen(symbolTable ST, int type) {
  auto varInfo = varIsStaticInAnySuperClass(ID, staticClassNum);
  auto ret = assignVal->codeGen(ST);
  if (!ret->getType()->isIntegerTy()) {
    ret = Builder.CreatePointerCast(ret, getLLVMTypeFromDJType(staticClassNum));
  }
  if (varInfo.first) {
    // because of subtyping, the program may be talking about A.b (where A
    // extends B) and b is actually a static field of class B. varIsStatic...
    // checks the entire superclass hierarchy of the class called in the
    // program (in this example, class A) to determine which class actually
    // declared the static variable
    auto actualID = varInfo.second + "." + ID;
    Builder.CreateStore(ret, GlobalValues[actualID]);
  } else {
    auto IDIndex = getGEPIndex(ID, staticClassNum);
    if (hasNullChild) {
      auto I = GetElementPtrInst::Create(
          allocatedClasses[typeString(staticClassNum)], objectLike->codeGen(ST),
          IDIndex);
      Builder.Insert(I);
      ret = assignVal->codeGen(ST, staticClassNum);
      ret =
          Builder.CreatePointerCast(ret, getLLVMTypeFromDJType(staticClassNum));
      Builder.CreateStore(ret, I);
    } else {
      auto I = GetElementPtrInst::Create(
          allocatedClasses[typeString(staticClassNum)], objectLike->codeGen(ST),
          IDIndex);
      Builder.Insert(I);
      Builder.CreateStore(ret, I);
    }
  }
  return ret;
}

Value *DJInstanceOf::codeGen(symbolTable ST, int type) {
  // using the class ID stored at the 1th field in the struct, call the ITable
  // function to determine if the type of the testee expression is a subtype of
  // the classID
  Value *testee = objectLike->codeGen(ST);
  std::vector<Value *> elementIndex = {
      ConstantInt::get(TheContext, APInt(32, 0)),
      ConstantInt::get(TheContext, APInt(32, 1))};
  auto I = GetElementPtrInst::Create(
      allocatedClasses[typeString(staticClassNum)], testee, elementIndex);
  std::vector<Value *> ITableArgs = {
      Builder.CreateLoad(Builder.Insert(I)),
      ConstantInt::get(TheContext, APInt(32, classID))};
  Function *TheFunction = TheModule->getFunction("ITable");
  return Builder.CreateCall(TheFunction, ITableArgs);
}

Value *DJDotMethodCall::codeGen(symbolTable ST, int type) {
  auto className = std::string(typeString(staticClassNum));
  auto LLMethodName = className + "_method_" + methodName;
  symbolTable methodST = NamedValues[LLMethodName];
  symbolTable classST = NamedValues[className];
  Function *TheMethod = TheModule->getFunction(LLMethodName);
  std::vector<Value *> methodArgs = {objectLike->codeGen(ST)};
  if (paramDeclaredType >= OBJECT_TYPE) {
    methodArgs.push_back(
        Builder.CreatePointerCast(methodParameter->codeGen(ST),
                                  getLLVMTypeFromDJType(paramDeclaredType)));
  } else {
    methodArgs.push_back(methodParameter->codeGen(ST));
  }
  return Builder.CreateCall(TheMethod, methodArgs);
}

Value *DJThis::codeGen(symbolTable ST, int type) {
  return Builder.CreateLoad(ST["this"]);
}

Value *DJUndotMethodCall::codeGen(symbolTable ST, int type) {
  auto className = std::string(typeString(staticClassNum));
  auto LLMethodName = className + "_method_" + methodName;
  symbolTable methodST = NamedValues[LLMethodName];
  Function *TheMethod = TheModule->getFunction(LLMethodName);
  std::vector<Value *> methodArgs = {Builder.CreateLoad(ST["this"])};
  if (paramDeclaredType >= OBJECT_TYPE) {
    methodArgs.push_back(
        Builder.CreatePointerCast(methodParameter->codeGen(ST),
                                  getLLVMTypeFromDJType(paramDeclaredType)));
  } else {
    methodArgs.push_back(methodParameter->codeGen(ST));
  }
  return Builder.CreateCall(TheMethod, methodArgs);
}
