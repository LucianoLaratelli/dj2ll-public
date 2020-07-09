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

std::vector<llvm::Type *> calculateInheritedStorageNeeds(
    int classID, std::map<std::string, llvm::StructType *> &allocatedClasses);

std::map<std::string, std::vector<llvm::Type *>> calculateClassStorageNeeds(
    std::map<std::string, llvm::StructType *> &allocatedClasses);
#endif // __CODEGENCLASS_HPP_
