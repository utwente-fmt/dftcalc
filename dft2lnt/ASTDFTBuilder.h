/*
 * ASTDFTBuilder.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#ifndef ASTDFTBUILDER_H
#define ASTDFTBUILDER_H

#include <vector>
#include <map>

#include "dft_ast.h"
#include "dft_parser.h"
#include "dftnodes/nodes.h"
#include "ASTVisitor.h"

namespace DFT {

/**
 * This class handles the first AST pass.
 * It will create empty instances of the needed Nodes.
 */
class ASTDFTBuilderPass1: public DFT::ASTVisitor<int> {
private:
	DFT::AST::ASTNodes* ast;
	Parser* parser;
	DFT::DFTree* dft;
	std::string topNode;
	//std::map<std::string,DFT::Nodes::Node*> nodeTable;
	static void f_aggregate(int& result, int value) {
	}
	
public:

	/**
	 * Returns a new Gate instance associated with the specified ASTGate
	 * class. Returns NULL if the ASTGate is not supported.
	 * The method buildGateTest() provides a way of testing is an ASTGate is
	 * supported without returning a new Gate.
	 * @param astgate The returned DFT Node will be based on this ASTGate.
	 * @return The DFT Node based on teh specified ASTGate.
	 */
	static DFT::Nodes::Gate* buildGate(DFT::AST::ASTGate* astgate) {
		DFT::Nodes::Gate* gate = NULL;
		DFT::Nodes::NodeType gateType = astgate->getGateType()->getGateType();

		switch(gateType) {
		case DFT::Nodes::GatePhasedOrType:
			break;
		case DFT::Nodes::GateOrType:
			gate = new DFT::Nodes::GateOr(astgate->getLocation(), astgate->getName()->getString());
			break;
		case DFT::Nodes::GateAndType:
			gate = new DFT::Nodes::GateAnd(astgate->getLocation(), astgate->getName()->getString());
			break;
		case DFT::Nodes::GateHSPType:
		case DFT::Nodes::GateWSPType:
		case DFT::Nodes::GateCSPType:
			gate = new DFT::Nodes::GateWSP(astgate->getLocation(), astgate->getName()->getString());
			break;
		case DFT::Nodes::GatePAndType:
			gate = new DFT::Nodes::GatePAnd(astgate->getLocation(), astgate->getName()->getString());
			break;
		case DFT::Nodes::GateSeqType:
			break;
		case DFT::Nodes::GateVotingType: {
			DFT::AST::ASTVotingGateType* vote = static_cast<DFT::AST::ASTVotingGateType*>(astgate->getGateType());
			gate = new DFT::Nodes::GateVoting(astgate->getLocation(), astgate->getName()->getString(),vote->getThreshold(),vote->getTotal());
			break;
		}
		case DFT::Nodes::GateFDEPType:
			gate = new DFT::Nodes::GateFDEP(astgate->getLocation(), astgate->getName()->getString());
			break;
		case DFT::Nodes::RepairUnitType:
			gate = new DFT::Nodes::RepairUnit(astgate->getLocation(), astgate->getName()->getString());
			break;
		case DFT::Nodes::GateTransferType:
			break;
		default:
			break;
		}
		return gate;
	}
	
	/**
	 * Checks if the specified ASTGate is supported. If it is supported,
	 * the method buildGate() should return a valid Gate instance for it.
	 * Returns true if the specified ASTGate is supported, false otherwise.
	 */
	static bool buildGateTest(DFT::AST::ASTGate* astgate) {
		DFT::Nodes::Gate* gate = buildGate(astgate);
		if(gate) {
			delete gate;
			return true;
		}
		return false;
	}
	
	/**
	 * Constructs a new ASTDFTBuilderPass1 instance using the specified
	 * AST and CompilerContext. New nodes will be added to the specified DFTree.
	 * Call build() to start the first AST pass of the DFT build process.
	 */
	ASTDFTBuilderPass1(DFT::AST::ASTNodes* ast, CompilerContext* cc, DFT::DFTree* dft):
		ASTVisitor<int>(ast,cc,&f_aggregate),
		dft(dft),
		topNode("") {
	}

	/**
	 * Start the first AST pass of the DFT build process.
	 */
	void build() {
		ASTVisitor<int>::visit();
	}
	
	virtual int visitTopLevel(DFT::AST::ASTTopLevel* topLevel) {
		
		topNode = topLevel->getTopNode()->getString();
		
		ASTVisitor<int>::visitTopLevel(topLevel);
	}
	virtual int visitBasicEvent(DFT::AST::ASTBasicEvent* basicEvent) {
		
		DFT::Nodes::BasicEvent* be = new DFT::Nodes::BasicEvent(basicEvent->getLocation(), basicEvent->getName()->getString());
		//nodeTable.insert( pair<std::string,DFT::Nodes::Node*>(basicEvent->getName()->getString(),be) );
		dft->addNode(be);
		ASTVisitor<int>::visitBasicEvent(basicEvent);
		
		DFT::Nodes::BE::CalculationMode calcMode = DFT::Nodes::BE::CalculationMode::UNDEFINED;
		
		// Find lambda
		{
			std::vector<DFT::AST::ASTAttribute*>::iterator it = basicEvent->getAttributes()->begin();
			for(; it!=basicEvent->getAttributes()->end(); ++it) {
				if((*it)->getLabel()==DFT::Nodes::BE::AttrLabelLambda) {
					if(calcMode==DFT::Nodes::BE::CalculationMode::EXPONENTIAL) {
						cc->reportErrorAt((*it)->getLocation(),"setting lambda would override previous calculation mode: " + DFT::Nodes::BE::getCalculationModeStr(calcMode));
					} else if(calcMode!=DFT::Nodes::BE::CalculationMode::UNDEFINED) {
						cc->reportWarningAt((*it)->getLocation(),"setting lambda twice, ignoring");
					} else {
						double v = (*it)->getValue()->getFloatValue();
						be->setLambda(v);
						calcMode = DFT::Nodes::BE::CalculationMode::EXPONENTIAL;
					}
				}
			}
		}
		
		// Find dorm
		{
			std::vector<DFT::AST::ASTAttribute*>::iterator it = basicEvent->getAttributes()->begin();
			for(; it!=basicEvent->getAttributes()->end(); ++it) {
				if((*it)->getLabel()==DFT::Nodes::BE::AttrLabelDorm) {
					double v = (*it)->getValue()->getFloatValue();
					be->setMu(be->getLambda()*v);
				}
			}
		}
		
		// Find repair
		{
			std::vector<DFT::AST::ASTAttribute*>::iterator it = basicEvent->getAttributes()->begin();
			for(; it!=basicEvent->getAttributes()->end(); ++it) {
				if((*it)->getLabel()==DFT::Nodes::BE::AttrLabelRepair) {
					double v = (*it)->getValue()->getFloatValue();
					be->setRepair(v);
					be->setRepairable(true);
				}
			}
		}

		// Find embedded distribution
		{
			std::vector<DFT::AST::ASTAttribute*>::iterator it = basicEvent->getAttributes()->begin();
			for(; it!=basicEvent->getAttributes()->end(); ++it) {
				if((*it)->getLabel()==DFT::Nodes::BE::AttrLabelAph) {
					if(calcMode==DFT::Nodes::BE::CalculationMode::APH) {
						cc->reportErrorAt((*it)->getLocation(),"setting phase type distribution would override previous calculation mode: " + DFT::Nodes::BE::getCalculationModeStr(calcMode));
					} else if(calcMode!=DFT::Nodes::BE::CalculationMode::UNDEFINED) {
						cc->reportWarningAt((*it)->getLocation(),"setting phase type distribition twice, ignoring");
					} else {
						std::string v = (*it)->getValue()->getStringValue()->getString();
                                        	File fileToEmbed(v);
						//std::cerr << "aph orig file='" << fileToEmbed.getFileRealPath() << "'" << std::endl;
						File dftFile((*it)->getLocation().getFileName());
                                                fileToEmbed.insertPathToPrefix(dftFile.getPathTo());
						be->setFileToEmbed(fileToEmbed.getFileRealPath());
						//std::cerr << "aph ext-file='" << fileToEmbed.getFileRealPath() << "'" << std::endl;
						calcMode = DFT::Nodes::BE::CalculationMode::APH;
					}
				}
			}
		}
		
		// Find rate
		{
			std::vector<DFT::AST::ASTAttribute*>::iterator it = basicEvent->getAttributes()->begin();
			for(; it!=basicEvent->getAttributes()->end(); ++it) {
				if((*it)->getLabel()==DFT::Nodes::BE::AttrLabelRate) {
					if(calcMode==DFT::Nodes::BE::CalculationMode::WEIBULL) {
						cc->reportErrorAt((*it)->getLocation(),"setting rate would override previous calculation mode: " + DFT::Nodes::BE::getCalculationModeStr(calcMode));
					} else if(calcMode!=DFT::Nodes::BE::CalculationMode::UNDEFINED) {
						cc->reportWarningAt((*it)->getLocation(),"setting rate twice, ignoring");
					} else {
						float v = (*it)->getValue()->getFloatValue();
						be->setRate(v);
						calcMode = DFT::Nodes::BE::CalculationMode::WEIBULL;
					}
				}
			}
		}
		
		// Find shape
		{
			std::vector<DFT::AST::ASTAttribute*>::iterator it = basicEvent->getAttributes()->begin();
			for(; it!=basicEvent->getAttributes()->end(); ++it) {
				if((*it)->getLabel()==DFT::Nodes::BE::AttrLabelShape) {
					int v = (*it)->getValue()->getNumberValue();
					be->setShape(v);
				}
			}
		}
		
		be->setMode(calcMode);
	}
	virtual int visitGate(DFT::AST::ASTGate* astgate) {
		DFT::Nodes::Gate* gate = buildGate(astgate);

		if(!gate) {
			cc->reportError("ASTDFTBuilder does not support this Gate yet: " + astgate->getGateType()->getString());
			return 0;
		}

		//nodeTable.insert( pair<std::string,DFT::Nodes::Node*>(astgate->getName()->getString(),n_gate) );
		dft->addNode(gate);

		ASTVisitor<int>::visitGate(astgate);
	}
	virtual int visitPage(DFT::AST::ASTPage* page) {
		ASTVisitor<int>::visitPage(page);
	}
	virtual int visitAttribute(DFT::AST::ASTAttribute* attribute) {
		//assert(buildingBE && "visitAttribute called, without a BE being built");

		DFT::AST::ASTAttrib* value = attribute->getValue();

		ASTVisitor<int>::visitAttribute(attribute);
		return 0;
	}
	virtual int visitAttrib(DFT::AST::ASTAttrib* attrib) {
		ASTVisitor<int>::visitAttrib(attrib);
	}
	virtual int visitAttribFloat(DFT::AST::ASTAttribFloat* af) {
		ASTVisitor<int>::visitAttribFloat(af);
	}
	virtual int visitAttribNumber(DFT::AST::ASTAttribNumber* an) {
		ASTVisitor<int>::visitAttribNumber(an);
	}
	virtual int visitAttribString(DFT::AST::ASTAttribString* as) {
		ASTVisitor<int>::visitAttribString(as);
	}
};

/**
 * This class handles the building of a DFT from an AST.
 * It uses a two-pass system. The ASTDFTBuilderPass1 class handles the first
 * pass. The first pass consists of creating the nodes and the second pass
 * consists of connecting the nodes.
 */
class ASTDFTBuilder: public DFT::ASTVisitor<int> {
private:
	DFT::AST::ASTNodes* ast;
	Parser* parser;
	DFT::DFTree* dft;
	std::string topNode;
	static void f_aggregate(int& result, int value) {
	}
public:

	/**
	 * Constructs a new ASTDFTBuilder instance using the specified
	 * AST and CompilerContext.
	 * Call build() to start the DFT build process.
	 */
	ASTDFTBuilder(DFT::AST::ASTNodes* ast, CompilerContext* cc):
		ASTVisitor<int>(ast,cc,&f_aggregate),
		ast(ast),
		parser(parser),
		dft(NULL),
		topNode("") {
	}

	/**
	 */
	 virtual void aggregate(int& result, const int& value) {
	 }

	/**
	 * Starts the AST to DFT build process using the AST given to the
	 * constructor.
	 * Returns a new DFTree instance describing the DFT created from the AST.
	 */
	DFT::DFTree* build() {
		dft = new DFT::DFTree();
		ASTDFTBuilderPass1* pass1 = new ASTDFTBuilderPass1(ast,cc,dft);
		pass1->build();
		ASTVisitor<int>::visit();
		delete pass1;
		//dft->transformFDEPNodes();
		return dft;
	}
	
	virtual int visitTopLevel(DFT::AST::ASTTopLevel* topLevel) {
		DFT::Nodes::Node* top = dft->getNode(topLevel->getTopNode()->getString());
		assert(top);
		dft->setTopNode(top);
		ASTVisitor<int>::visitTopLevel(topLevel);
	}
	virtual int visitBasicEvent(DFT::AST::ASTBasicEvent* basicEvent) {
		DFT::Nodes::Node* n = dft->getNode(basicEvent->getName()->getString());
		assert(n);
		assert(n->getType()==DFT::Nodes::BasicEventType);
		DFT::Nodes::BasicEvent* be = static_cast<DFT::Nodes::BasicEvent*>(n);
		ASTVisitor<int>::visitBasicEvent(basicEvent);
	}
	virtual int visitGate(DFT::AST::ASTGate* gate) {
		DFT::Nodes::Node* n = dft->getNode(gate->getName()->getString());
		assert(n);
		assert(DFT::Nodes::Node::typeMatch(n->getType(),DFT::Nodes::GateType));
		
		DFT::Nodes::GateFDEP* g = static_cast<DFT::Nodes::GateFDEP*>(n);
		if(DFT::Nodes::Node::typeMatch(n->getType(),DFT::Nodes::GateFDEPType)) {
			
			// Add the trigger as a child
			DFT::Nodes::Node* node = dft->getNode(gate->getChildren()->at(0)->getString());
			node->getParents().push_back(n);
			g->getChildren().push_back(node);
			
			// Add the rest of the children as dependers
			for(int i=1; i<gate->getChildren()->size(); ++i) {
				DFT::Nodes::Node* node = dft->getNode(gate->getChildren()->at(i)->getString());
				g->getDependers().push_back(node);
			}
		} else {
			// Fix the parent-child relationship for this Gate and its children
			for(int i=0; i<gate->getChildren()->size(); ++i) {
				
				// Get a child of the Gate
				DFT::Nodes::Node* node = dft->getNode(gate->getChildren()->at(i)->getString());
				
				// Add the Gate to that child's list of parents
				node->getParents().push_back(n);
				
				// Add the child to the Gate's list of children
				g->getChildren().push_back(node);
			}
		}
		ASTVisitor<int>::visitGate(gate);
	}
	virtual int visitPage(DFT::AST::ASTPage* page) {
		ASTVisitor<int>::visitPage(page);
	}
	virtual int visitAttribute(DFT::AST::ASTAttribute* attribute) {
		ASTVisitor<int>::visitAttribute(attribute);
	}
	virtual int visitAttrib(DFT::AST::ASTAttrib* attrib) {
		ASTVisitor<int>::visitAttrib(attrib);
	}
	virtual int visitAttribFloat(DFT::AST::ASTAttribFloat* af) {
		ASTVisitor<int>::visitAttribFloat(af);
	}
	virtual int visitAttribNumber(DFT::AST::ASTAttribNumber* an) {
		ASTVisitor<int>::visitAttribNumber(an);
	}
	virtual int visitAttribString(DFT::AST::ASTAttribString* as) {
		ASTVisitor<int>::visitAttribString(as);
	}
};

} // Namespace: DFT

#endif // ASTDFTBUILDER_H
