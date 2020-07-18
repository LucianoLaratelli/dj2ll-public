#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "dj2ll.hpp"

int main(int argc, char **argv) {
  std::vector<std::string> availableFlags = {"--skip-codegen", "--run-optis",
                                             "emit-llvm", "--verbose"};
  std::map<std::string, bool> compilerFlags;
  compilerFlags["codegen"] = true;
  compilerFlags["optimizations"] = false;
  compilerFlags["emitLLVM"] = false;
  compilerFlags["verbose"] = false;
  if (argc < 2) {
    printf("Usage: %s filename [flags]", argv[0]);
    printf("I know about these flags:\n");
    for (auto f : availableFlags) {
      printf("%s%s\n", FOURSPACES, f.c_str());
    }
    exit(-1);
  } else if (argc > 2) {
    if (findCLIOption(argv, argv + argc, "--skip-codegen")) {
      compilerFlags["codegen"] = false;
    } else if (findCLIOption(argv, argv + argc, "--run-optis")) {
      compilerFlags["optimizations"] = true;
    } else if (findCLIOption(argv, argv + argc, "--emit-llvm")) {
      compilerFlags["emitLLVM"] = true;
    } else if (findCLIOption(argv, argv + argc, "--verbose")) {
      compilerFlags["verbose"] = true;
    } else {
      printf("%sUnknown flag detected.%s\nI only know about these flags:\n",
             LRED, LRESET);
      for (auto f : availableFlags) {
        printf("%s%s\n", FOURSPACES, f.c_str());
      }
      exit(-1);
    }
  }
  std::string fileName = argv[1];
  dj2ll(compilerFlags, fileName, argv);
}
