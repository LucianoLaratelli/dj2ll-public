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
