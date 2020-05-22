#ifndef DJ2LL_TYPE_ERROR_HEADER
#define DJ2LL_TYPE_ERROR_HEADER

#ifdef __cplusplus
{
#endif

#include "util.h"
#include <stdio.h>
#include <stdlib.h>

  void reportSemanticError(ASTree * t, char *argumentModifier, ASTree *badChild,
                           int badType, char *expected);
  void reportDuplicateField(char *varName, char *className, char *superClass,
                            int varLine, int superClassVarLine);
  void reportBadType(int lineNumber, char *varName);

#ifdef __cplusplus
}
#endif

#endif
