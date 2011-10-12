#ifndef ASTVALIDATOR_H
#define ASTVALIDATOR_H

#include <vector>
#include <iostream>

#include "dft_ast.h"
#include "dft_parser.h"
#include "ASTVisitor.h"
#include "ASTDFTBuilder.h"

namespace DFT {

/**
 * This class handles the validation of an AST.
 * It will validate the following concepts:
 *   - All references are to existing nodes
 */
class ASTValidator: public DFT::ASTVisitor<int,true> {
private:
	std::vector<std::string> definedNodes;
public:

	/**
	 * Constructs a new ASTValidator using the specified
	 * AST and CompilerContext.
	 * Call validate() to start the validation process.
	 */
	ASTValidator(std::vector<DFT::AST::ASTNode*>* ast, CompilerContext* cc):
		ASTVisitor(ast,cc,[](int& ret, int val){ret = ret && val;}) {
	}

	/**
	 * Builds a list of defined DFT nodes in the AST specified in the
	 * constructor. The definedNodes member will be overridden.
	 */
	void buildDefinedNodesList() {
		
		// Clear the list
		definedNodes.clear();
		
		// Go through all the AST nodes in a linear fashion
		for(int i=0; i<ast->size(); ++i) {
			DFT::AST::ASTNode* node = ast->at(i);
			switch(node->getType()) {
				
			// TopLevel references a node
			case DFT::AST::TopLevelType: {
				DFT::AST::ASTTopLevel* t = static_cast<DFT::AST::ASTTopLevel*>(node);
				//referencedNodes.push_back(t->getTopNode());
				break;
			}

			// A BasicEvent ASTNode defines a DFT node
			case DFT::AST::BasicEventType: {
				DFT::AST::ASTBasicEvent* be = static_cast<DFT::AST::ASTBasicEvent*>(node);
				//std::cout << "Defined: " << basicEvent->getName() << std::endl;
				assert(be);
				assert(be->getName());
				definedNodes.push_back(be->getName()->getString());
				break;
			}

			// Any Gate ASTNode defines a DFT node
			// Any Gate can reference multiple nodes
			case DFT::AST::GateType: {
				DFT::AST::ASTGate* g = static_cast<DFT::AST::ASTGate*>(node);
				definedNodes.push_back(g->getName()->getString());
				std::vector<DFT::AST::ASTIdentifier*>* children = g->getChildren();
				//for(int i=children->size();i--;) {
				//	referencedNodes.push_back(children->at(i));
				//}
				break;
			}
			
			// A Page ASTNode references a node
			case DFT::AST::PageType: {
				DFT::AST::ASTPage* p = static_cast<DFT::AST::ASTPage*>(node);
				//referencedNodes.push_back(p->getNodeName());
				break;
			}
			default:
				break;
			}
		}
	}

	/**
	 * Starts the validation process of the AST specified in the
	 * constructor. Returns whether the AST is deemed valid or not.
	 * Returns true if it is valid, false otherewise.
	 */
	int validate() {
		
		buildDefinedNodesList();
		
		int valid = true;
		
		valid = ASTVisitor::visit() ? valid : false;
		
		return valid;
	}
	
/*	int checkReferences() {
		int valid = true;
		std::vector<std::string> unknownNodes = referencedNodes;

//		std::cout << "Referenced nodes:";
//		for(int i=0;i<unknownNodes.size();++i) {
//			std::cout << " " << unknownNodes.at(i);
//		}
//		std::cout << std::endl;
		
		for(int i=definedNodes.size();i--;) {
		//	std::cout << "== Removing: " << definedNodes.at(i) << std::endl;
			std::vector<std::string>::iterator it = std::find(unknownNodes.begin(),unknownNodes.end(),definedNodes.at(i));
			if(it != unknownNodes.end()) unknownNodes.erase(it);
		}
		for(int i=0;i<unknownNodes.size();++i) {
			parser->getCC()->reportErrorAt(unknownNodes.at(i),"undefined node: ")
			std::cout << "Undefined node: " << unknownNodes.at(i) << std::endl;
		}
		valid = unknownNodes.size()==0 ? valid : false;

		return valid;
	}
*/

	virtual int visitTopLevel(DFT::AST::ASTTopLevel* topLevel) {
		int valid = true;
		
		valid = ASTVisitor::visitTopLevel(topLevel) ? valid : false ;
		
		std::vector<std::string>::iterator it = std::find(definedNodes.begin(),definedNodes.end(),topLevel->getTopNode()->getString());
		if(it == definedNodes.end()) {
			valid = false;
			cc->reportErrorAt(topLevel->getTopNode()->getLocation(),"undefined node referenced: " + topLevel->getTopNode()->getString());
		}
		return valid;
	}
	virtual int visitBasicEvent(DFT::AST::ASTBasicEvent* basicEvent) {
		int valid = true;
		valid = ASTVisitor::visitBasicEvent(basicEvent) ? valid : false ;
		return valid;
	}
	virtual int visitGate(DFT::AST::ASTGate* gate) {
		int valid = true;

		if(!ASTDFTBuilderPass1::buildGateTest(gate)) {
			valid = false;
			cc->reportErrorAt(gate->getLocation(),"unsupported gate type: " + gate->getGateType()->getString());
		}

		valid = ASTVisitor::visitGate(gate) ? valid : false ;

		std::vector<DFT::AST::ASTIdentifier*>* children = gate->getChildren();
		for(int i=children->size();i--;) {
			std::vector<std::string>::iterator it = std::find(definedNodes.begin(),definedNodes.end(),children->at(i)->getString());
			if(it == definedNodes.end()) {
				valid = false;
				cc->reportErrorAt(children->at(i)->getLocation(),"undefined node referenced: " + children->at(i)->getString());
			}
		}
		return valid;
	}
	virtual int visitPage(DFT::AST::ASTPage* page) {
		int valid = true;

		valid = ASTVisitor::visitPage(page) ? valid : false ;

		std::vector<std::string>::iterator it = std::find(definedNodes.begin(),definedNodes.end(),page->getNodeName()->getString());
		if(it == definedNodes.end()) {
			valid = false;
			cc->reportErrorAt(page->getNodeName()->getLocation(),"undefined node referenced: " + page->getNodeName()->getString());
		}
		return valid;
	}
	virtual int visitAttrib(DFT::AST::ASTAttrib* attr) {
		int valid = true;
		valid = ASTVisitor::visitAttrib(attr) ? valid : false;
		return valid;
	}
	
	virtual int visitAttribute(DFT::AST::ASTAttribute* attribute) {
		int valid = true;
		valid = ASTVisitor::visitAttribute(attribute) ? valid : false;
		return valid;
	}
	virtual int visitAttribFloat(DFT::AST::ASTAttribFloat* af) {
		int valid = true;
		valid = ASTVisitor::visitAttribFloat(af) ? valid : false;
		return valid;
	}
	virtual int visitAttribNumber(DFT::AST::ASTAttribNumber* an) {
		int valid = true;
		valid = ASTVisitor::visitAttribNumber(an) ? valid : false;
		return valid;
	}
	virtual int visitAttribString(DFT::AST::ASTAttribString* as) {
		int valid = true;
		valid = ASTVisitor::visitAttribString(as) ? valid : false;
		return valid;
	}
};

} // Namespace: DFT

#endif // ASTVALIDATOR_H
