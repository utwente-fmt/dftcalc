/*
 * dft_parser.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

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

	CompilerContext* compilerContext;
public:
	yyscan_t scanner;

	/**
	 * Constructs a new parser using the specified file properties and
	 * CompilerContext. The CompilerContext will be used to generate
	 * errors and warnings.
	 * Call parser() to start parsing.
	 */
	Parser(FILE* file, std::string fileName, CompilerContext* compilerContext): file(file), fileName(fileName), compilerContext(compilerContext) {
		assert(compilerContext);
	}

	/**
	 * Start the parsing. The parsing will start with the file specified in
	 * the constructor. The specified CompilerContext will be used to generate
	 * errors and warnings.
	 * The parser will build the AST along the way and return it.
	 * @return The list of ASTNodes parsed from the source.
	 */
	DFT::AST::ASTNodes* parse();

	/**
	 * Start parsing at the specified file. After the parsing of this file is
	 * done, yywrap() will be automatically called, which will call popFile().
	 * After the specified file has been parser, the parser will continue where
	 * it paused with the previous file on the stack. If there is no previous
	 * file, the parsing is done and parse() will return.
	 */
	FILE* pushFile(std::string fileName);

	/**
	 * Pops the last file from the parse stack. See pushFile() for more.
	 * @return UNDECIDED
	 */
	int popFile();
	
	/**
	 * Returns the current source file where the Parser is currently at.
	 * @return he current source file where the Parser is currently at.
	 */
	std::string getCurrentFileName();

	/**
	 * Returns the current source location where the Parser is currently at.
	 * @return he current source location where the Parser is currently at.
	 */
	Location getCurrentLocation();
	
	/**
	 * Returns the CompilerContext used by this Parser.
	 * @return The CompilerContext used by this Parser.
	 */
	CompilerContext* getCC() { return compilerContext; }
};

#endif // DFT_PARSER_H