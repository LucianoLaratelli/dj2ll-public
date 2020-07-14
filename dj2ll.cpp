#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "dj2ll.hpp"

int main(int argc, char **argv) {
  std::map<std::string, bool> compilerFlags;
  compilerFlags["codegen"] = true;
  compilerFlags["optimizations"] = false;
  if (argc < 2) {
    printf("Usage: %s [flags] filename\n", argv[0]);
    printf("I also know about these flags:\n%s--skip-codegen\n%s--run-optis\n",
           FOURSPACES, FOURSPACES);
    exit(-1);
  } else if (argc > 2) {
    if (findCLIOption(argv, argv + argc, "--skip-codegen")) {
      compilerFlags["codegen"] = false;
    } else if (findCLIOption(argv, argv + argc, "--run-optis")) {
      compilerFlags["optimizations"] = true;
    } else {
      printf(
          "I only know about these flags:\n%s--skip-codegen\n%s--run-optis\n",
          FOURSPACES, FOURSPACES);
      exit(-1);
    }
  }
  std::string fileName = argv[argc - 1];
  dj2ll(compilerFlags, fileName, argv);
}
