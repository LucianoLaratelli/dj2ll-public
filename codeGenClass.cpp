#include "codeGenClass.hpp"

bool varIsStaticInClass(std::string ID, int classNum) {
  for (int i = 0; i < classesST[classNum].numStaticVars; i++) {
    if (classesST[classNum].staticVarList[i].varName == ID) {
      return true;
    }
  }
  return false;
}

std::pair<bool, std::string> varIsStaticInAnySuperClass(std::string ID,
                                                        int classNum) {
  int count = 0;
  while (count < numClasses && classNum != 0) {
    if (varIsStaticInClass(ID, classNum)) {
      return std::make_pair(true, std::string(typeString(classNum)));
    }
    classNum = classesST[classNum].superclass;
  }
  return std::make_pair(false, "");
}

bool isVarRegularInClass(std::string ID, int classNum) {
  for (int i = 0; i < classesST[classNum].numVars; i++) {
    if (classesST[classNum].varList[i].varName == ID) {
      return true;
    }
  }
  return false;
}

int getIndexOfRegularField(std::string desired, int classIndex) {
  // TODO: squash this and isVarRegular into one to save looping twice
  // given an ID and a class, return the index of that field in the class
  auto ST = classesST[classIndex].varList;
  for (int i = 0; i < classesST[classIndex].numVars; i++) {
    if (std::string(ST[i].varName) == desired) {
      return i;
    }
  }
  return -1;
}

// class memory is laid out like so for a class that declares N fields and
// inherits M fields:
//
// 0            , 1            , 2  , ... , N            , N + 1, ... , N + M
// this pointer , 1th declared , 2nd, ... , Nth declared, 1th inherited,

int getIndexOfRegularOrInheritedField(std::string ID, int classNum) {
  // given an ID and a class from which to start, return the index of a field in
  // that class, offset by 1 for the this pointer and by the number of variables
  // before it in the inheritance graph
  int count = 0;
  int ind = 0;
  while (count < numClasses && classNum != 0) {
    if (isVarRegularInClass(ID, classNum)) {
      return ind + getIndexOfRegularField(ID, classNum) + 1;
    }
    ind += classesST[classNum].numVars;
    classNum = classesST[classNum].superclass;
  }
  return -1;
}
