#include <cstdio>
#include <cstdlib>
#include <cstring>

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
  // mostly taken from https://stackoverflow.com/a/5309508
  char *dot = strrchr(argv[1], '.');
  // TODO: actually implement the extension-checking
  if (!dot || dot == argv[1]) {
    printf("ERROR: %s must be called on files ending with \".dj\"\n", argv[1]);
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

  char output[1024];
  strncpy(output, argv[1], (dot - argv[1] + 1));
  strcat(output, "dism\0");

  FILE *outputFile = fopen(output, "w");

  fclose(outputFile);

  return 0;
}
