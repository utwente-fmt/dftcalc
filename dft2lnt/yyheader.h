#ifndef YYHEADER_H
#define YYHEADER_H

	class Parser;

	int yyparse(Parser* parser, yyscan_t scanner, std::vector<DFT::AST::ASTNode*>** result_nodes);
	int yylex(YYSTYPE * yylval_param,YYLTYPE * yylloc_param ,yyscan_t yyscanner);
	void yyerror(Location* location, Parser* parser, yyscan_t scanner, std::vector<DFT::AST::ASTNode*>** result_nodes, const char *str);
	extern int yydebug;
	extern int yychar;
	extern "C" int yywrap(yyscan_t scanner);
 
#endif // YYHEADER_H