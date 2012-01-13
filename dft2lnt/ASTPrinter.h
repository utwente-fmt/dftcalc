#ifndef ASTPRINTER_H
#define ASTPRINTER_H

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
class ASTPrinter: public DFT::ASTVisitor<int,true> {
private:
	FileWriter out;
public:

	/**
	 * Constructs a new ASTPrinter using the specified
	 * AST and CompilerContext.
	 * Call validate() to start the validation process.
	 */
	ASTPrinter(std::vector<DFT::AST::ASTNode*>* ast, CompilerContext* cc):
		//ASTVisitor(ast,cc,[](int& ret, int val){ret = ret && val;}) {
		ASTVisitor<int,true>(ast,cc,NULL) {
	}

	/**
	 */
	 virtual void aggregate(int& result, const int& value) {
		 result = result + value;
	 }

	/**
	 * Starts the validation process of the AST specified in the
	 * constructor. Returns whether the AST is deemed valid or not.
	 * Returns true if it is valid, false otherewise.
	 */
	std::string print() {
		
		int valid = true;
		
		valid = ASTVisitor<int,true>::visit() ? valid : false;
		
		return out.toString();
	}
	
	virtual int visitTopLevel(DFT::AST::ASTTopLevel* topLevel) {
		int valid = true;
		
		out << out.applyprefix << "TopLevel \"" << topLevel->getTopNode()->getString() << "\"" << out.applypostfix;
		
		valid = ASTVisitor<int,true>::visitTopLevel(topLevel) ? valid : false ;
		
		return valid;
	}
	virtual int visitBasicEvent(DFT::AST::ASTBasicEvent* basicEvent) {
		int valid = true;
		
		out << out.applyprefix << "BasicEvent \"" << basicEvent->getName()->getString() << "\"" << out.applypostfix;
		out.indent();
		valid = ASTVisitor<int,true>::visitBasicEvent(basicEvent) ? valid : false ;
		out.outdent();
		
		return valid;
	}
	virtual int visitGate(DFT::AST::ASTGate* gate) {
		int valid = true;
		
//		if(!ASTDFTBuilderPass1::buildGateTest(gate)) {
//			valid = false;
//			cc->reportErrorAt(gate->getLocation(),"unsupported gate type: " + gate->getGateType()->getString());
//		}
		
		out << out.applyprefix << "Gate[" << gate->getGateType()->getString() << "] \"" << gate->getName()->getString() << "\"" << out.applypostfix;
		out.indent();
		valid = ASTVisitor<int,true>::visitGate(gate) ? valid : false ;
		out.outdent();
		
//		std::vector<DFT::AST::ASTIdentifier*>* children = gate->getChildren();
//		for(int i=children->size();i--;) {
//			std::vector<std::string>::iterator it = std::find(definedNodes.begin(),definedNodes.end(),children->at(i)->getString());
//			if(it == definedNodes.end()) {
//				valid = false;
//				cc->reportErrorAt(children->at(i)->getLocation(),"undefined node referenced: " + children->at(i)->getString());
//			}
//		}
		return valid;
	}
	virtual int visitPage(DFT::AST::ASTPage* page) {
		int valid = true;
		
		out << out.applyprefix << "Page " << page->getPage() << ": \"" << page->getNodeName()->getString() << "\"" << out.applypostfix;
		
		valid = ASTVisitor<int,true>::visitPage(page) ? valid : false ;
		
//		std::vector<std::string>::iterator it = std::find(definedNodes.begin(),definedNodes.end(),page->getNodeName()->getString());
//		if(it == definedNodes.end()) {
//			valid = false;
//			cc->reportErrorAt(page->getNodeName()->getLocation(),"undefined node referenced: " + page->getNodeName()->getString());
//		}
		return valid;
	}
	virtual int visitAttrib(DFT::AST::ASTAttrib* attr) {
		int valid = true;
		valid = ASTVisitor<int,true>::visitAttrib(attr) ? valid : false;
		return valid;
	}
	
	virtual int visitAttribute(DFT::AST::ASTAttribute* attribute) {
		int valid = true;
		DFT::AST::ASTAttrib* value = attribute->getValue();

		out << out.applyprefix << attribute->getString() << " = ";
		valid = ASTVisitor<int,true>::visitAttrib(value) ? valid : false;
		out << out.applypostfix;

		switch(attribute->getLabel()) {
			case DFT::Nodes::BE::AttrLabelLambda:
				break;
			case DFT::Nodes::BE::AttrLabelDorm:
				break;
			case DFT::Nodes::BE::AttrLabelAph:

			case DFT::Nodes::BE::AttrLabelProb:

			case DFT::Nodes::BE::AttrLabelRate:
			case DFT::Nodes::BE::AttrLabelShape:
			case DFT::Nodes::BE::AttrLabelMean:
			case DFT::Nodes::BE::AttrLabelStddev:
			case DFT::Nodes::BE::AttrLabelCov:
			case DFT::Nodes::BE::AttrLabelRes:
			case DFT::Nodes::BE::AttrLabelRepl:

			case DFT::Nodes::BE::AttrLabelNone:
			case DFT::Nodes::BE::AttrLabelOther:
			default:
				//valid = false;
				break;
		}
		
		return valid;
	}
	virtual int visitAttribFloat(DFT::AST::ASTAttribFloat* af) {
		int valid = true;
		out << af->getValue() << " (float)";
		valid = ASTVisitor<int,true>::visitAttribFloat(af) ? valid : false;
		return valid;
	}
	virtual int visitAttribNumber(DFT::AST::ASTAttribNumber* an) {
		int valid = true;
		out << an->getValue() << " (int)";
		valid = ASTVisitor<int,true>::visitAttribNumber(an) ? valid : false;
		return valid;
	}
	virtual int visitAttribString(DFT::AST::ASTAttribString* as) {
		int valid = true;
		out << as->getValue()->getString() << " (string)";
		valid = ASTVisitor<int,true>::visitAttribString(as) ? valid : false;
		return valid;
	}
};

} // Namespace: DFT

#endif // ASTPRINTER_H
