#ifndef __DJ2LL_H_
#define __DJ2LL_H_

#include "llast.hpp"
#include "llvm_includes.hpp"
#include "translateAST.hpp"
#include "util.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

ASTree *wholeProgram;
ASTree *mainExprs;
int numMainBlockLocals;
VarDecl *mainBlockST;
int numClasses;
ClassDecl *classesST;
std::string inputFile;

extern FILE *yyin;
extern FILE *yyout;
extern "C" int yyparse(void);
extern ASTree *pgmAST;

bool findCLIOption(char **begin, char **end, const std::string &flag) {
  return std::find(begin, end, flag) != end;
}

void runClang() {
  std::string command = "clang " + inputFile + ".o";
  std::system(command.c_str());
}

void dj2ll(std::map<std::string, bool> compilerFlags, std::string fileName,
           char **argv) {
  std::string extension = fileName.substr(fileName.size() - 3, fileName.size());
  inputFile = fileName.substr(0, fileName.size() - 3);
  if (extension != ".dj") {
    printf("ERROR: %s must be called on files ending with \".dj\"\n", argv[0]);
    exit(-1);
  }

  yyin = fopen(fileName.c_str(), "r");
  if (yyin == NULL) {
    printf("ERROR: could not open file %s\n", fileName.c_str());
    exit(-1);
  }
  yyparse();
  fclose(yyin);

  setupSymbolTables(pgmAST);
  typecheckProgram();

  //  printAST(wholeProgram);
  auto LLProgram = translateAST(wholeProgram);
  LLProgram.print();

  if (compilerFlags["codegen"]) {
    LLProgram.runOptimizations = compilerFlags["optimizations"];
    symbolTable ST; /*throwaway*/
    LLProgram.codeGen(ST);
  }
  runClang();
}

#endif // __DJ2LL_H_
