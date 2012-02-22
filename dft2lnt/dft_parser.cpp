/*
 * dft_parser.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include "dft_parser.h"
#include "dft_ast.h"
#include <iostream>

using namespace std;

Parser* pp = NULL;

#include "lexer.l.h"
#include "yyheader.h"

#ifndef __PARSER_Y_HPP
#define __PARSER_Y_HPP
#	include "parser.y.hpp"
#endif


#include "math.h"

int yywrap(yyscan_t scanner) {
	return yyget_extra(scanner)->popFile();
}

void yyerror(Location* location, Parser* parser, yyscan_t scanner, DFT::AST::ASTNodes **result_nodes, const char *str) {
//	fprintf(stderr,"PARSE ERROR: %s\n",str);
//	switch(yychar) {
//		case LACCOL:
//			fprintf(stderr,"Expected curly brace!\n");
//	}
}

DFT::AST::ASTNodes* Parser::parse() {

#if defined(DEBUG) || !defined(NDEBUG)
//	yydebug = 1;
#endif

	if(pp) {
		assert(0 && "Already a parser is active!");
		return NULL;
	}
	if(!file) {
		return NULL;
	}
	
	pp = this;
	
	yylex_init_extra(this,&scanner);
	//yyin = file;
	yyset_in(file,scanner);
	compilerContext->fileContexts = 1;
	
	// Add to FILE* stack
	compilerContext->fileContext[0].fileHandle = file;
	compilerContext->fileContext[0].filename = fileName;
	
	// 0: valid grammer, 1: invalid grammar
	DFT::AST::ASTNodes* result_nodes = NULL;
	yyparse(this,scanner,&result_nodes);
	fflush(stdout);
	
	// Free the lexer
	yylex_destroy(scanner);
	
	return result_nodes;//new Program(ASTroot);
}

FILE* Parser::pushFile(std::string fileName) {
	if((compilerContext->getFileContexts())>=MAX_FILE_NESTING) {
		printf("ERROR: Files nested too deep\n");
		return NULL;
	}
	FILE* f = fopen(fileName.c_str(),"r");
	if(!f) {
		printf("ERROR: unable to open file %s\n",fileName.c_str());
		return NULL;
	}
	
	// Create the buffer for yacc and push the buffer
	YY_BUFFER_STATE buffer = yy_create_buffer(f,YY_BUF_SIZE,scanner);
	yypush_buffer_state(buffer,scanner);
	
	// Add to FILE* stackLocation
	compilerContext->fileContext[(compilerContext->fileContexts)].fileHandle = f;
	compilerContext->fileContext[(compilerContext->fileContexts)].filename = fileName;
	++(compilerContext->fileContexts);
	
	// Remember yylloc of last buffer and reset yylloc
	assert((compilerContext->fileContexts)-2 >= 0);
	compilerContext->fileContext[(compilerContext->fileContexts)-2].loc = *yyget_lloc(scanner);
	yyget_lloc(scanner)->reset();
	yyget_lloc(scanner)->setFileName(compilerContext->fileContext[(compilerContext->fileContexts)-1].filename);
	
	// Return the FILE*
	return f;
}

int Parser::popFile() {
	
	// Pop buffer from stack
	yypop_buffer_state(scanner);
	
	// Remove from FILE* stack and close FILE*
	assert(compilerContext);
	fclose(compilerContext->fileContext[--(compilerContext->fileContexts)].fileHandle);
	
	if(compilerContext->fileContexts>0) {
	
		// Reinit yylloc from buffer
		*yyget_lloc(scanner) = compilerContext->fileContext[(compilerContext->fileContexts-1)].loc;
	}
	
	// Return whether this was last FILE* or not
	return (compilerContext->fileContexts)<=0;
}

Location Parser::getCurrentLocation() {
	return *yyget_lloc(scanner);
}

std::string Parser::getCurrentFileName() {
	return compilerContext->fileContext[(compilerContext->fileContexts-1)].filename;
}