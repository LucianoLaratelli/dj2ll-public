#ifndef __CODEGENCLASS_HPP_
#define __CODEGENCLASS_HPP_

#include "llvm_includes.hpp"
#include "util.h"

bool varIsStaticInClass(std::string ID, int classNum);
std::pair<bool, std::string> varIsStaticInAnySuperClass(std::string ID,
                                                        int classNum);
int findClassNumOfStaticVar(std::string ID);

#endif // __CODEGENCLASS_HPP_
