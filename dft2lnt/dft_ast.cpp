#include "dft_ast.h"

DFT::AST::ASTTopLevel::~ASTTopLevel() {
	if(topNode) delete topNode;
}

DFT::AST::ASTAttribString::~ASTAttribString() {
	if(value) delete value;
}

DFT::AST::ASTAttribute::~ASTAttribute() {
	if(value) delete value;
}

void DFT::AST::ASTTopLevel::setTopNode(ASTIdentifier* topNode) {
	if(this->topNode) delete this->topNode;
	this->topNode = topNode;
}
