#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "llast.hpp"
#include "llvm_includes.hpp"
#include "translateAST.hpp"
#include "util.h"

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

struct CInterface {
  ClassDecl *classesST;
  int numclasses;
  VarDecl *mainBlockST;
};

int main(int argc, char **argv) {
  bool codegen = true;
  if (argc < 2) {
    printf("Usage: %s filename\n", argv[0]);
    exit(-1);
  } else if (argc == 3) {
    if (std::string(argv[2]) == "--skip-codegen") {
      codegen = false;
    } else {
      printf("I only know about the --skip-codegen flag.\n");
      exit(-1);
    }
  }
  std::string fileName = argv[1];
  std::string extension = fileName.substr(fileName.size() - 3, fileName.size());
  inputFile = fileName.substr(0, fileName.size() - 3);
  std::cout << inputFile << std::endl;
  if (extension != ".dj") {
    printf("ERROR: %s must be called on files ending with \".dj\"\n", argv[0]);
    exit(-1);
  }

  yyin = fopen(argv[1], "r");
  if (yyin == NULL) {
    printf("ERROR: could not open file %s\n", argv[1]);
    exit(-1);
  }
  yyparse();
  fclose(yyin);

  setupSymbolTables(pgmAST);
  typecheckProgram();

  //  printAST(wholeProgram);
  auto LLProgram = translateAST(wholeProgram);
  LLProgram.print();

  if (codegen) {
    LLProgram.codeGen();
  }
}
