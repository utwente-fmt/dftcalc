class Parser;

#ifndef DFT_PARSER_H
#define DFT_PARSER_H

#include <vector>
#include <assert.h>
#include <iostream>
#include <stdio.h>

#include "dft_ast.h"
#include "dft2lnt.h"
#include "dft_parser_location.h"

using namespace std;

extern Parser* pp;

/* An opaque pointer. */
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif


/**
 * Creates a new parser.
 */
class Parser {
private:
	FILE* file;
	std::string fileName;

	DFT::CompilerContext* compilerContext;
public:
	yyscan_t scanner;

	Parser(FILE* file, std::string fileName, DFT::CompilerContext* compilerContext): file(file), fileName(fileName), compilerContext(compilerContext) {
		assert(compilerContext);
	}

	std::vector<DFT::AST::ASTNode*>* parse();

	FILE* pushFile(std::string fileName);

	int popFile();

	std::string getCurrentFileName();

	Location getCurrentLocation();
	
	DFT::CompilerContext* getCC() { return compilerContext; }
};

#endif // DFT_PARSER_H