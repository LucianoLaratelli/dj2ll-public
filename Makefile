all: dj2ll
debug: djdebug
CC=gcc
CXX=g++
CFLAGS=-Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow -Wpointer-arith -Wcast-qual -Wno-missing-braces -pedantic
DFLAGS=-ggdb3 -O0


dj2ll: dj.tab.c lex.yy.c ast.c symtbl.c typecheck.c codegen.cpp
	$(CC) $(CFLAGS) -c ast.c symtbl.c typecheck.c dj.tab.c
	$(CXX) $(CFLAGS) --std=c++11 ast.o symtbl.o typecheck.o dj.tab.o dj2ll.cpp codegen.cpp -o dj2ll

dj.tab.c: dj.y
	bison -v dj.y
	sed -i '/extern YYSTYPE yylval/d' dj.tab.c

lex.yy.c: dj.l
	flex dj.l

clean:
	rm dj2ll dj.tab.c lex.yy.c
