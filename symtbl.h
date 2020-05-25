/* File symtbl.h: Enhanced symbol-table data structures for DJ */
/*Copyright Dr. Ligatti, 2007 - 2020, USF*/
/*Some modifications by Luciano Laratelli*/

#ifndef SYMTBL_H
#define SYMTBL_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ast.h"

/* METHOD TO SET UP GLOBALS (INCLUDING SYMBOL TABLES) */
/* Using the parameter AST representing the whole program, allocate
   and build the global symbol-table data.

   NOTE on typing conventions:
   This compiler represents types as integers.
   The DJ type denoted by int i is:
     The ith class declared in the source program, if i > 0
     Object, if i = 0
     nat, if i = -1
     bool, if i = -2
     "any-object" (i.e., the type of "null"), if i = -3
     "no-object" (i.e., the type of Object's superclass), if i = -4
     Undefined (i.e., an illegal type), if i < -4
   When i>=0, the symbol table for type (i.e., class) i is at classesST[i].

   NOTE on typechecking:
   This method does NOT perform any checks on the symbol tables
   it builds, although the entry for the Object class
   (in classesST[0]) will of course be free of errors.
   E.g., variable/method/class names may appear multiple times
   in the tables, and types may be invalid.
   If the DJ program declares a variable of an undefined type,
   that type will appear as -5 in the symbol table. */
void setupSymbolTables(ASTree *fullProgramAST);

/* HELPER METHOD TO CONVERT TYPE NAMES TO NUMBERS */
/* Returns the number for a given type name.
   Returns: -5 if type does not exist, -2 for bool, -1 for nat, 0 for Object,
     1 for first class declared in program,
     2 for 2nd class declared in program, etc.
     (-3 and -4 are reserved for "any-object" and "no-object" types)
   Determines number for given type name using the global
   variable wholeProgram (defined below).  */
int typeNameToNumber(char *typeName);

/* TYPEDEFS FOR ENHANCED SYMBOL TABLES */
/* Encapsulate all information relevant to a DJ variable:
   the variable name, source-program line number on which the variable is
   declared, variable type, and line number on which the type appears. */
typedef struct vdecls {
  char *varName;
  int varNameLineNumber;
  int type;
  int typeLineNumber;
} VarDecl;

/* Encapsulate all information relevant to a DJ method:
   the method name, return type, parameter name, parameter type,
   local variables, and method body. */
typedef struct mdecls {
  char *methodName;
  int methodNameLineNumber;
  int returnType;
  int returnTypeLineNumber;
  char *paramName;
  int paramNameLineNumber;
  int paramType;
  int paramTypeLineNumber;

  // An array of this method's local variables
  int numLocals;    // size of the array
  VarDecl *localST; // the array itself

  // The method's executable body
  ASTree *bodyExprs;
} MethodDecl;

/* Encapsulate all information relevant to a DJ class:
   the class name, superclass, and arrays of information
   about the class's variables and methods. */
typedef struct classinfo {
  char *className;
  int classNameLineNumber;
  int superclass;
  int superclassLineNumber;

  // array of static-variable information--the ith element of
  // the array encapsulates information about the ith
  // static variable field in this class
  int numStaticVars;      // size of the array
  VarDecl *staticVarList; // the array itself

  // array of (non-static) variable information--the ith element of
  // the array encapsulates information about the ith
  //(non-static) variable field in this class
  int numVars;      // size of the array
  VarDecl *varList; // the array itself

  // array of method information--the ith element of the array
  // encapsulates information about the ith method in this class
  int numMethods;         // size of the array
  MethodDecl *methodList; // the array itself
} ClassDecl;

/* GLOBALS THAT PROVIDE EASY ACCESS TO PARTS OF THE AST */
/* THESE GLOBALS GET SET IN setupSymbolTables */
// The entire program's AST
#ifdef __cplusplus
extern "C" ASTree *wholeProgram;
#else
extern ASTree *wholeProgram;
#endif

// The expression list in the main block of the DJ program
#ifdef __cplusplus
extern "C" ASTree *mainExprs;
#else
extern ASTree *mainExprs;
#endif

// Array (symbol table) of locals in the main block
#ifdef __cplusplus
extern "C" int numMainBlockLocals; // size of the array
#else
extern int numMainBlockLocals; // size of the array
#endif
#ifdef __cplusplus
extern "C" VarDecl *mainBlockST; // the array itself
#else
extern VarDecl *mainBlockST;   // the array itself
#endif

// Array (symbol table) of class declarations
// Note that the minimum array size is 1,
//   due to the always-present Object class
#ifdef __cplusplus
extern "C" int numClasses; // size of the array
#else
extern int numClasses;         // size of the array
#endif
#ifdef __cplusplus
extern "C" ClassDecl *classesST; // the array itself
#else
extern ClassDecl *classesST;   // the array itself
#endif

#ifdef __cplusplus
}
#endif
#endif
