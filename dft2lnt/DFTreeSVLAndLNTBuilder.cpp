/*
 * DFTreeSVLAndLNTBuilder.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include "DFTreeSVLAndLNTBuilder.h"
#include "DFTreeBCGNodeBuilder.h"
#include "FileWriter.h"

#include <iostream>

DFT::DFTreeSVLAndLNTBuilder::DFTreeSVLAndLNTBuilder(std::string root, std::string tmp, std::string name, DFT::DFTree* dft, CompilerContext* cc):
	root(root),
	tmp(tmp),
	name(name),
	dft(dft),
	cc(cc) {
	
}
int DFT::DFTreeSVLAndLNTBuilder::build() {

	// Check all the nodes in the DFT, adding BasicEvents to basicEvents and
	// Gates to gates. Also keep track of what Lotos NT files are needed by
	// adding them to neededFiles.
	for(size_t i=0; i<dft->getNodes().size(); ++i) {
		DFT::Nodes::Node* node = dft->getNodes().at(i);
		neededFiles.insert( (node->getType()) );
		if(DFT::Nodes::Node::typeMatch(node->getType(),DFT::Nodes::BasicEventType)) {
			DFT::Nodes::BasicEvent* be = static_cast<DFT::Nodes::BasicEvent*>(node);
			basicEvents.push_back(be);
		} else if(DFT::Nodes::Node::typeMatch(node->getType(),DFT::Nodes::GateType)) {
			DFT::Nodes::Gate* gate = static_cast<DFT::Nodes::Gate*>(node);
			gates.push_back(gate);
		} else {
			assert(0);
		}
	}

	// Build the SVL script
	buildSVLOptions();
	buildSVLHeader();
	buildSVLBody();
	
	// Temporary: print SVL script
	std::cout << svl_options.toString();
	std::cout << svl_dependencies.toString();
	std::cout << svl_body.toString();
	
	return 0;
}

int DFT::DFTreeSVLAndLNTBuilder::buildSVLOptions() {
	svl_options.appendLine("%BCG_MIN_OPTIONS=\"-self\"");
	return 0;
}

const std::string& DFT::DFTreeSVLAndLNTBuilder::getFileForNodeType(DFT::Nodes::NodeType nodeType) {
	switch(nodeType) {
	case DFT::Nodes::BasicEventType:
		return DFT::Files::BasicEvent;
		break;
	case DFT::Nodes::GateAndType:
		return DFT::Files::GateAnd;
		break;
	case DFT::Nodes::GateOrType:
		return DFT::Files::GateOr;
		break;
	case DFT::Nodes::GateVotingType:
		return DFT::Files::GateVoting;
		break;
	default:
		return DFT::Files::Unknown;
		break;
	}
} 

int DFT::DFTreeSVLAndLNTBuilder::buildSVLHeader() {
	svl_dependencies.appendLine("");
	
	// Go though all the needed Lotos NT specication files and add them so
	// they are compiled to Lotos
	std::set<DFT::Nodes::NodeType>::iterator it = neededFiles.begin();
	for(;it != neededFiles.end(); it++) {
		const std::string& fileName = getFileForNodeType(*it);
		//svl_dependencies << svl_dependencies.applyprefix << "%lnt.open \"" << fileName << ".lnt\" -" << svl_dependencies.applypostfix;
		svl_dependencies << svl_dependencies.applyprefix << "\"$DFT2LNT_ROOT/bcgnodes/" << fileName << ".bcg\" = strong reduction of \"$DFT2LNT_ROOT/lotosnodes/" << fileName << ".lnt\";" << svl_dependencies.applypostfix;
	}
	return 0;
}

int DFT::DFTreeSVLAndLNTBuilder::buildSVLBody() {
	svl_body.appendLine("");
	
	// Specify the resulting .bcg uses smart compose
	svl_body << svl_body.applyprefix << "\"" << name << "." << DFT::FileExtensions::BCG << "\" = smart stochastic branching reduction of" << svl_body.applypostfix;

	// Hide all the internal communication
	svl_body.indent();
	svl_body.appendPrefix().append("hide ");
	{
		bool first = true;
		std::vector<DFT::Nodes::Node*>& nodes = dft->getNodes();
		for(size_t i=0; i<nodes.size(); ++i) {
			DFT::Nodes::Node* node = nodes.at(i);
			if(node != dft->getTopNode()) {
				if(!first) {
					svl_body << ", ";
				}
				svl_body << "F_" << node->getName();
				first = false;
			}
		}
	}
	svl_body.append(" in (");
	svl_body.appendPostfix();

	// Build the import commands of the BasicEvents and the Gates
	// FIXME: This should be changed to starting at the root node
	//        and traversing the DFT in DFS style.
	{
		svl_body.indent();
		int total = basicEvents.size() + gates.size();
		int current = 0;
		
		// Basic Events
		for(size_t  i=0; i<basicEvents.size(); ++i) {
			buildBasicEvent(current, total, basicEvents.at(i));
		}
		
		// Gates
		// Note that the TopNode is reserved for last.
		for(size_t  i=0; i<gates.size(); ++i) {
			if(gates.at(i) != dft->getTopNode()) {
				buildGate(current, total, gates.at(i));
			}
		}
		if(dft->getTopNode()->isBasicEvent()) {
			buildBasicEvent(current, total, static_cast<DFT::Nodes::BasicEvent*>(dft->getTopNode()));
		} else if(dft->getTopNode()->isGate()) {
			buildGate(current, total, static_cast<DFT::Nodes::Gate*>(dft->getTopNode()));
		}
		svl_body.outdent();
	}
	
	svl_body.appendLine("};");
	svl_body.outdent();
	return 0;
}

int DFT::DFTreeSVLAndLNTBuilder::buildBasicEvent(int& current, int& total, DFT::Nodes::BasicEvent* basicEvent) {
	
	// FIXME: obtain rates from DFT
	std::string dorm = "rate 5";
	
	// Add SVL script for the specified BasicEvent
	svl_body.appendPrefix();
	{
		svl_body << "(";

		// Rename from local to global, where y is the name of the BasicEvent:
		//   F !1 -> F_y
		//   A !1 -> A_y
		//   FRATE !1 !1 -> active failure rate
		//   FRATE !1 !2 -> dormant failure rate
		svl_body << "total rename ";
		svl_body << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << " !1\" -> \"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << "_" << basicEvent->getName() << "\"";
		svl_body << ", ";
		svl_body << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE << " !1\" -> \"" << DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE << "_" << basicEvent->getName() << "\"";
		svl_body << ", ";
		svl_body << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !1 !2\" -> \"" << dorm << "\" in ";
		
		// Specify the needed Lotos specification
		svl_body << "\"" << getFileForNodeType(basicEvent->getType()) << ".lotos\"";

		svl_body << ")";
	}
	svl_body.appendPostfix();
	
	++current;
	
	// If this is not the last node, add the rule to compose the processes.
	// The communication has to be synchronous on the F_ and A_ gates.
	// FIXME: Only F_ is supported at the moment
	if(current<total) {
		svl_body.indent();
		svl_body.appendPrefix();
		svl_body << "|[";
		svl_body << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << "_" << basicEvent->getName();
		svl_body << "]|";
		svl_body.appendPostfix();
		svl_body.outdent();
	}
	return 0;
}
int DFT::DFTreeSVLAndLNTBuilder::buildGate(int& current, int& total, DFT::Nodes::Gate* gate) {
	
	compileGate(gate);
	
	// Add SVL script for the specified Gate
	svl_body.appendPrefix();
	{
		svl_body << "(";

		// Rename local F !x gates to global F_y gates where y is the name of
		// the node that x connects to. Needed for synchronization.
		svl_body << "total rename ";
		for(size_t i=0; i<gate->getChildren().size(); ++i) {
			svl_body << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << " !" << (i+1) << "\" -> \"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << "_" << gate->getChildren().at(i)->getName() << "\", ";
		}
		svl_body << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << " !" << (gate->getChildren().size()+1) << "\" -> \"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << "_" << gate->getName() << "\" in ";
		
		// Specify the needed Lotos specification
		svl_body << "\"" << getFileForNodeType(gate->getType()) << ".lotos\"";

		svl_body << ")";
	}
	svl_body.appendPostfix();
	
	++current;
	
	// If this is not the last node, add the rule to compose the processes.
	// The communication has to be synchronous on the F_ and A_ gates.
	// FIXME: Only F_ is supported at the moment
	if(current<total) {
		svl_body.indent();
		svl_body.appendPrefix();
		svl_body << "|[";
		svl_body << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << "_" << gate->getName();
		svl_body << "]|";
		svl_body.appendPostfix();
		svl_body.outdent();
	}
	return 0;
}

bool DFT::DFTreeSVLAndLNTBuilder::shouldCompileBCG_1(std::string fileName) {
	FILE* fp = fopen((root + "/bcgnodes/" + fileName).c_str(),"r+");
	if(fp) {
		fclose(fp);
		return false;
	} else {
		return true;
	}
}

bool DFT::DFTreeSVLAndLNTBuilder::shouldGenerateLNT_1(std::string fileName) {
	FILE* fp = fopen((root + "/lntnodes/" + fileName).c_str(),"r+");
	if(fp) {
		fclose(fp);
		return false;
	} else {
		return true;
	}
}

int DFT::DFTreeSVLAndLNTBuilder::compileGate(DFT::Nodes::Gate* gate) {
	
	int children = gate->getChildren().size();

	std::string targetFileName;
	{
		std::stringstream out;
		out << DFT::Files::GateAnd;
		out << "_" << children;
		out << "." << DFT::FileExtensions::BCG;
		targetFileName = out.str();
	}
	std::string targetFilePath = root + "/bcgnodes/" + targetFileName;

	std::string sourceFileName;
	{
		std::stringstream out;
		out << DFT::Files::GateAnd;
		out << "_" << children;
		out << "." << DFT::FileExtensions::BCG;
		sourceFileName = out.str();
	}
	std::string sourceFilePath = root + "/lntnodes/" + sourceFileName;

	if(shouldCompileBCG_1(targetFileName)) {
		if(!shouldGenerateLNT_1(sourceFileName)) {
			FileWriter node;
			generateLNT(node,gate);
		}
		compileLNT(sourceFileName,targetFileName);
	}
	
	return 0;
}

void DFT::DFTreeSVLAndLNTBuilder::generateLNT(FileWriter& out, DFT::Nodes::Gate* gate) {
	switch(gate->getType()) {
		case DFT::Nodes::GatePhasedOrType:
		case DFT::Nodes::GateOrType:
		case DFT::Nodes::GateAndType: {
			DFT::Nodes::GateAnd* gateAnd = static_cast<DFT::Nodes::GateAnd*>(gate);
			generateLNTAnd(out,gateAnd);
			break;
		}
		case DFT::Nodes::GateHSPType:
		case DFT::Nodes::GateWSPType:
		case DFT::Nodes::GateCSPType:
		case DFT::Nodes::GatePAndType:
		case DFT::Nodes::GateSeqType:
		case DFT::Nodes::GateVotingType:
		case DFT::Nodes::GateFDEPType:
		case DFT::Nodes::GateTransferType:
		default:
			assert(0 && "not yet implemented");
			break;
	}
}

void DFT::DFTreeSVLAndLNTBuilder::generateLNTAnd(FileWriter& out, DFT::Nodes::GateAnd* gate) {
	
	int children = gate->getChildren().size();
	
	out << out.applyprefix << "module AND(VOTING_GENERIC) is";

	out.indent();
		out << out.applyprefix << "type NAT_SET is array[1.." << children << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "process MAIN [" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << " : NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "VOTING [" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << "] (" << children << " of NAT, " << children << " of NAT, (NAT_SET(FALSE";
			for(int i=1; i < children; ++i) {
				out << ", FALSE";
			}
			out << ")))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();

	out << out.applyprefix << "end module" << out.applypostfix;
}

int DFT::DFTreeSVLAndLNTBuilder::compileLNT(std::string lntFile, std::string bcgFile) {
	return 0;
}