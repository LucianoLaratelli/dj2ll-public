/* File ast.h: Abstract-syntax-tree data structure for DJ */
/*Copyright Dr. Ligatti, 2007 - 2020, USF*/
/*Some modifications by Luciano Laratelli*/

#ifndef AST_H
#define AST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

/* define types of AST nodes */
typedef enum {
  /* program, class, field, and method declarations: */
  PROGRAM,
  CLASS_DECL_LIST,
  CLASS_DECL,
  STATIC_VAR_DECL_LIST,
  STATIC_VAR_DECL,
  VAR_DECL_LIST, /* regular (non-static) variable declarations */
  VAR_DECL,
  METHOD_DECL_LIST,
  METHOD_DECL,
  /* types, including generic IDs: */
  NAT_TYPE,
  BOOL_TYPE,
  AST_ID,
  /* expression-lists: */
  EXPR_LIST,
  /* expressions: */
  /*`B:` denotes a base case in the recursion*/
  DOT_METHOD_CALL_EXPR, /* E.ID(E) */
  METHOD_CALL_EXPR,     /* ID(E) */
  DOT_ID_EXPR,          /* E.ID */
  ID_EXPR,              /* B: ID */
  DOT_ASSIGN_EXPR,      /* E.ID = E */
  ASSIGN_EXPR,          /* ID = E */
  PLUS_EXPR,            /* E + E */
  MINUS_EXPR,           /* E - E */
  TIMES_EXPR,           /* E * E */
  EQUALITY_EXPR,        /* E==E */
  GREATER_THAN_EXPR,    /* E > E */
  NOT_EXPR,             /* !E */
  AND_EXPR,             /* E&&E */
  INSTANCEOF_EXPR,      /* E instanceof ID */
  IF_THEN_ELSE_EXPR,    /* if(E) {Es} else {Es} */
  FOR_EXPR,             /* for(E;E;E) {Es} */
  PRINT_EXPR,           /* printNat(E) */
  READ_EXPR,            /* B: readNat() */
  THIS_EXPR,            /* B: this */
  NEW_EXPR,             /* B: new */
  NULL_EXPR,            /* B: null */
  NAT_LITERAL_EXPR,     /* B: N */
  TRUE_LITERAL_EXPR,    /* B: true */
  FALSE_LITERAL_EXPR    /* B: false */
} ASTNodeType;

/* define a list of AST nodes */
typedef struct astlistnode {
  struct astnode *data;
  struct astlistnode *next;
} ASTList;

/* define the actual AST nodes */
typedef struct astnode {
  ASTNodeType typ;
  /* list of children nodes: */
  ASTList *children; /* head of the list of children */
  ASTList *childrenTail;
  /* which source-program line does this node end on: */
  unsigned int lineNumber;
  /* node attributes: */
  unsigned int natVal;
  char *idVal;
  /* Node attributes used on the first 6 kinds of expressions enumerated above
     (E.ID(E), ID(E), E.ID, ID, E.ID = E, and ID = E).
     These attributes get set during type checking and used during code gen,
     so code gen doesn't duplicate the work of the type checker.
     The attributes store the statically determined class and member number
     of an ID that refers to a method, static var, or non-static var.
     When these attributes are all 0, the ID refers not to a member of a class
     but instead to a local/parameter variable. */
  unsigned int staticClassNum; /* class number in which this member resides */
  unsigned int isMemberStaticVar; /* nonzero iff the member is a static var */
  unsigned int staticMemberNum;   /* when set to i, this member is the ith
                                     method/static-var/non-static-var in the
                                     staticClassNum-th class */
} ASTree;

/* METHODS TO CREATE AND MANIPULATE THE AST */

/* Create a new AST node of type t having only one child.
   (That is, create a one-node list of children for the new tree.)
   If the child argument is NULL, then the single list node in the
   new AST node's list of children will have a NULL data field.
   If t is NAT_LITERAL_EXPR then the proper natAttribute should be
   given; otherwise natAttribute is ignored.
   If t is AST_ID then the proper idAttribute should be given;
   otherwise idAttribute is ignored.
*/
ASTree *newAST(ASTNodeType t, ASTree *child, unsigned int natAttribute,
               char *idAttribute, unsigned int lineNum);

/* Append an AST node onto a parent's list of children */
ASTree *appendToChildrenList(ASTree *parent, ASTree *newChild);

void printNodeTypeAndAttribute(ASTree *t);

void printASTree(ASTree *t, int depth);

/* Print the AST to stdout with indentations marking tree depth. */
void printAST(ASTree *t);

#ifdef __cplusplus
}
#endif
#endif
