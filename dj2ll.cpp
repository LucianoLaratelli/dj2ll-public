#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "ast.h"
#include "symtbl.h"
#include "typecheck.h"

extern FILE *yyin;
extern FILE *yyout;
extern "C" int yyparse(void);
extern ASTree *pgmAST;

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s filename\n", argv[0]);
    exit(-1);
  }
  std::string fileName = argv[1];
  std::string extension = fileName.substr(fileName.size() - 3, fileName.size());
  if (extension != ".dj") {
    printf("ERROR: %s must be called on files ending with \".dj\"\n", argv[0]);
    exit(-1);
  }
  yyin = fopen(argv[1], "r");
  if (yyin == NULL) {
    printf("ERROR: could not open file %s\n", argv[1]);
    exit(-1);
  }
  /* parse the input program */
  yyparse();
  setupSymbolTables(pgmAST);
  typecheckProgram();
  return 0;
}
