#ifndef __LLVM_INCLUDES_H_
#define __LLVM_INCLUDES_H_

// llvm library includes, globals involving LLVM classes

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

static llvm::LLVMContext TheContext;
/*Builder keeps track of where we are in the IR and helps us generate
 * instructions*/
static llvm::IRBuilder<> Builder(TheContext);
/*TheModule contains all functions and variables in the source program*/
typedef std::map<std::string, llvm::AllocaInst *> symbolTable;
static std::map<std::string, symbolTable> NamedValues;

static std::map<std::string, llvm::GlobalVariable *> GlobalValues;

static std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;

#endif // __LLVM_INCLUDES_H_
