#ifndef __CODEGENCLASS_HPP_
#define __CODEGENCLASS_HPP_

#include "llvm_includes.hpp"
#include "util.h"
#include <iostream>

bool varIsStaticInClass(std::string ID, int classNum);
std::pair<bool, std::string> varIsStaticInAnySuperClass(std::string ID,
                                                        int classNum);
int findClassNumOfStaticVar(std::string ID);

bool isVarRegularInClass(std::string ID, int classNum);

int getIndexOfRegularField(std::string desired, int classIndex);

int getIndexOfRegularOrInheritedField(std::string ID, int classNum);

typedef int classID;
typedef int methodNum;
std::pair<classID, methodNum>
getDynamicMethodInfo(int staticClass, int staticMethod, int dynamicType);

bool methodTypeMatchesVTable(int methodReturn, int methodParam,
                             std::string VTableRet, std::string VTableParam);

#endif // __CODEGENCLASS_HPP_
