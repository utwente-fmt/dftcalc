#ifndef DFT2LNT_H
#define DFT2LNT_H

#include "dft_ast.h"
#include "DFTree.h"
#include "ConsoleWriter.h"
#include "compiler.h"
#include "mrmc.h"

namespace DFT {

class DFT2LNT {
public:
	static int validateCommands(const std::vector<DFT::AST::ASTNode*>* ast) {
		int valid = true;
		for(size_t i=0; i<ast->size(); ++i) {
//			valid = ast->at(i)->validate() ? valid : false;
		}
		return valid;
	}
	static DFTree* createDFT(const std::vector<DFT::AST::ASTNode*>* ast) {
		DFTree* dft = new DFTree();
		for(size_t i=0; i<ast->size(); ++i) {
//			ast->at(i)->handleCommand(dft);
		}
		return dft;
	}
};

} //Namespace: DFT

#endif // DFT2LNT_H
