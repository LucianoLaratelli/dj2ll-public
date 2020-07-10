#include "codeGenClass.hpp"
#include "llvm_includes.hpp"

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
// 0,1,2,...,N,N+1,...,N+M
// this pointer,class number , 1th declared , 2nd, ... , Nth declared, 1th
// inherited,

int getIndexOfRegularOrInheritedField(std::string ID, int classNum) {
  // given an ID and a class from which to start, return the index of a field in
  // that class, offset by 1 for the this pointer, by another 1 for the class
  // ID, and by the number of variables before it in the inheritance graph
  int count = 0;
  int ind = 0;
  while (count < numClasses && classNum != 0) {
    if (isVarRegularInClass(ID, classNum)) {
      return ind + getIndexOfRegularField(ID, classNum) + 1 + 1;
    }
    ind += classesST[classNum].numVars;
    classNum = classesST[classNum].superclass;
  }
  return -1;
}

using namespace llvm;
std::vector<Type *> calculateInheritedStorageNeeds(
    int classNum, std::map<std::string, llvm::StructType *> &allocatedClasses) {
  // given a class ID, iterate inclusively from that class through all its
  // superclasses, adding LLVM types to its declaration
  int count = 0;
  std::vector<Type *> members;
  while (count < numClasses && classNum != 0 && classNum != -4) {
    for (int j = 0; j < classesST[classNum].numVars; j++) {
      switch (classesST[classNum].varList[j].type) {
      case BAD_TYPE:
      case NO_OBJECT:
      case ANY_OBJECT:
        std::cerr
            << "bad regular var encountered in calculateClassStorageNeeds\n";
        exit(-1);
      case TYPE_BOOL: {
        members.push_back(Type::getInt1Ty(TheContext));
        break;
      }
      case TYPE_NAT: {
        members.push_back(Type::getInt32Ty(TheContext));
        break;
      }
      default: { // all objects
        members.push_back(PointerType::getUnqual(
            allocatedClasses[typeString(classesST[classNum].varList[j].type)]));
        break;
      }
      }
    }
    classNum = classesST[classNum].superclass;
  }
  return members;
}

std::map<std::string, std::vector<llvm::Type *>> calculateClassStorageNeeds(
    std::map<std::string, llvm::StructType *> &allocatedClasses) {
  // determine the storage needs of every class declared by the program
  std::map<std::string, std::vector<llvm::Type *>> ret;
  std::vector<llvm::Type *> members;
  for (int i = 0; i < numClasses; i++) {
    /*allocate `this` pointer*/
    members.push_back(
        PointerType::getUnqual(allocatedClasses[classesST[i].className]));
    /*make space for class ID*/
    members.push_back(Type::getInt32Ty(TheContext));
    for (int j = 0; j < classesST[i].numVars; j++) {
      switch (classesST[i].varList[j].type) {
      case BAD_TYPE:
      case NO_OBJECT:
      case ANY_OBJECT:
        std::cerr
            << "bad regular var encountered in calculateClassStorageNeeds\n";
        exit(-1);
      case TYPE_BOOL: {
        members.push_back(Type::getInt1Ty(TheContext));
        break;
      }
      case TYPE_NAT: {
        members.push_back(Type::getInt32Ty(TheContext));
        break;
      }
      default: { // all objects
        members.push_back(PointerType::getUnqual(
            allocatedClasses[typeString(classesST[i].varList[j].type)]));
        break;
      }
      }
    }
    auto inherited = calculateInheritedStorageNeeds(classesST[i].superclass,
                                                    allocatedClasses);
    members.insert(members.end(), inherited.begin(), inherited.end());
    ret[classesST[i].className] = members;
    members.clear();
  }
  return ret;
}
