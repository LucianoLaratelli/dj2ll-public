#ifndef __CODEGENCLASS_HPP_
#define __CODEGENCLASS_HPP_

#include "llvm_includes.hpp"
#include "util.h"

bool varIsStaticInClass(std::string ID, int classNum);
std::pair<bool, std::string> varIsStaticInAnySuperClass(std::string ID,
                                                        int classNum);
int findClassNumOfStaticVar(std::string ID);

bool isVarRegularInClass(std::string ID, int classNum);

int getIndexOfRegularField(std::string desired, int classIndex);

int getIndexOfRegularOrInheritedField(std::string ID, int classNum);

#endif // __CODEGENCLASS_HPP_
