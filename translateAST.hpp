#ifndef __TRANSLATEAST_H_
#define __TRANSLATEAST_H_
/*functions that handle translation from ASTree type to a tree of DJNodes as
 * defined in llast.hpp/.cpp*/

#include "llast.hpp"
#include "util.h"

DJProgram translateAST(ASTree *old);

ClassDeclList translateClassDeclList(ASTree *old);
VarDeclList translateVarDeclList(ASTree *old);
ExprList translateExprList(ASTree *old);

DJExpression *translateExpr(ASTree *old);

#endif // __TRANSLATEAST_H_
