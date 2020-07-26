/* File typecheck.h: Typechecker for DJ */
/*Copyright Dr. Ligatti, 2007 - 2020, USF*/

#ifndef TYPECHECK_H
#define TYPECHECK_H
#ifdef __cplusplus
extern "C" {
#endif
#include "symtbl.h"

/* This method performs all typechecking for the entire DJ program.
   This method assumes setupSymbolTables(), declared in symtbl.h,
   has already executed to set all the global variables (i.e., the
   enhanced symbol tables) declared in symtbl.h.

   If this method finds a typing error, it reports the error and
   exits the compiler.
*/
void typecheckProgram(void);

/* HELPER METHODS FOR typecheckProgram(): */

/* Returns nonzero iff sub is a subtype of super */
int isSubtype(int sub, int super);

/* Returns the type of the expression AST in the given context.
   Also sets t->staticClassNum, t->isMemberStaticVar, and t->staticMemberNum
   attributes as needed.
   If classContainingExpr < 0 then this expression is in the main
   block of the program; otherwise the expression is in the given class.
*/
int typeExpr(ASTree *t, int classContainingExpr, int methodContainingExpr);

/* Returns the type of the EXPR_LIST AST in the given context. */
int typeExprs(ASTree *t, int classContainingExprs, int methodContainingExprs);
#ifdef __cplusplus
}
#endif
#endif
