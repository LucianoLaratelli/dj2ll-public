#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "dj2ll.hpp"

int main(int argc, char **argv) {
  std::vector<std::string> availableFlags = {"--skip-codegen", "--run-optis",
                                             "--emit-llvm", "--verbose"};
  std::map<std::string, bool> compilerFlags;
  compilerFlags["codegen"] = true;
  compilerFlags["optimizations"] = false;
  compilerFlags["emitLLVM"] = false;
  compilerFlags["verbose"] = false;
  if (argc < 2) {
    printf("Usage: %s filename [flags]\n", argv[0]);
    printf("I know about these flags:\n");
    for (auto f : availableFlags) {
      printf("%s%s\n", FOURSPACES, f.c_str());
    }
    exit(-1);
  } else if (argc > 2) {
    if (findCLIOption(argv, argv + argc, "--skip-codegen")) {
      compilerFlags["codegen"] = false;
    }
    if (findCLIOption(argv, argv + argc, "--run-optis")) {
      compilerFlags["optimizations"] = true;
    }
    if (findCLIOption(argv, argv + argc, "--emit-llvm")) {
      compilerFlags["emitLLVM"] = true;
    }
    if (findCLIOption(argv, argv + argc, "--verbose")) {
      compilerFlags["verbose"] = true;
    }
  }
  std::string fileName = argv[1];
  dj2ll(compilerFlags, fileName, argv);
}
