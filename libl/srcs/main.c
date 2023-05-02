#include <stdio.h>
int  yylex(void);
extern FILE *yyin;

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	if (argc > 1)
		yyin = fopen(argv[1], "r");
	else
		yyin = stdin;
	yylex();
}