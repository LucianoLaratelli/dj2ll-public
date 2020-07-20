#ifndef DJ2LL_H
#define DJ2LL_H

#include "llast.hpp"
#include "llvm_includes.hpp"
#include "translateAST.hpp"
#include "util.h"

extern FILE *yyin;
extern FILE *yyout;
extern "C" int yyparse(void);
extern ASTree *pgmAST;

bool findCLIOption(char **begin, char **end, const std::string &flag);

void runClang();

void dj2ll(std::map<std::string, bool> compilerFlags, std::string fileName,
           char **argv);

#endif // __DJ2LL_H_
