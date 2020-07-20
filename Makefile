.PHONY: all
.PHONY: clean
all: dj2ll
CC=clang
CXX=clang++
CFLAGS=-O2
DFLAGS=-ggdb3 -O0 -fstandalone-debug
WFLAGS =    -Weverything                        \
			-Wno-exit-time-destructors          \
			-Wno-global-constructors            \
			-Wno-non-virtual-dtor               \
			-Wno-padded                         \
			-Wno-shadow-field-in-constructor    \
			-Wno-shadow                         \
			-Wno-sign-conversion                \
			-Wno-weak-vtables                   \
			-Wno-c++98-compat                   \

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
	$(CXX)  $(CFLAGS) $(WFLAGS) --std=c++11 $(OBJECTS) $(CXXSOURCES) $(DJ2LLMAIN) `llvm-config --cxxflags --ldflags --system-libs --libs all` -o dj2ll

debug: lex.yy.c $(CSOURCES) $(CXXSOURCES)
	$(CC)  $(DFLAGS) $(WFLAGS) -c $(CSOURCES)
	$(CXX)  $(DFLAGS) $(WFLAGS) --std=c++11  $(OBJECTS) $(CXXSOURCES) $(DJ2LLMAIN) `llvm-config --cxxflags --ldflags --system-libs --libs all` -o dj2ll

test: lex.yy.c $(CSOURCES) $(CXXSOURCES)
	$(CC)  $(CFLAGS) $(WFLAGS) -c $(CSOURCES)
	$(CXX)  $(CFLAGS) $(WFLAGS) --std=c++11  $(OBJECTS) $(CXXSOURCES) $(TESTMAIN) `llvm-config --cxxflags --ldflags --system-libs --libs all` -o dj2ll

dj.tab.c: dj.y
	$(BISON) dj.y
	$(SED) -i '/extern YYSTYPE yylval/d' dj.tab.c
	echo "#pragma clang diagnostic push" > temp
	echo "#pragma clang diagnostic ignored \"-Weverything\"" > temp
	cat dj.tab.c >> temp
	echo "#pragma clang diagnostic pop" >> temp
	cat temp > dj.tab.c
	rm temp

lex.yy.c: dj.l
	flex dj.l

clean:
	@rm -f dj2ll *.o dj.tab.c lex.yy.c
