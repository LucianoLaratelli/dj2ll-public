#ifndef DJ2LL_UTIL_HEADER
#define DJ2LL_UTIL_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#include "ast.h"
#include "llast.hpp"
#include "symtbl.h"
#include "typecheck.h"

//  the five built-in DJ types and an error type
#define BAD_TYPE -5   // illegal type
#define NO_OBJECT -4  // the type of Object's superclass
#define ANY_OBJECT -3 // the type of "null"
#define TYPE_BOOL -2
#define TYPE_NAT -1
#define OBJECT_TYPE 0 // the type of every user-defined class' superclass

// colors for pretty-printing error messages
// just having a little fun!
/*TODO RENAME THESE, LLVM FAILLING*/
#define LRESET "\033[0m"
#define LRED "\033[31m"
#define LGREEN "\033[32m"
#define LBOLDRED "\33[1m\033[31m"
#define LBOLDGREEN "\033[1m\033[32m"
#define LBOLDMAGENTA "\033[1m\033[35m"

char *BRBTR(char *arg);
char *BGBTR(char *arg);
char *BMBTR(char *arg);

#define FOURSPACES "    "
#define EIGHTSPACES FOURSPACES FOURSPACES
#define TWELVESPACES EIGHTSPACES FOURSPACES

#define NULL_CHECK(somePointer)                                                \
  if (somePointer == NULL) {                                                   \
    fprintf(                                                                   \
        stderr,                                                                \
        "%s(internal error)\nFound NULL pointer with name %s%s\"%s\"%s%s in "  \
        "function%s%s \"%s\"%s%s, exiting.\n%s",                               \
        LRED, LRESET, LBOLDGREEN, #somePointer, LRESET, LRED, LRESET,          \
        LBOLDMAGENTA, __func__, LRESET, LRED, LRESET);                         \
    exit(-1);                                                                  \
  }

#define DEBUG_TYPE(someType)                                                   \
  printf("type of expr with variable name %s from function %s is %s\n",        \
         #someType, __func__, typeString(someType));

typedef struct {
  int classNum;
  int methodIndex;
  int paramType;
  int returnType;
} methodInfo;

typedef struct {
  int indexST; // index in the symbol table this variable resides
  int isStatic;
} classVarInfo;

void reportLookupError(char *requestedVar, int lineNumber);
int lookupMainST(char *desired, int lineNumber);
int lookupClassesST(char *desired, int lineNumber);
int lookupClassVars(char *desired, int lineNumber, int CCE);
int lookupVarType(char *desired, int CCE, int MCE, int usageLine);
methodInfo findMethodIndex(int CCE, char *methodName, int lineNumber);

int join(int a, int b);
void printMainST(void);
void printVarDeclST(VarDecl *st, int size, char *theClass, char *modifier);
void printMethodDeclST(MethodDecl *st, int size, char *theClass);
void printClassesST(void);
char *typeString(int t);
char *nodeTypeString(ASTNodeType t);

char *numToString(int n);

#ifdef __cplusplus
}
#endif
#endif // #DJ2LL_UTIL_HEADER
