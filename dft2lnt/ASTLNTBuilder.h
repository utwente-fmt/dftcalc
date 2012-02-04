/*
 * ASTLNTBuilder.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#ifndef ASTLNTBUILDER_H
#define ASTLNTBUILDER_H

#include <vector>

#include "dft_ast.h"
#include "dft_parser.h"
#include "ASTVisitor.h"

namespace DFT {

class ASTLNTBuilder: public DFT::ASTVisitor<int> {
private:
	DFT::AST::ASTNodes* ast;
	Parser* parser;
	DFT::DFTree* dft;
public:

	ASTLNTBuilder(DFT::AST::ASTNodes* ast, Parser* parser): ast(ast), parser(parser), dft(NULL) {
	}

	DFT::DFTree* build() {
		dft = new DFT::DFTree();
		return dft;
	}

	virtual int visitTopLevel(DFT::AST::ASTTopLevel* topLevel) {
	}
	virtual int visitBasicEvent(DFT::AST::ASTBasicEvent* basicEvent) {
	}
	virtual int visitGate(DFT::AST::ASTGate* gate) {
	}
	virtual int visitPage(DFT::AST::ASTPage* page) {
	}
	virtual int visitAttribute(DFT::AST::ASTAttribute* attribute) {
	}
	virtual int visitAttrib(DFT::AST::ASTAttrib* attrib) {
	}
	virtual int visitAttribFloat(DFT::AST::ASTAttribFloat* af) {
	}
	virtual int visitAttribNumber(DFT::AST::ASTAttribNumber* an) {
	}
	virtual int visitAttribString(DFT::AST::ASTAttribString* as) {
	}
};

} // Namespace: DFT

#endif // ASTLNTBUILDER_H
