all: dj2ll
debug: djdebug
CFLAGS=-Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow -Wpointer-arith -Wcast-qual -Wno-missing-braces -pedantic --std=c++11
DFLAGS=-ggdb3 -O0


dj2dism: dj.tab.c lex.yy.c ast.c symtbl.c typecheck.c codegen.c
	gcc $(CFLAGS) -o dj2dism dj.tab.c ast.c symtbl.c typecheck.c codegen.c

djdebug: dj.tab.c lex.yy.c ast.c symtbl.c typecheck.c codegen.c
	gcc $(CFLAGS) $(DFLAGS) -o dj2dism dj.tab.c ast.c symtbl.c typecheck.c codegen.c

dj.tab.c: dj.y
	bison -v dj.y
	sed -i '/extern YYSTYPE yylval/d' dj.tab.c

lex.yy.c: dj.l
	flex dj.l

clean:
	rm dj2ll dj.tab.c lex.yy.c
