#ifndef ASTDFTBUILDER_H
#define ASTDFTBUILDER_H

#include <vector>
#include <map>

#include "dft_ast.h"
#include "dft_parser.h"
#include "dftnodes/nodes.h"
#include "ASTVisitor.h"

namespace DFT {

class ASTDFTBuilderPass1: public DFT::ASTVisitor<int> {
private:
	std::vector<DFT::AST::ASTNode*>* ast;
	Parser* parser;
	DFT::DFTree* dft;
	std::string topNode;
	//std::map<std::string,DFT::Nodes::Node*> nodeTable;
public:
	static DFT::Nodes::Gate* buildGate(DFT::AST::ASTGate* astgate) {
		DFT::Nodes::Gate* gate = NULL;
		DFT::Nodes::NodeType gateType = astgate->getGateType()->getGateType();

		switch(gateType) {
		case DFT::Nodes::GatePhasedOrType:
			break;
		case DFT::Nodes::GateOrType:
			break;
		case DFT::Nodes::GateAndType:
			gate = new DFT::Nodes::GateAnd(astgate->getName()->getString());
			break;
		case DFT::Nodes::GateHSPType:
			break;
		case DFT::Nodes::GateWSPType:
			break;
		case DFT::Nodes::GateCSPType:
			break;
		case DFT::Nodes::GatePAndType:
			break;
		case DFT::Nodes::GateSeqType:
			break;
		case DFT::Nodes::GateOFType:
			break;
		case DFT::Nodes::GateFDEPType:
			break;
		case DFT::Nodes::GateTransferType:
			break;
		default:
			break;
		}
		return gate;
	}
	static bool buildGateTest(DFT::AST::ASTGate* astgate) {
		DFT::Nodes::Gate* gate = buildGate(astgate);
		if(gate) {
			delete gate;
			return true;
		}
		return false;
	}
	ASTDFTBuilderPass1(std::vector<DFT::AST::ASTNode*>* ast, CompilerContext* cc, DFT::DFTree* dft):
		ASTVisitor(ast,cc,[](int& ret, int val) {}),
		dft(dft),
		topNode("") {
	}

	void build() {
		ASTVisitor::visit();
	}
	
	virtual int visitTopLevel(DFT::AST::ASTTopLevel* topLevel) {
		
		topNode = topLevel->getTopNode()->getString();
		
		ASTVisitor::visitTopLevel(topLevel);
	}
	virtual int visitBasicEvent(DFT::AST::ASTBasicEvent* basicEvent) {
		
		DFT::Nodes::BasicEvent* be = new DFT::Nodes::BasicEvent(basicEvent->getName()->getString());
		//nodeTable.insert( pair<std::string,DFT::Nodes::Node*>(basicEvent->getName()->getString(),be) );
		dft->addNode(be);
		ASTVisitor::visitBasicEvent(basicEvent);
	}
	virtual int visitGate(DFT::AST::ASTGate* astgate) {
		DFT::Nodes::Gate* gate = buildGate(astgate);

		if(!gate) {
			cc->reportError("ASTDFTBuilder does not support this Gate yet: " + astgate->getGateType()->getString());
			return 0;
		}

		//nodeTable.insert( pair<std::string,DFT::Nodes::Node*>(astgate->getName()->getString(),n_gate) );
		dft->addNode(gate);

		ASTVisitor::visitGate(astgate);
	}
	virtual int visitPage(DFT::AST::ASTPage* page) {
		ASTVisitor::visitPage(page);
	}
	virtual int visitAttribute(DFT::AST::ASTAttribute* attribute) {
		ASTVisitor::visitAttribute(attribute);
	}
	virtual int visitAttrib(DFT::AST::ASTAttrib* attrib) {
		ASTVisitor::visitAttrib(attrib);
	}
	virtual int visitAttribFloat(DFT::AST::ASTAttribFloat* af) {
		ASTVisitor::visitAttribFloat(af);
	}
	virtual int visitAttribNumber(DFT::AST::ASTAttribNumber* an) {
		ASTVisitor::visitAttribNumber(an);
	}
	virtual int visitAttribString(DFT::AST::ASTAttribString* as) {
		ASTVisitor::visitAttribString(as);
	}
};

class ASTDFTBuilder: public DFT::ASTVisitor<int> {
private:
	std::vector<DFT::AST::ASTNode*>* ast;
	Parser* parser;
	DFT::DFTree* dft;
	std::string topNode;
public:

	ASTDFTBuilder(std::vector<DFT::AST::ASTNode*>* ast, CompilerContext* cc):
		ASTVisitor(ast,cc,[](int& ret, int val){}),
		ast(ast),
		parser(parser),
		dft(NULL),
		topNode("") {
	}

	DFT::DFTree* build() {
		dft = new DFT::DFTree();
		ASTDFTBuilderPass1* pass1 = new ASTDFTBuilderPass1(ast,cc,dft);
		pass1->build();
		ASTVisitor::visit();
		delete pass1;
		return dft;
	}
	
	virtual int visitTopLevel(DFT::AST::ASTTopLevel* topLevel) {
		DFT::Nodes::Node* top = dft->getNode(topLevel->getTopNode()->getString());
		assert(top);
		dft->setTopNode(top);
		ASTVisitor::visitTopLevel(topLevel);
	}
	virtual int visitBasicEvent(DFT::AST::ASTBasicEvent* basicEvent) {
		DFT::Nodes::Node* n = dft->getNode(basicEvent->getName()->getString());
		assert(n);
		assert(n->getType()==DFT::Nodes::BasicEventType);
		DFT::Nodes::BasicEvent* be = static_cast<DFT::Nodes::BasicEvent*>(n);
		ASTVisitor::visitBasicEvent(basicEvent);
	}
	virtual int visitGate(DFT::AST::ASTGate* gate) {
		DFT::Nodes::Node* n = dft->getNode(gate->getName()->getString());
		assert(n);
		assert(DFT::Nodes::Node::typeMatch(n->getType(),DFT::Nodes::GateType));
		DFT::Nodes::Gate* g = static_cast<DFT::Nodes::Gate*>(n);
		std::vector<DFT::Nodes::Node*>& nodes = g->getChildren();
		for(int i=0; i<gate->getChildren()->size(); ++i) {
			DFT::Nodes::Node* node = dft->getNode(gate->getChildren()->at(i)->getString());
			nodes.push_back(node);
		}
		ASTVisitor::visitGate(gate);
	}
	virtual int visitPage(DFT::AST::ASTPage* page) {
		ASTVisitor::visitPage(page);
	}
	virtual int visitAttribute(DFT::AST::ASTAttribute* attribute) {
		ASTVisitor::visitAttribute(attribute);
	}
	virtual int visitAttrib(DFT::AST::ASTAttrib* attrib) {
		ASTVisitor::visitAttrib(attrib);
	}
	virtual int visitAttribFloat(DFT::AST::ASTAttribFloat* af) {
		ASTVisitor::visitAttribFloat(af);
	}
	virtual int visitAttribNumber(DFT::AST::ASTAttribNumber* an) {
		ASTVisitor::visitAttribNumber(an);
	}
	virtual int visitAttribString(DFT::AST::ASTAttribString* as) {
		ASTVisitor::visitAttribString(as);
	}
};

} // Namespace: DFT

#endif // ASTDFTBUILDER_H
