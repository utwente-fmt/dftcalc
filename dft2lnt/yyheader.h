/*
 * yyheader.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#ifndef YYHEADER_H
#define YYHEADER_H

	class Parser;

	int yyparse(Parser* parser, yyscan_t scanner, DFT::AST::ASTNodes** result_nodes);
	int yylex(YYSTYPE * yylval_param,YYLTYPE * yylloc_param ,yyscan_t yyscanner);
	void yyerror(Location* location, Parser* parser, yyscan_t scanner, DFT::AST::ASTNodes** result_nodes, const char *str);
	extern int yydebug;
	extern int yychar;
	extern "C" int yywrap(yyscan_t scanner);
 
#endif // YYHEADER_H