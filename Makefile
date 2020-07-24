.PHONY: all
.PHONY: clean
all: dj2ll

CC=clang
CXX=clang++
CFLAGS=-O2
DFLAGS=-ggdb3 -O0 -fstandalone-debug
WFLAGS = -Wall -Wpedantic -Wunused-variable -Wno-c++17-extensions

BISON=bison
SED=sed
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	BISON=/usr/local/opt/bison/bin/bison
	SED=gsed
endif

CSOURCES=ast.c symtbl.c typecheck.c util.c dj.tab.c typeErrors.c
CXXSOURCES=codegen.cpp codeGenClass.cpp llast.cpp translateAST.cpp dj2ll.cpp test.cpp
DJ2LLMAIN=main.cpp
TESTMAIN=testMain.cpp
OBJECTS=ast.o dj.tab.o symtbl.o typecheck.o typeErrors.o util.o

dj2ll: lex.yy.c $(CSOURCES) $(CXXSOURCES)
	$(CC)  $(CFLAGS) $(WFLAGS) -c $(CSOURCES)
	$(CXX)  $(CFLAGS) $(WFLAGS) --std=c++17 $(OBJECTS) $(CXXSOURCES) $(DJ2LLMAIN) `llvm-config --cxxflags --ldflags --system-libs --libs all` -o dj2ll

debug: lex.yy.c $(CSOURCES) $(CXXSOURCES)
	$(CC)  $(DFLAGS) $(WFLAGS) -c $(CSOURCES)
	$(CXX)  $(DFLAGS) $(WFLAGS) --std=c++17  $(OBJECTS) $(CXXSOURCES) $(DJ2LLMAIN) `llvm-config --cxxflags --ldflags --system-libs --libs all` -o dj2ll

test: lex.yy.c $(CSOURCES) $(CXXSOURCES)
	$(CC)  $(CFLAGS) $(WFLAGS) -c $(CSOURCES)
	$(CXX)  $(CFLAGS) $(WFLAGS) --std=c++17  $(OBJECTS) $(CXXSOURCES) $(TESTMAIN) `llvm-config --cxxflags --ldflags --system-libs --libs all` -o dj2ll

dj.tab.c: dj.y
	$(BISON) dj.y
	$(SED) -i '/extern YYSTYPE yylval/d' dj.tab.c

lex.yy.c: dj.l
	flex dj.l

clean:
	@rm -f dj2ll *.o dj.tab.c lex.yy.c
