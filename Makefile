all: dj2ll
CC=gcc
CXX=g++
CFLAGS=-O2
ANNOY=-pedantic
WFLAGS=-Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow -Wpointer-arith -Wcast-qual -Wno-missing-braces
DFLAGS=-ggdb3 -O0

CSOURCES=ast.c symtbl.c typecheck.c util.c dj.tab.c
CXXSOURCES=codegen.cpp dj2ll.cpp

dj2ll: lex.yy.c $(CSOURCES) $(CXXSOURCES)
	$(CC)  $(CFLAGS) $(WFLAGS) $(ANNOY) -c $(CSOURCES)
	$(CXX) $(CFLAGS) $(WFLAGS) $(ANNOY) --std=c++11 *.o $(CXXSOURCES) -o dj2ll

debug: lex.yy.c $(CSOURCES) $(CXXSOURCES)
	$(CC)  $(DFLAGS) $(WFLAGS) $(ANNOY) -c $(CSOURCES)
	$(CXX) $(DFLAGS) $(WFLAGS) $(ANNOY) --std=c++11 *.o $(CXXSOURCES) -o dj2ll

dj.tab.c: dj.y
	bison dj.y
	sed -i '/extern YYSTYPE yylval/d' dj.tab.c

lex.yy.c: dj.l
	flex dj.l

.PHONY: clean

clean:
	@rm -f $(TARGET) $(OBJECTS)
