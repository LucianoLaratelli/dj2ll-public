#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include "llvm_includes.hpp"
#include "util.h"
#include <fstream>

void generateLL(std::ofstream &outputFile);

void codeGenExpr(ASTree *t, int classNumber, int methodNumber);
void codeGenExprs(ASTree *expList, int classNumber, int methodNumber);

llvm::Type *getLLVMTypeFromDJType(std::string djType);
llvm::Type *getLLVMTypeFromDJType(int djType);

extern ASTree *wholeProgram;
// The expression list in the main block of the DJ program
extern ASTree *mainExprs;
// Array (symbol table) of locals in the main block
extern int numMainBlockLocals; // size of the array
extern VarDecl *mainBlockST;   // the array itself
// Array (symbol table) of class declarations
// Note that the minimum array size is 1,
//   due to the always-present Object class
extern int numClasses;       // size of the array
extern ClassDecl *classesST; // the array itself

#endif
