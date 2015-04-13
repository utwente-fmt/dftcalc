/*
 * DFTreeEXPBuilder.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg and extended by Dennis Guck
 */

#include "DFTreeBCGNodeBuilder.h"
#include "DFTreeEXPBuilder.h"
#include "FileWriter.h"
#include "dft2lnt.h"

#include <map>
#include <fstream>

static const int VERBOSITY_FLOW = 1;
static const int VERBOSITY_RULES = 2;
static const int VERBOSITY_RULEORIGINS = 3;

std::ostream& operator<<(std::ostream& stream, const DFT::EXPSyncItem& item) {
	bool first = true;
	for(int a: item.args) {
		if(first) first = false;
		else stream << ",";
		stream << a;
	}
	return stream;
}

void DFT::DFTreeEXPBuilder::printSyncLineShort(std::ostream& stream, const EXPSyncRule& rule) {
	stream << "< ";
	bool first = true;
	for(auto syncIdx: rule.label) {
		if(first) first = false;
		else stream << " | ";
		const DFT::Nodes::Node* node = getNodeWithID(syncIdx.first);
		if(node)
			stream << node->getName();
		else
			stream << "error";
		stream << ":";
		stream << *syncIdx.second;
	}
	stream << " > @ " << (rule.syncOnNode?rule.syncOnNode->getName():"NOSYNC") << " -> " << rule.toLabel;
}

std::string DFT::DFTreeEXPBuilder::getBEProc(const DFT::Nodes::BasicEvent& be) const {
	std::stringstream ss;

	if(be.getMode() == DFT::Nodes::BE::CalculationMode::APH) {
		ss << "total rename ";
		ss << "\"ACTIVATE\" -> \"" << DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE << " !0 !FALSE\"";
		ss << ", ";
		ss << "\"FAIL\" -> \"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << " !0\"";
		//ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE << " !0 !FALSE\" -> \"A\"";
		//ss << ", ";
		//ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << " !0\" -> \"F\"";
		ss << " in \"";
		ss << be.getFileToEmbed();
		ss << "\" end rename";
	} else if(be.getLambda()>0){
		ss << "total rename ";
		// Insert lambda value
		ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !1 !2\" -> \"rate " << be.getLambda() << "\"";
	
		// Insert mu value (only for non-cold BE's)
		if(be.getMu()>0) {
			ss << ", ";
			ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !1 !1\" -> \"rate " << be.getMu()     << "\"";
		}
        if(be.getMaintain()>0) {
            ss << ", ";
            ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !1 !4\" -> \"rate " << be.getMaintain()     << "\"";
            if(be.getMu()>0) {
                ss << ", ";
                ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !1 !3\" -> \"rate " << be.getMaintain() << "\"";
            }
        }
		ss << " in \"";
		ss << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(be);
		ss << ".bcg\" end rename";
	} else {
		ss << "\"";
		ss << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(be);
		ss << ".bcg\"";
    }
	return ss.str();
}

std::string DFT::DFTreeEXPBuilder::getRUProc(const DFT::Nodes::Gate& ru) const {
	std::stringstream ss;

	ss << "total rename ";

	for(size_t n = 0; n<ru.getChildren().size(); ++n) {

		// Get the current child and associated childID
		const DFT::Nodes::Node& child = *ru.getChildren().at(n);

		if(child.isBasicEvent()) {
			const DFT::Nodes::BasicEvent& be = *static_cast<const DFT::Nodes::BasicEvent*>(&child);

			// Insert repair values
			ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_REPAIR << " !1 !" << n+1 << "\" -> \"rate " << be.getRepair() << "\"";
			if(n < ru.getChildren().size()-1){
				ss << ", ";
			}
		}

	}

	ss << " in \"";
	ss << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(ru);
	ss << ".bcg\" end rename";

	return ss.str();
}

std::string DFT::DFTreeEXPBuilder::getINSPProc(const DFT::Nodes::Inspection& insp) const {
    std::stringstream ss;
    
    ss << "total rename ";
    // Insert lambda value
    ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_INSPECTION << " !1" << "\" -> \"rate " << insp.getLambda() << "\"";
    
    ss << " in \"";
    ss << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(insp);
    ss << ".bcg\" end rename";
    
    return ss.str();
}

std::string DFT::DFTreeEXPBuilder::getREPProc(const DFT::Nodes::Replacement& rep) const {
    std::stringstream ss;
    
    ss << "total rename ";
    // Insert lambda value
    ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_PERIOD << " !1" << "\" -> \"rate " << rep.getLambda() << "\"";
    
    ss << " in \"";
    ss << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(rep);
    ss << ".bcg\" end rename";
    
    return ss.str();
}

void DFT::DFTreeEXPBuilder::printSyncLine(const EXPSyncRule& rule, const vector<unsigned int>& columnWidths) {
	std::map<unsigned int,EXPSyncItem*>::const_iterator it = rule.label.begin();
	size_t c=0;
	for(; it!=rule.label.end();++it) {
		while(c<it->first) {
			if(c>0) exp_body << " * ";
			exp_body.outlineLeftNext(columnWidths[c],' ');
			exp_body << "_";
			++c;
		}
		if(c>0) exp_body << " * ";
		exp_body.outlineLeftNext(columnWidths[c],' ');
		exp_body << it->second->toStringQuoted();
		++c;
	}
	while(c<dft->getNodes().size()) {
		if(c>0) exp_body << " * ";
		exp_body.outlineLeftNext(columnWidths[c],' ');
		exp_body << "_";
		++c;
	}
	exp_body << " -> " << rule.toLabel;
}

DFT::DFTreeEXPBuilder::DFTreeEXPBuilder(std::string root, std::string tmp, std::string nameBCG, std::string nameEXP, DFT::DFTree* dft, CompilerContext* cc):
	root(root),
	bcgRoot(root+DFT2LNT::BCGSUBROOT+"/"),
	tmp(tmp),
	nameBCG(nameBCG),
	nameEXP(nameEXP),
	nameTop(""),
	dft(dft),
	cc(cc) {
	
}
int DFT::DFTreeEXPBuilder::build() {

	bool ok = true;
	
	// Check all the nodes in the DFT, adding BasicEvents to basicEvents and
	// Gates to gates. Also keep track of what Lotos NT files are needed by
	// adding them to neededFiles.
	basicEvents.clear();
	gates.clear();
	nodeIDs.clear();
	for(size_t i=0; i<dft->getNodes().size(); ++i) {
		DFT::Nodes::Node* node = dft->getNodes().at(i);
		if(DFT::Nodes::Node::typeMatch(node->getType(),DFT::Nodes::BasicEventType)) {
			DFT::Nodes::BasicEvent* be = static_cast<DFT::Nodes::BasicEvent*>(node);
			basicEvents.push_back(be);
		} else if(DFT::Nodes::Node::typeMatch(node->getType(),DFT::Nodes::GateType)) {
			DFT::Nodes::Gate* gate = static_cast<DFT::Nodes::Gate*>(node);
			gates.push_back(gate);
		} else {
			cc->reportErrorAt(node->getLocation(),"DFTreeEXPBuilder cannot handles this node");
			ok = false;
		}
		nodeIDs.insert( pair<const DFT::Nodes::Node*, unsigned int>(node,i) );
	}
	
	if(ok) {
		// Build the EXP file
		svl_header.clearAll();
		svl_body.clearAll();
		exp_header.clearAll();
		exp_body.clearAll();
		
		vector<DFT::EXPSyncRule*> activationRules;
		vector<DFT::EXPSyncRule*> failRules;
		// new rules for repair, repaired and online
		vector<DFT::EXPSyncRule*> repairRules;
		vector<DFT::EXPSyncRule*> repairedRules;
		vector<DFT::EXPSyncRule*> repairingRules;
		vector<DFT::EXPSyncRule*> onlineRules;
        // extra inspection rules
        vector<DFT::EXPSyncRule*> inspectionRules;
        vector<DFT::EXPSyncRule*> inspectedRules;
        // reset rules
        vector<DFT::EXPSyncRule*> resetRules;
		

		//parseDFT(activationRules,failRules);
		//buildEXPHeader(activationRules,failRules);
		//buildEXPBody(activationRules,failRules);
		
        //parseDFT(activationRules,failRules,repairRules,repairedRules,repairingRules,onlineRules);
		//buildEXPHeader(activationRules,failRules,repairRules,repairedRules,onlineRules);
		//buildEXPBody(activationRules,failRules,repairRules,repairedRules,repairingRules,onlineRules);
        
        parseDFT(activationRules,failRules,repairRules,repairedRules,repairingRules,onlineRules,inspectionRules,inspectedRules,resetRules);
        buildEXPHeader(activationRules,failRules,repairRules,repairedRules,onlineRules,inspectionRules,inspectedRules,resetRules);
        buildEXPBody(activationRules,failRules,repairRules,repairedRules,repairingRules,onlineRules,inspectionRules,inspectedRules,resetRules);

		
		// Build SVL file
		svl_body << "% BCG_MIN_OPTIONS=\"-self\";" << svl_body.applypostfix;
		svl_body << "\"" << nameBCG << "\" = smart stochastic branching reduction of \"" << nameEXP << "\";" << svl_body.applypostfix;
		svl_body << "% BCG_MIN_OPTIONS=\"\"" << svl_body.applypostfix;
		
	}
	return !ok;
}

void DFT::DFTreeEXPBuilder::printEXP(std::ostream& out) {
	out << exp_header.toString();
	out << exp_body.toString();
}

void DFT::DFTreeEXPBuilder::printSVL(std::ostream& out) {
	out << svl_header.toString();
	out << svl_body.toString();
}

unsigned int DFT::DFTreeEXPBuilder::getIDOfNode(const DFT::Nodes::Node& node) const {

	std::map<const DFT::Nodes::Node*, unsigned int>::const_iterator it = nodeIDs.find(&node);
	assert( (it != nodeIDs.end()) && "getIDOfNode() did not know the ID of the node");
	
	return it->second;
//	for(size_t i=0; i<nodes.size();++i) {
//		//std::remove(notDefinedButReferencedNodes.begin(),notDefinedButReferencedNodes.end(),nodes.at(i));
//		std::vector<Nodes::Node*>::iterator it = std::find(notDefinedButReferencedNodes.begin(),notDefinedButReferencedNodes.end(),nodes.at(i));
//		if(it != notDefinedButReferencedNodes.end()) notDefinedButReferencedNodes.erase(it);
//	}
}

int DFT::DFTreeEXPBuilder::getLocalIDOfNode(const DFT::Nodes::Node* parent, const DFT::Nodes::Node* child) const {
	if(parent->isGate()) {
		const DFT::Nodes::Gate* gate = static_cast<const DFT::Nodes::Gate*>(parent);
		int i=(int)gate->getChildren().size(); 
		for(;i--;) {
			if(gate->getChildren()[i]==child) {
				return i;
			}
		}
	}
	return -1;
}

const DFT::Nodes::Node* DFT::DFTreeEXPBuilder::getNodeWithID(unsigned int id) {
	assert(0 <= id && id < dft->getNodes().size());
	return dft->getNodes().at(id);
}

int DFT::DFTreeEXPBuilder::buildEXPHeader(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules) {
	return 0;
}

int DFT::DFTreeEXPBuilder::buildEXPHeader(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
		vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& onlineRules) {
	return 0;
}

int DFT::DFTreeEXPBuilder::buildEXPHeader(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
                                          vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& onlineRules, vector<DFT::EXPSyncRule*>& inspectionRules, vector<DFT::EXPSyncRule*>& inspectedRules, vector<DFT::EXPSyncRule*>& resetRules) {
    return 0;
}

int DFT::DFTreeEXPBuilder::parseDFT(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules) {
	
	/* Create synchronization rules for all nodes in the DFT */

	// Create synchronization rules for the Top node in the DFT
	createSyncRuleTop(activationRules,failRules);

	// Create synchronization rules for all nodes in the DFT
	{
		std::vector<DFT::Nodes::Node*>::const_iterator it = dft->getNodes().begin();
		for(;it!=dft->getNodes().end();++it) {
			if((*it)->isGate()) {
				const DFT::Nodes::Gate& gate = static_cast<const DFT::Nodes::Gate&>(**it);
				cc->reportAction("Creating synchronization rules for `" + gate.getName() + "' (THIS node)",VERBOSITY_FLOW);
				createSyncRule(activationRules,failRules,gate,getIDOfNode(gate));
			}
		}
	}
	return 0;
}

int DFT::DFTreeEXPBuilder::parseDFT(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
		vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules) {

	/* Create synchronization rules for all nodes in the DFT */

	// Create synchronization rules for the Top node in the DFT
	createSyncRuleTop(activationRules,failRules,onlineRules);

	// Create synchronization rules for all nodes in the DFT
	{
		std::vector<DFT::Nodes::Node*>::const_iterator it = dft->getNodes().begin();
		for(;it!=dft->getNodes().end();++it) {
			if((*it)->isGate()) {
				const DFT::Nodes::Gate& gate = static_cast<const DFT::Nodes::Gate&>(**it);
				cc->reportAction("Creating synchronization rules for `" + gate.getName() + "' (THIS node)",VERBOSITY_FLOW);
				createSyncRule(activationRules,failRules,repairRules,repairedRules,repairingRules,onlineRules,gate,getIDOfNode(gate));
			}
		}
	}
	return 0;
}

int DFT::DFTreeEXPBuilder::parseDFT(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
                                    vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules, vector<DFT::EXPSyncRule*>& inspectionRules, vector<DFT::EXPSyncRule*>& inspectedRules, vector<DFT::EXPSyncRule*>& resetRules) {
    
    /* Create synchronization rules for all nodes in the DFT */
    
    // Create synchronization rules for the Top node in the DFT
    createSyncRuleTop(activationRules,failRules,onlineRules);
    
    // Create synchronization rules for all nodes in the DFT
    {
        std::vector<DFT::Nodes::Node*>::const_iterator it = dft->getNodes().begin();
        for(;it!=dft->getNodes().end();++it) {
            if((*it)->isGate()) {
                const DFT::Nodes::Gate& gate = static_cast<const DFT::Nodes::Gate&>(**it);
                cc->reportAction("Creating synchronization rules for `" + gate.getName() + "' (THIS node)",VERBOSITY_FLOW);
                createSyncRule(activationRules,failRules,repairRules,repairedRules,repairingRules,onlineRules,inspectionRules,inspectedRules,resetRules,gate,getIDOfNode(gate));
            }
        }
    }
    return 0;
}

int DFT::DFTreeEXPBuilder::buildEXPBody(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules) {

	/* Generate the EXP based on the generated synchronization rules */
	exp_body.clearAll();
	exp_body << exp_body.applyprefix << "(* Number of rules: " << (activationRules.size()+failRules.size()) << "*)" << exp_body.applypostfix;
	exp_body << exp_body.applyprefix << "hide" << exp_body.applypostfix;
	exp_body.indent();
	
		// Hide rules
		for(size_t s=0; s<activationRules.size(); ++s) {
			EXPSyncRule& rule = *activationRules.at(s);
			if(rule.hideToLabel) {
				exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
			}
		}
		for(size_t s=0; s<failRules.size(); ++s) {
			EXPSyncRule& rule = *failRules.at(s);
			if(rule.hideToLabel) {
				exp_body << exp_body.applyprefix << rule.toLabel;
				if(s<failRules.size()-1) exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}
	
	exp_body.outdent();
	exp_body.appendLine("in");
	exp_body.indent();
	{
		// Synchronization rules
		vector<unsigned int> columnWidths(dft->getNodes().size(),0);
		calculateColumnWidths(columnWidths,activationRules);
		calculateColumnWidths(columnWidths,failRules);

		exp_body << exp_body.applyprefix << "label par using" << exp_body.applypostfix;
		exp_body << exp_body.applyprefix << "(*\t";
		std::vector<DFT::Nodes::Node*>::const_iterator it = dft->getNodes().begin();
		for(int c=0;it!=dft->getNodes().end();++it,++c) {
			if(c>0) exp_body << "   ";
			exp_body.outlineLeftNext(columnWidths[c],' ');
			exp_body << exp_body._push << (*it)->getTypeStr() << getIDOfNode(**it) << exp_body._pop;
		}
		exp_body << " *)" << exp_body.applypostfix;
		
		exp_body.indent();
		// Generate activation rules
		{
			for(size_t s=0; s<activationRules.size(); ++s) {
				exp_body << exp_body.applyprefix;
				printSyncLine(*activationRules.at(s),columnWidths);
				exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}
		// Generate fail rules
		{
			for(size_t s=0; s<failRules.size(); ++s) {
				exp_body << exp_body.applyprefix;
				printSyncLine(*failRules.at(s),columnWidths);
				if(s<failRules.size()-1) exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}
		exp_body.outdent();

		// Generate the parallel composition of all the nodes
		exp_body << exp_body.applyprefix << "in" << exp_body.applypostfix;
		exp_body.indent();
		{
			int c=0;
			{
				std::vector<DFT::Nodes::Node*>::iterator it = dft->getNodes().begin();
				for(;it!=dft->getNodes().end();++it,++c) {
					const DFT::Nodes::Node& node = **it;
					if(c>0) exp_body << exp_body.applyprefix << "||" << exp_body.applypostfix;
					if(node.isBasicEvent()) {
						const DFT::Nodes::BasicEvent& be = *static_cast<const DFT::Nodes::BasicEvent*>(&node);
						exp_body << exp_body.applyprefix << getBEProc(be) << exp_body.applypostfix;
					} else if(node.isGate()) {
						exp_body << exp_body.applyprefix << "\"" << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(node) << ".bcg\"" << exp_body.applypostfix;
					} else {
						assert(0 && "buildEXPBody(): Unknown node type");
					}
				}
			}
		}
		exp_body.outdent();
		exp_body << exp_body.applyprefix << "end par" << exp_body.applypostfix;
	}
	exp_body.outdent();
	exp_body.appendLine("end hide");
	return 0;
}

int DFT::DFTreeEXPBuilder::buildEXPBody(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
		vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules) {

	/* Generate the EXP based on the generated synchronization rules */
	exp_body.clearAll();
	exp_body << exp_body.applyprefix << "(* Number of rules: " << (activationRules.size()+failRules.size()+repairRules.size()+repairedRules.size()+repairingRules.size()+onlineRules.size()) << "*)" << exp_body.applypostfix;
	exp_body << exp_body.applyprefix << "hide" << exp_body.applypostfix;
	exp_body.indent();

		// Hide rules
		for(size_t s=0; s<activationRules.size(); ++s) {
			EXPSyncRule& rule = *activationRules.at(s);
			if(rule.hideToLabel) {
				exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
			}
		}
		for(size_t s=0; s<repairRules.size(); ++s) {
			EXPSyncRule& rule = *repairRules.at(s);
			if(rule.hideToLabel) {
				exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
			}
		}
		for(size_t s=0; s<repairedRules.size(); ++s) {
			EXPSyncRule& rule = *repairedRules.at(s);
			if(rule.hideToLabel) {
				exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
			}
		}
		for(size_t s=0; s<onlineRules.size(); ++s) {
			EXPSyncRule& rule = *onlineRules.at(s);
			if(rule.hideToLabel) {
				exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
			}
		}
		for(size_t s=0; s<failRules.size(); ++s) {
			EXPSyncRule& rule = *failRules.at(s);
			if(rule.hideToLabel) {
				exp_body << exp_body.applyprefix << rule.toLabel;
				if(s<failRules.size()-1) exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}

	exp_body.outdent();
	exp_body.appendLine("in");
	exp_body.indent();
	{
		// Synchronization rules
		vector<unsigned int> columnWidths(dft->getNodes().size(),0);
		calculateColumnWidths(columnWidths,activationRules);
		calculateColumnWidths(columnWidths,failRules);
		calculateColumnWidths(columnWidths,repairRules);
		calculateColumnWidths(columnWidths,repairedRules);
		calculateColumnWidths(columnWidths,onlineRules);

		exp_body << exp_body.applyprefix << "label par using" << exp_body.applypostfix;
		exp_body << exp_body.applyprefix << "(*\t";
		std::vector<DFT::Nodes::Node*>::const_iterator it = dft->getNodes().begin();
		for(int c=0;it!=dft->getNodes().end();++it,++c) {
			if(c>0) exp_body << "   ";
			exp_body.outlineLeftNext(columnWidths[c],' ');
			exp_body << exp_body._push << (*it)->getTypeStr() << getIDOfNode(**it) << exp_body._pop;
		}
		exp_body << " *)" << exp_body.applypostfix;

		exp_body.indent();
		// Generate activation rules
		{
			for(size_t s=0; s<activationRules.size(); ++s) {
				exp_body << exp_body.applyprefix;
				printSyncLine(*activationRules.at(s),columnWidths);
				exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}
		// Generate repair rules
		{
			for(size_t s=0; s<repairRules.size(); ++s) {
				exp_body << exp_body.applyprefix;
				printSyncLine(*repairRules.at(s),columnWidths);
				exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}
		if(!repairingRules.empty()){
		// Generate repairing rules
		{
			for(size_t s=0; s<repairRules.size(); ++s) {
				exp_body << exp_body.applyprefix;
				printSyncLine(*repairingRules.at(s),columnWidths);
				exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}
		}
		// Generate repaired rules
		{
			for(size_t s=0; s<repairedRules.size(); ++s) {
				exp_body << exp_body.applyprefix;
				printSyncLine(*repairedRules.at(s),columnWidths);
				exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}
		// Generate online rules
		{
			for(size_t s=0; s<onlineRules.size(); ++s) {
				exp_body << exp_body.applyprefix;
				printSyncLine(*onlineRules.at(s),columnWidths);
				exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}
		// Generate fail rules
		{
			for(size_t s=0; s<failRules.size(); ++s) {
				exp_body << exp_body.applyprefix;
				printSyncLine(*failRules.at(s),columnWidths);
				if(s<failRules.size()-1) exp_body << ",";
				exp_body << exp_body.applypostfix;
			}
		}
		exp_body.outdent();

		// Generate the parallel composition of all the nodes
		exp_body << exp_body.applyprefix << "in" << exp_body.applypostfix;
		exp_body.indent();
		{
			int c=0;
			{
				std::vector<DFT::Nodes::Node*>::iterator it = dft->getNodes().begin();
				for(;it!=dft->getNodes().end();++it,++c) {
					const DFT::Nodes::Node& node = **it;
					if(c>0) exp_body << exp_body.applyprefix << "||" << exp_body.applypostfix;
					if(node.isBasicEvent()) {
						const DFT::Nodes::BasicEvent& be = *static_cast<const DFT::Nodes::BasicEvent*>(&node);
						exp_body << exp_body.applyprefix << getBEProc(be) << exp_body.applypostfix;
					} else if(node.isGate()) {
						if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitType) || DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitFcfsType)
						|| DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitPrioType) || DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitNdType)){
							const DFT::Nodes::Gate& ru = *static_cast<const DFT::Nodes::Gate*>(&node);
							exp_body << exp_body.applyprefix << getRUProc(ru) << exp_body.applypostfix;
						}else {
							exp_body << exp_body.applyprefix << "\"" << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(node) << ".bcg\"" << exp_body.applypostfix;
						}
					} else {
						assert(0 && "buildEXPBody(): Unknown node type");
					}
				}
			}
		}
		exp_body.outdent();
		exp_body << exp_body.applyprefix << "end par" << exp_body.applypostfix;
	}
	exp_body.outdent();
	exp_body.appendLine("end hide");
	return 0;
}

int DFT::DFTreeEXPBuilder::buildEXPBody(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
                                        vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules, vector<DFT::EXPSyncRule*>& inspectionRules, vector<DFT::EXPSyncRule*>& inspectedRules, vector<DFT::EXPSyncRule*>& resetRules) {
    
    /* Generate the EXP based on the generated synchronization rules */
    exp_body.clearAll();
    exp_body << exp_body.applyprefix << "(* Number of rules: " << (activationRules.size()+failRules.size()+repairRules.size()+repairedRules.size()+repairingRules.size()+onlineRules.size()+inspectionRules.size()) << "*)" << exp_body.applypostfix;
    exp_body << exp_body.applyprefix << "hide" << exp_body.applypostfix;
    exp_body.indent();
    
    // Hide rules
    for(size_t s=0; s<activationRules.size(); ++s) {
        EXPSyncRule& rule = *activationRules.at(s);
        if(rule.hideToLabel) {
            exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
        }
    }
    for(size_t s=0; s<repairRules.size(); ++s) {
        EXPSyncRule& rule = *repairRules.at(s);
        if(rule.hideToLabel) {
            exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
        }
    }
    for(size_t s=0; s<repairedRules.size(); ++s) {
        EXPSyncRule& rule = *repairedRules.at(s);
        if(rule.hideToLabel) {
            exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
        }
    }
    for(size_t s=0; s<onlineRules.size(); ++s) {
        EXPSyncRule& rule = *onlineRules.at(s);
        if(rule.hideToLabel) {
            exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
        }
    }
    for(size_t s=0; s<inspectionRules.size(); ++s) {
        EXPSyncRule& rule = *inspectionRules.at(s);
        if(rule.hideToLabel) {
            exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
        }
    }
    for(size_t s=0; s<inspectedRules.size(); ++s) {
        EXPSyncRule& rule = *inspectedRules.at(s);
        if(rule.hideToLabel) {
            exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
        
        }
    }
    for(size_t s=0; s<resetRules.size(); ++s) {
        EXPSyncRule& rule = *resetRules.at(s);
        if(rule.hideToLabel) {
            exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
            
        }
    }
    for(size_t s=0; s<failRules.size(); ++s) {
        EXPSyncRule& rule = *failRules.at(s);
        if(rule.hideToLabel) {
            exp_body << exp_body.applyprefix << rule.toLabel;
            if(s<failRules.size()-1) exp_body << ",";
            exp_body << exp_body.applypostfix;
        }
    }
    
    exp_body.outdent();
    exp_body.appendLine("in");
    exp_body.indent();
    {
        // Synchronization rules
        vector<unsigned int> columnWidths(dft->getNodes().size(),0);
        calculateColumnWidths(columnWidths,activationRules);
        calculateColumnWidths(columnWidths,failRules);
        calculateColumnWidths(columnWidths,repairRules);
        calculateColumnWidths(columnWidths,repairedRules);
        calculateColumnWidths(columnWidths,onlineRules);
        calculateColumnWidths(columnWidths,inspectionRules);
        calculateColumnWidths(columnWidths,inspectedRules);
        calculateColumnWidths(columnWidths,resetRules);
        
        exp_body << exp_body.applyprefix << "label par using" << exp_body.applypostfix;
        exp_body << exp_body.applyprefix << "(*\t";
        std::vector<DFT::Nodes::Node*>::const_iterator it = dft->getNodes().begin();
        for(int c=0;it!=dft->getNodes().end();++it,++c) {
            if(c>0) exp_body << "   ";
            exp_body.outlineLeftNext(columnWidths[c],' ');
            exp_body << exp_body._push << (*it)->getTypeStr() << getIDOfNode(**it) << exp_body._pop;
        }
        exp_body << " *)" << exp_body.applypostfix;
        
        exp_body.indent();
        // Generate activation rules
        {
            for(size_t s=0; s<activationRules.size(); ++s) {
                exp_body << exp_body.applyprefix;
                printSyncLine(*activationRules.at(s),columnWidths);
                exp_body << ",";
                exp_body << exp_body.applypostfix;
            }
        }
        // Generate repair rules
        {
            for(size_t s=0; s<repairRules.size(); ++s) {
                exp_body << exp_body.applyprefix;
                printSyncLine(*repairRules.at(s),columnWidths);
                exp_body << ",";
                exp_body << exp_body.applypostfix;
            }
        }
        if(!repairingRules.empty()){
            // Generate repairing rules
            {
                for(size_t s=0; s<repairRules.size(); ++s) {
                    exp_body << exp_body.applyprefix;
                    printSyncLine(*repairingRules.at(s),columnWidths);
                    exp_body << ",";
                    exp_body << exp_body.applypostfix;
                }
            }
        }
        // Generate repaired rules
        {
            for(size_t s=0; s<repairedRules.size(); ++s) {
                exp_body << exp_body.applyprefix;
                printSyncLine(*repairedRules.at(s),columnWidths);
                exp_body << ",";
                exp_body << exp_body.applypostfix;
            }
        }
        // Generate online rules
        {
            for(size_t s=0; s<onlineRules.size(); ++s) {
                exp_body << exp_body.applyprefix;
                printSyncLine(*onlineRules.at(s),columnWidths);
                exp_body << ",";
                exp_body << exp_body.applypostfix;
            }
        }
        // Generate inspection rules
        {
            for(size_t s=0; s<inspectionRules.size(); ++s) {
                exp_body << exp_body.applyprefix;
                printSyncLine(*inspectionRules.at(s),columnWidths);
                exp_body << ",";
                exp_body << exp_body.applypostfix;
            }
        }
        // Generate inspected rules
        {
            for(size_t s=0; s<inspectedRules.size(); ++s) {
                exp_body << exp_body.applyprefix;
                printSyncLine(*inspectedRules.at(s),columnWidths);
                exp_body << ",";
                exp_body << exp_body.applypostfix;
            }
        }
        // Generate reset rules
        {
            for(size_t s=0; s<resetRules.size(); ++s) {
                exp_body << exp_body.applyprefix;
                printSyncLine(*resetRules.at(s),columnWidths);
                exp_body << ",";
                exp_body << exp_body.applypostfix;
            }
        }
        // Generate fail rules
        {
            for(size_t s=0; s<failRules.size(); ++s) {
                exp_body << exp_body.applyprefix;
                printSyncLine(*failRules.at(s),columnWidths);
                if(s<failRules.size()-1) exp_body << ",";
                exp_body << exp_body.applypostfix;
            }
        }
        exp_body.outdent();
        
        // Generate the parallel composition of all the nodes
        exp_body << exp_body.applyprefix << "in" << exp_body.applypostfix;
        exp_body.indent();
        {
            int c=0;
            {
                std::vector<DFT::Nodes::Node*>::iterator it = dft->getNodes().begin();
                for(;it!=dft->getNodes().end();++it,++c) {
                    const DFT::Nodes::Node& node = **it;
                    if(c>0) exp_body << exp_body.applyprefix << "||" << exp_body.applypostfix;
                    if(node.isBasicEvent()) {
                        const DFT::Nodes::BasicEvent& be = *static_cast<const DFT::Nodes::BasicEvent*>(&node);
                        exp_body << exp_body.applyprefix << getBEProc(be) << exp_body.applypostfix;
                    } else if(node.isGate()) {
                        if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitType) || DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitFcfsType)
                           || DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitPrioType) || DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitNdType)){
                            const DFT::Nodes::Gate& ru = *static_cast<const DFT::Nodes::Gate*>(&node);
                            exp_body << exp_body.applyprefix << getRUProc(ru) << exp_body.applypostfix;
                        }else if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::InspectionType)){
                            const DFT::Nodes::Inspection& insp = *static_cast<const DFT::Nodes::Inspection*>(&node);
                            exp_body << exp_body.applyprefix << getINSPProc(insp) << exp_body.applypostfix;
                        } else if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::ReplacementType)){
                            const DFT::Nodes::Replacement& rep = *static_cast<const DFT::Nodes::Replacement*>(&node);
                            exp_body << exp_body.applyprefix << getREPProc(rep) << exp_body.applypostfix;
                        } else {
                            exp_body << exp_body.applyprefix << "\"" << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(node) << ".bcg\"" << exp_body.applypostfix;
                        }
                    } else {
                        assert(0 && "buildEXPBody(): Unknown node type");
                    }
                }
            }
        }
        exp_body.outdent();
        exp_body << exp_body.applyprefix << "end par" << exp_body.applypostfix;
    }
    exp_body.outdent();
    exp_body.appendLine("end hide");
    return 0;
}


	int DFT::DFTreeEXPBuilder::createSyncRuleBE(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::BasicEvent& node, unsigned int nodeID) {
		return 0;
	}
	int DFT::DFTreeEXPBuilder::createSyncRuleGateOr(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateOr& node, unsigned int nodeID) {
		return 0;
	}
	int DFT::DFTreeEXPBuilder::createSyncRuleGateAnd(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateAnd& node, unsigned int nodeID) {
		return 0;
	}
    int DFT::DFTreeEXPBuilder::createSyncRuleGateSAnd(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateSAnd& node, unsigned int nodeID) {
        return 0;
    }
	int DFT::DFTreeEXPBuilder::createSyncRuleGateWSP(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateWSP& node, unsigned int nodeID) {
		return 0;
	}
	int DFT::DFTreeEXPBuilder::createSyncRuleGatePAnd(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GatePAnd& node, unsigned int nodeID) {
		return 0;
	}
    int DFT::DFTreeEXPBuilder::createSyncRuleGatePor(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GatePor& node, unsigned int nodeID) {
        return 0;
    }
	int DFT::DFTreeEXPBuilder::createSyncRuleGateVoting(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateVoting& node, unsigned int nodeID) {
		return 0;
	}
	
	/**
	 * Add FDEP Specific syncRules
	 * The FDEP needs to synchronize with multiple nodes: its source (trigger) and its dependers.
	 * To synchronize with the source, the general parent-child relationship is used. But for the rest, we need
	 * this method. This method will add a failSyncRule per depender to the list of rules, such that each
	 * dependers' parents will be notified, in a nondeterministic fashion. Thus, if the source fails, the order
	 * of failing dependers is nondeterministic.
	 */
	int DFT::DFTreeEXPBuilder::createSyncRuleGateFDEP(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateFDEP& node, unsigned int nodeID) {
        
		// Loop over all the dependers
		cc->reportAction3("FDEP Dependencies of THIS node...",VERBOSITY_RULEORIGINS);
		for(int dependerLocalID=0; dependerLocalID<(int)node.getDependers().size(); ++dependerLocalID) {
			DFT::Nodes::Node* depender = node.getDependers()[dependerLocalID];
			unsigned int dependerID = getIDOfNode(*depender);
			
			cc->reportAction3("Depender `" + depender->getName() + "'",VERBOSITY_RULEORIGINS);
			
			// Create a new failSyncRule
			std::stringstream ss;
			ss << "f_" << node.getTypeStr() << nodeID << "_" << depender->getTypeStr() << dependerID;
			EXPSyncRule* ruleF = new EXPSyncRule(ss.str());
			
			// Add the depender to the synchronization (+2, because in LNT the depender list starts at 2)
			ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(dependerLocalID+2)) );
			cc->reportAction3("Added fail rule, notifying:",VERBOSITY_RULEORIGINS);
			
			// Loop over the parents of the depender
			for(DFT::Nodes::Node* depParent: depender->getParents()) {
				// Obtain the localChildID of the depender seen from this parent
				unsigned int parentID = getIDOfNode(*depParent);
				int localChildID = getLocalIDOfNode(depParent,depender);
				assert(localChildID>=0 && "depender is not a child of its parent");
				
				// Add the parent to the synchronization rule, hooking into the FAIL of the depender,
				// making it appear to the parent that the child failed (+1, because in LNT the child list starts at 1)
				ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(parentID,syncFail(localChildID+1)) );
				cc->reportAction3("  Node `" + depParent->getName() + "'",VERBOSITY_RULEORIGINS);
			}
			
			// Add it to the list of rules
			failRules.push_back(ruleF);
			{
				std::stringstream report;
				report << "Added new fail       sync rule: ";
				printSyncLineShort(report,*ruleF);
				cc->reportAction2(report.str(),VERBOSITY_RULES);
			}
		}
		
		return 0;
	}

	/**
	 * Add RU Specific syncRules
	 * The RU needs to synchronize with multiple nodes: namely its dependers.
	 */ 
	int DFT::DFTreeEXPBuilder::createSyncRuleRepairUnit(vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, const DFT::Nodes::RepairUnit& node, unsigned int nodeID) {
		return 0;
	}

    /**
     * Add INSP Specific syncRules
     * The INSP needs to synchronize with multiple nodes: namely its dependers.
     */
    int DFT::DFTreeEXPBuilder::createSyncRuleInspection(vector<DFT::EXPSyncRule*>& inspectionRules, vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, const DFT::Nodes::Inspection& node, unsigned int nodeID) {
        return 0;
    }

    /**
    * Add INSP Specific syncRules
    * The INSP needs to synchronize with multiple nodes: namely its dependers.
    */
    int DFT::DFTreeEXPBuilder::createSyncRuleInspected(vector<DFT::EXPSyncRule*>& inspectedRules, vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, const DFT::Nodes::Inspection& node, unsigned int nodeID) {
        return 0;
    }

    /**
     * Add REP Specific syncRules
     * The REP needs to synchronize with multiple nodes: namely its dependers.
     */
    int DFT::DFTreeEXPBuilder::createSyncRuleReplacement(vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, const DFT::Nodes::Replacement& node, unsigned int nodeID) {
        return 0;
    }

	int DFT::DFTreeEXPBuilder::createSyncRuleTop(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules) {
		std::stringstream ss;
		
		// Obtain the Top Node
		std::map<const DFT::Nodes::Node*, unsigned int>::iterator it = nodeIDs.find(dft->getTopNode());
		assert( (it != nodeIDs.end()) );

		// Generate the Top Node Activate rule
		ss << DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE;
		if(!nameTop.empty()) ss << "_" << nameTop;
		DFT::EXPSyncRule* ruleA = new EXPSyncRule(ss.str(),false);
		ss.str("");
		ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(it->second,syncActivate(0,false)) );
        
        // Generate the FDEP Node Activate rule
        int c=0;
        {
            std::vector<DFT::Nodes::Node*>::iterator it = dft->getNodes().begin();
            for(;it!=dft->getNodes().end();++it,++c) {
                const DFT::Nodes::Node& node = **it;
                if(node.isGate()) {
                    if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::GateFDEPType))
                        ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(c,syncActivate(0,false)) );
                }
            }
        }
		
		std::stringstream report;
		report << "New EXPSyncRule " << ss.str();
		cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
		
		// Generate the Top Node Fail rule
		ss << DFT::DFTreeBCGNodeBuilder::GATE_FAIL;
		if(!nameTop.empty()) ss << "_" << nameTop;
		DFT::EXPSyncRule* ruleF = new EXPSyncRule(ss.str(),false);
		ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(it->second,syncFail(0)) );
		
		// Add the generated rules to the lists
		activationRules.push_back(ruleA);
		failRules.push_back(ruleF);
		
		cc->reportAction("Creating synchronization rules for Top node",VERBOSITY_FLOW);
		{
			std::stringstream report;
			report << "Added new activation sync rule: ";
			printSyncLineShort(report,*ruleA);
			cc->reportAction2(report.str(),VERBOSITY_RULES);
		}
		{
			std::stringstream report;
			report << "Added new fail       sync rule: ";
			printSyncLineShort(report,*ruleF);
			cc->reportAction2(report.str(),VERBOSITY_RULES);
		}
		
		return 0;
	}

	int DFT::DFTreeEXPBuilder::createSyncRuleTop(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, vector<DFT::EXPSyncRule*>& onlineRules) {
		std::stringstream ss;

		// Obtain the Top Node
		std::map<const DFT::Nodes::Node*, unsigned int>::iterator it = nodeIDs.find(dft->getTopNode());
		assert( (it != nodeIDs.end()) );

		// Generate the Top Node Activate rule
		ss << DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE;
		if(!nameTop.empty()) ss << "_" << nameTop;
		DFT::EXPSyncRule* ruleA = new EXPSyncRule(ss.str(),false);
		ss.str("");
		ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(it->second,syncActivate(0,false)) );
        
        // Generate the FDEP Node Activate rule
        int c=0;
        {
            std::vector<DFT::Nodes::Node*>::iterator it = dft->getNodes().begin();
            for(;it!=dft->getNodes().end();++it,++c) {
                const DFT::Nodes::Node& node = **it;
                if(node.isGate()) {
                    if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::GateFDEPType))
                        ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(c,syncActivate(0,false)) );
                }
            }
        }
        
        // Generate the Inspection Node Activate rule
        c=0;
        {
            std::vector<DFT::Nodes::Node*>::iterator it = dft->getNodes().begin();
            for(;it!=dft->getNodes().end();++it,++c) {
                const DFT::Nodes::Node& node = **it;
                if(node.isGate()) {
                    if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::InspectionType))
                        ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(c,syncActivate(0,false)) );
                }
            }
        }


		std::stringstream report;
		report << "New EXPSyncRule " << ss.str();
		cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);

		// Generate the Top Node Fail rule
		ss << DFT::DFTreeBCGNodeBuilder::GATE_FAIL;
		if(!nameTop.empty()) ss << "_" << nameTop;
		DFT::EXPSyncRule* ruleF = new EXPSyncRule(ss.str(),false);
		ss.str("");
		ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(it->second,syncFail(0)) );

		DFT::EXPSyncRule* ruleO;
		if(dft->getTopNode()->isRepairable()){
			// Generate the Top Node Online rule
			ss << DFT::DFTreeBCGNodeBuilder::GATE_ONLINE;
			ruleO = new EXPSyncRule(ss.str(),false);
			if(!nameTop.empty()) ss << "_" << nameTop;
			ruleO->label.insert( pair<unsigned int,EXPSyncItem*>(it->second,syncOnline(0)) );
			onlineRules.push_back(ruleO);
		}

		// Add the generated rules to the lists
		activationRules.push_back(ruleA);
		failRules.push_back(ruleF);

		cc->reportAction("Creating synchronization rules for Top node",VERBOSITY_FLOW);
		{
			std::stringstream report;
			report << "Added new activation sync rule: ";
			printSyncLineShort(report,*ruleA);
			cc->reportAction2(report.str(),VERBOSITY_RULES);
		}
		{
			std::stringstream report;
			report << "Added new fail       sync rule: ";
			printSyncLineShort(report,*ruleF);
			cc->reportAction2(report.str(),VERBOSITY_RULES);
		}
		if(dft->getTopNode()->isRepairable()){
		{
			std::stringstream report;
			report << "Added new online     sync rule: ";
			printSyncLineShort(report,*ruleO);
			cc->reportAction2(report.str(),VERBOSITY_RULES);
		}
		}

		return 0;
	}

	int DFT::DFTreeEXPBuilder::createSyncRule(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::Gate& node, unsigned int nodeID) {
		
		/* Generate non-specific rules for the node */
		
		// Go through all the children
		for(size_t n = 0; n<node.getChildren().size(); ++n) {
			
			// Get the current child and associated childID
			const DFT::Nodes::Node& child = *node.getChildren().at(n);
			
			std::map<const DFT::Nodes::Node*, unsigned int>::iterator it = nodeIDs.find(&child);
			assert( (it != nodeIDs.end()) && "createSyncRule() was looking for nonexistent node");
			unsigned int childID = it->second;
			
			cc->reportAction2("Child `" + child.getName() + "'" + (child.usesDynamicActivation()?" (dynact)":"") + " ...",VERBOSITY_RULES);
			
			/** ACTIVATION RULE **/
			{
				// Create labelTo string
				std::stringstream ss;
				ss << "a_" << node.getTypeStr() << nodeID << "_" << child.getTypeStr() << childID;
				EXPSyncRule* ruleA = new EXPSyncRule(ss.str());
				
				std::stringstream report;
				report << "New EXPSyncRule " << ss.str();
				cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
				
				// Set synchronization node
				ruleA->syncOnNode = &child;
				
				// Add synchronization of THIS node to the synchronization rule
				ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,true)) );
				cc->reportAction3("THIS node added to sync rule",VERBOSITY_RULEORIGINS);
				
				// Go through all the existing activation rules
				std::vector<DFT::EXPSyncRule*>::iterator ita = activationRules.begin();
				for(;ita != activationRules.end();++ita) {
					
					// If there is a rule that also synchronizes on the same node,
					// we have come across a child with another parent.
					if((*ita)->syncOnNode == &child) {
						EXPSyncRule* otherRuleA = *ita;
						std::stringstream report;
						report << "Detected earlier activation rule: ";
						printSyncLineShort(report,*otherRuleA);
						cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
						
						// First, we look up the sending Node of the
						// current activation rule...
						int otherNodeID = -1;
						int otherLocalNodeID = -1;
						for(auto& syncItem: otherRuleA->label) {
							if(syncItem.second->getArg(1)) {
								otherNodeID = syncItem.first;
								otherLocalNodeID = syncItem.second->getArg(0);
								break;
							}
						}
						if(otherNodeID<0) {
							cc->reportError("Could not find sender of the other rule, bailing...");
							return 1;
						}
						const DFT::Nodes::Node* otherNode = getNodeWithID(otherNodeID);
						assert(otherNode);
						
						// The synchronization depends if THIS node uses
						// dynamic activation or not. The prime example of
						// this is the Spare gate.
						// It also depends on the sending Node of the current
						// activation rule (otherNode). Both have to use
						// dynamic activation, because otherwise a Spare gate
						// will not activate a child if it's activated by a
						// static node, which is what is required.
						// The fundamental issue is that the semantics of
						// "using" or "claiming" a node is implicitely within
						// activating it. This should be covered more
						// thoroughly in the theory first, thus:
						/// @todo Fix "using" semantics in activation"
						if(node.usesDynamicActivation() && otherNode->usesDynamicActivation()) {
							
							
							// If the THIS node used dynamic activation, the
							// node is activated by an other parent.
							
							// Thus, add a synchronization item to the
							// existing synchronization rule, specifying that
							// the THIS node also receives a sent Activate.
							otherRuleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,false)) );
							cc->reportAction3("THIS node added, activation listening synchronization",VERBOSITY_RULEORIGINS);
							
							// This is not enough, because the other way
							// around also has to be added: the other node
							// wants to listen to Activates of the THIS node
							// as well.
							// Thus, we add a synchronization item to the
							// new rule we create for the THIS node, specifying
							// the other node wants to listen to activates of
							// the THIS node.
							ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(otherNodeID,syncActivate(otherLocalNodeID,false)) );
							cc->reportAction3("Detected (other) dynamic activator `" + otherNode->getName() + "', added to sync rule",VERBOSITY_RULEORIGINS);
							
							// TODO: primary is a special case??????
						} else {
							
							// If the THIS node does not use dynamic
							// activation, we can simply add the THIS node
							// as a sender to the other synchronization rule.
							// FIXME: Possibly this is actually never wanted,
							// as it could allow multiple senders to synchronize
							// with each other.
							//otherRuleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,true)) );
						}
					}
				}
				
				// Add the child Node to the synchronization rule
				// Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
				ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncActivate(0,false)) );
				cc->reportAction3("Child added to sync rule",VERBOSITY_RULEORIGINS);
				
				{
					std::stringstream report;
					report << "Added new activation sync rule: ";
					printSyncLineShort(report,*ruleA);
					cc->reportAction2(report.str(),VERBOSITY_RULES);
				}
				activationRules.push_back(ruleA);
			}
			
			/** FAIL RULE **/
			{
				// Go through all the existing fail rules
				std::vector<EXPSyncRule*>::iterator itf = failRules.begin();
				bool areOtherRules = false;
				for(;itf != failRules.end();++itf) {
					
					// If there is a rule that also synchronizes on the same node,
					// we have come across a child with another parent.
					if((*itf)->syncOnNode == &child) {
						cc->reportAction3("Detected earlier fail rule",VERBOSITY_RULEORIGINS);
						if(areOtherRules) {
							cc->reportError("There should be only one FAIL rule per node, bailing...");
							return 1;
						}
						areOtherRules = true;
						// We can simply add the THIS node as a sender to
						// the other synchronization rule.
						// FIXME: Possibly this is actually never wanted,
						// as it could allow multiple senders to synchronize
						// with each other.
						(*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(n+1)) );
						cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
					}
				}
				
				// If there are other rules that synchronize on the same node,
				// we do not need to synchronize any further
				// FIXME: This is probably not true!
				if(!areOtherRules) {
					// Add the child Node to the synchronization rule
					// Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
					std::stringstream ss;
					ss << "f_" << child.getTypeStr() << childID;
					EXPSyncRule* ruleF = new EXPSyncRule(ss.str());
					ruleF->syncOnNode = &child;
					ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(n+1)) );
					ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncFail(0)) );
					{
						std::stringstream report;
						report << "Added new fail       sync rule: ";
						printSyncLineShort(report,*ruleF);
						cc->reportAction2(report.str(),VERBOSITY_RULES);
					}
					failRules.push_back(ruleF);
				}
			}
			
		}
		
		/* Generate node-specific rules for the node */
		
		switch(node.getType()) {
		case DFT::Nodes::BasicEventType: {
//			const DFT::Nodes::BasicEvent* be = static_cast<const DFT::Nodes::BasicEvent*>(&node);
//			createSyncRuleBE(syncRules,*be,nodeID);
			break;
		}
		case DFT::Nodes::GateType: {
			cc->reportError("A gate should have a specialized type, not the general GateType");
			break;
		}
		case DFT::Nodes::GatePhasedOrType: {
			cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
			break;
		}
		case DFT::Nodes::GateOrType: {
			const DFT::Nodes::GateOr* g = static_cast<const DFT::Nodes::GateOr*>(&node);
			createSyncRuleGateOr(activationRules,failRules,*g,nodeID);
			break;
		}
		case DFT::Nodes::GateAndType: {
			const DFT::Nodes::GateAnd* g = static_cast<const DFT::Nodes::GateAnd*>(&node);
			createSyncRuleGateAnd(activationRules,failRules,*g,nodeID);
			break;
		}
        case DFT::Nodes::GateSAndType: {
            const DFT::Nodes::GateSAnd* g = static_cast<const DFT::Nodes::GateSAnd*>(&node);
            createSyncRuleGateSAnd(activationRules,failRules,*g,nodeID);
            break;
        }
		case DFT::Nodes::GateHSPType: {
			cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
			break;
		}
		case DFT::Nodes::GateWSPType: {
			const DFT::Nodes::GateWSP* g = static_cast<const DFT::Nodes::GateWSP*>(&node);
			createSyncRuleGateWSP(activationRules,failRules,*g,nodeID);
			break;
		}
		case DFT::Nodes::GateCSPType: {
			cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
			break;
		}
		case DFT::Nodes::GatePAndType: {
			const DFT::Nodes::GatePAnd* g = static_cast<const DFT::Nodes::GatePAnd*>(&node);
			createSyncRuleGatePAnd(activationRules,failRules,*g,nodeID);
			break;
		}
        case DFT::Nodes::GatePorType: {
            const DFT::Nodes::GatePor* g = static_cast<const DFT::Nodes::GatePor*>(&node);
            createSyncRuleGatePor(activationRules,failRules,*g,nodeID);
            break;
        }
		case DFT::Nodes::GateSeqType: {
			cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
			break;
		}
		case DFT::Nodes::GateVotingType: {
			const DFT::Nodes::GateVoting* g = static_cast<const DFT::Nodes::GateVoting*>(&node);
			createSyncRuleGateVoting(activationRules,failRules,*g,nodeID);
			break;
		}
		case DFT::Nodes::GateFDEPType: {
			const DFT::Nodes::GateFDEP* g = static_cast<const DFT::Nodes::GateFDEP*>(&node);
			createSyncRuleGateFDEP(activationRules,failRules,*g,nodeID);
			break;
		}
		case DFT::Nodes::GateTransferType: {
			cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
			break;
		}
		default: {
			cc->reportError("UnknownNode");
			break;
		}
		}
		
		return 0;
	}

	int DFT::DFTreeEXPBuilder::createSyncRule(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
			vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules, const DFT::Nodes::Gate& node, unsigned int nodeID) {

			/* Generate non-specific rules for the node */

			// Go through all the children
			for(size_t n = 0; n<node.getChildren().size(); ++n) {

				// Get the current child and associated childID
				const DFT::Nodes::Node& child = *node.getChildren().at(n);

				std::map<const DFT::Nodes::Node*, unsigned int>::iterator it = nodeIDs.find(&child);
				assert( (it != nodeIDs.end()) && "createSyncRule() was looking for nonexistent node");
				unsigned int childID = it->second;

				cc->reportAction2("Child `" + child.getName() + "'" + (child.usesDynamicActivation()?" (dynact)":"") + " ...",VERBOSITY_RULES);

				// ask if we have a repair unit (if it is the case we don't have to handle activation and fail)
				if(!DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitType) && !DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitFcfsType)
				&& !DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitPrioType) && !DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitNdType))
				{

				/** ACTIVATION RULE **/
				{
					// Create labelTo string
					std::stringstream ss;
					ss << "a_" << node.getTypeStr() << nodeID << "_" << child.getTypeStr() << childID;
					EXPSyncRule* ruleA = new EXPSyncRule(ss.str());

					std::stringstream report;
					report << "New EXPSyncRule " << ss.str();
					cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);

					// Set synchronization node
					ruleA->syncOnNode = &child;

					// Add synchronization of THIS node to the synchronization rule
					ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,true)) );
					cc->reportAction3("THIS node added to sync rule",VERBOSITY_RULEORIGINS);

					// Go through all the existing activation rules
					std::vector<DFT::EXPSyncRule*>::iterator ita = activationRules.begin();
					for(;ita != activationRules.end();++ita) {

						// If there is a rule that also synchronizes on the same node,
						// we have come across a child with another parent.
						if((*ita)->syncOnNode == &child) {
							EXPSyncRule* otherRuleA = *ita;
							std::stringstream report;
							report << "Detected earlier activation rule: ";
							printSyncLineShort(report,*otherRuleA);
							cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);

							// First, we look up the sending Node of the
							// current activation rule...
							int otherNodeID = -1;
							int otherLocalNodeID = -1;
							for(auto& syncItem: otherRuleA->label) {
								if(syncItem.second->getArg(1)) {
									otherNodeID = syncItem.first;
									otherLocalNodeID = syncItem.second->getArg(0);
									break;
								}
							}
							if(otherNodeID<0) {
								cc->reportError("Could not find sender of the other rule, bailing...");
								return 1;
							}
							const DFT::Nodes::Node* otherNode = getNodeWithID(otherNodeID);
							assert(otherNode);

							// The synchronization depends if THIS node uses
							// dynamic activation or not. The prime example of
							// this is the Spare gate.
							// It also depends on the sending Node of the current
							// activation rule (otherNode). Both have to use
							// dynamic activation, because otherwise a Spare gate
							// will not activate a child if it's activated by a
							// static node, which is what is required.
							// The fundamental issue is that the semantics of
							// "using" or "claiming" a node is implicitely within
							// activating it. This should be covered more
							// thoroughly in the theory first, thus:
							/// @todo Fix "using" semantics in activation"
							if(node.usesDynamicActivation() && otherNode->usesDynamicActivation()) {


								// If the THIS node used dynamic activation, the
								// node is activated by an other parent.

								// Thus, add a synchronization item to the
								// existing synchronization rule, specifying that
								// the THIS node also receives a sent Activate.
								otherRuleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,false)) );
								cc->reportAction3("THIS node added, activation listening synchronization",VERBOSITY_RULEORIGINS);

								// This is not enough, because the other way
								// around also has to be added: the other node
								// wants to listen to Activates of the THIS node
								// as well.
								// Thus, we add a synchronization item to the
								// new rule we create for the THIS node, specifying
								// the other node wants to listen to activates of
								// the THIS node.
								ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(otherNodeID,syncActivate(otherLocalNodeID,false)) );
								cc->reportAction3("Detected (other) dynamic activator `" + otherNode->getName() + "', added to sync rule",VERBOSITY_RULEORIGINS);

								// TODO: primary is a special case??????
							} else {

								// If the THIS node does not use dynamic
								// activation, we can simply add the THIS node
								// as a sender to the other synchronization rule.
								// FIXME: Possibly this is actually never wanted,
								// as it could allow multiple senders to synchronize
								// with each other.
								//otherRuleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,true)) );
							}
						}
					}

					// Add the child Node to the synchronization rule
					// Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
					ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncActivate(0,false)) );
					cc->reportAction3("Child added to sync rule",VERBOSITY_RULEORIGINS);

					{
						std::stringstream report;
						report << "Added new activation sync rule: ";
						printSyncLineShort(report,*ruleA);
						cc->reportAction2(report.str(),VERBOSITY_RULES);
					}
					activationRules.push_back(ruleA);
				}

				/** FAIL RULE **/
				{
					// Go through all the existing fail rules
					std::vector<EXPSyncRule*>::iterator itf = failRules.begin();
					bool areOtherRules = false;
					for(;itf != failRules.end();++itf) {

						// If there is a rule that also synchronizes on the same node,
						// we have come across a child with another parent.
						if((*itf)->syncOnNode == &child) {
							cc->reportAction3("Detected earlier fail rule",VERBOSITY_RULEORIGINS);
							if(areOtherRules) {
								cc->reportError("There should be only one FAIL rule per node, bailing...");
								return 1;
							}
							areOtherRules = true;
							// We can simply add the THIS node as a sender to
							// the other synchronization rule.
							// FIXME: Possibly this is actually never wanted,
							// as it could allow multiple senders to synchronize
							// with each other.
							(*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(n+1)) );
							cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
						}
					}

					// If there are other rules that synchronize on the same node,
					// we do not need to synchronize any further
					// FIXME: This is probably not true!
					if(!areOtherRules) {
						// Add the child Node to the synchronization rule
						// Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
						std::stringstream ss;
						ss << "f_" << child.getTypeStr() << childID;
						EXPSyncRule* ruleF = new EXPSyncRule(ss.str());
						ruleF->syncOnNode = &child;
						ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(n+1)) );
						ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncFail(0)) );
						{
							std::stringstream report;
							report << "Added new fail       sync rule: ";
							printSyncLineShort(report,*ruleF);
							cc->reportAction2(report.str(),VERBOSITY_RULES);
						}
						failRules.push_back(ruleF);
					}
				}

				/** ONLINE Rules **/
				{
                    if(node.isRepairable()) {
					// Go through all the existing fail rules
					std::vector<EXPSyncRule*>::iterator itf = onlineRules.begin();
					bool areOtherRules = false;
					for(;itf != onlineRules.end();++itf) {

						// If there is a rule that also synchronizes on the same node,
						// we have come across a child with another parent.
						if((*itf)->syncOnNode == &child) {
							cc->reportAction3("Detected earlier online rule",VERBOSITY_RULEORIGINS);
							if(areOtherRules) {
								cc->reportError("There should be only one ONLINE rule per node, bailing...");
								return 1;
							}
							areOtherRules = true;
							// We can simply add the THIS node as a sender to
							// the other synchronization rule.
							// FIXME: Possibly this is actually never wanted,
							// as it could allow multiple senders to synchronize
							// with each other.
							(*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(n+1)) );
							cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
						}
					}

					// If there are other rules that synchronize on the same node,
					// we do not need to synchronize any further
					// FIXME: This is probably not true!
					if(!areOtherRules) {
						// Add the child Node to the synchronization rule
						// Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
						std::stringstream ss;
						ss << "o_" << child.getTypeStr() << childID;
						EXPSyncRule* ruleO = new EXPSyncRule(ss.str());
						ruleO->syncOnNode = &child;
						ruleO->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncOnline(n+1)) );
						ruleO->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncOnline(0)) );
						{
							std::stringstream report;
							report << "Added new online     sync rule: ";
							printSyncLineShort(report,*ruleO);
							cc->reportAction2(report.str(),VERBOSITY_RULES);
						}
						onlineRules.push_back(ruleO);
					}
                    }
				}

				}else {

				/* REPAIR rule */
				{
					// Go through all the existing repair rules
					std::vector<EXPSyncRule*>::iterator itf = repairRules.begin();
					bool areOtherRules = false;
					for(;itf != repairRules.end();++itf) {

						// If there is a rule that also synchronizes on the same node,
						// we have come across a child with another parent.
						if((*itf)->syncOnNode == &child) {
							cc->reportAction3("Detected earlier repair rule",VERBOSITY_RULEORIGINS);
							if(areOtherRules) {
								cc->reportError("There should be only one REPAIR rule per node, bailing...");
								return 1;
							}
							areOtherRules = true;
							// We can simply add the THIS node as a sender to
							// the other synchronization rule.
							// FIXME: Possibly this is actually never wanted,
							// as it could allow multiple senders to synchronize
							// with each other.
							(*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(n+1)) );
							cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
						}
					}

					// If there are other rules that synchronize on the same node,
					// we do not need to synchronize any further
					// FIXME: This is probably not true!
					if(!areOtherRules) {
						// Add the child Node to the synchronization rule
						// Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
						std::stringstream ss;
						ss << "r_" << child.getTypeStr() << childID;
						EXPSyncRule* ruleR = new EXPSyncRule(ss.str());
						ruleR->syncOnNode = &child;
						ruleR->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepair(n+1)) );
						ruleR->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncRepair(0)) );
						{
							std::stringstream report;
							report << "Added new repair     sync rule: ";
							printSyncLineShort(report,*ruleR);
							cc->reportAction2(report.str(),VERBOSITY_RULES);
						}
						repairRules.push_back(ruleR);
					}
				}

				{
				/** REPAIRING RULE for ND **/
				if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitNdType)) {
					// Create labelTo string
					std::stringstream ss;
					ss << "REPAIRING_" << child.getTypeStr() << childID;
					EXPSyncRule* ruleRnd = new EXPSyncRule(ss.str(),false);

					std::stringstream report;
					report << "New EXPSyncRule " << ss.str();
					cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);

					ss << DFT::DFTreeBCGNodeBuilder::GATE_REPAIRING;
					if(!nameTop.empty()) ss << "_" << nameTop;
					ruleRnd->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepairing(n+1)) );
					repairingRules.push_back(ruleRnd);
				}
				}

				/** REPAIRED RULE **/
				{
					// Create labelTo string
					std::stringstream ss;
					ss << "rp_" << node.getTypeStr() << nodeID << "_" << child.getTypeStr() << childID;
					EXPSyncRule* ruleRP = new EXPSyncRule(ss.str());

					std::stringstream report;
					report << "New EXPSyncRule " << ss.str();
					cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);

					// Set synchronization node
					ruleRP->syncOnNode = &child;

					// Add synchronization of THIS node to the synchronization rule
					ruleRP->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepaired(n+1,true)) );
					cc->reportAction3("THIS node added to sync rule",VERBOSITY_RULEORIGINS);

					// Go through all the existing repaired rules
					std::vector<DFT::EXPSyncRule*>::iterator ita = repairedRules.begin();
					for(;ita != repairedRules.end();++ita) {

						// If there is a rule that also synchronizes on the same node,
						// we have come across a child with another parent.
						// Note: this can be interesting if we allow assining several RUs to a BE
						if((*ita)->syncOnNode == &child) {
							EXPSyncRule* otherRuleRP = *ita;
							std::stringstream report;
							report << "Detected earlier maintenance rule: ";
							printSyncLineShort(report,*otherRuleRP);
							cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);

							// First, we look up the sending Node of the
							// current activation rule...
							int otherNodeID = -1;
							int otherLocalNodeID = -1;
							for(auto& syncItem: otherRuleRP->label) {
								if(syncItem.second->getArg(1)) {
									otherNodeID = syncItem.first;
									otherLocalNodeID = syncItem.second->getArg(0);
									break;
								}
							}
							if(otherNodeID<0) {
								cc->reportError("Could not find sender of the other rule, bailing...");
								return 1;
							}
							const DFT::Nodes::Node* otherNode = getNodeWithID(otherNodeID);
							assert(otherNode);

							// The synchronization depends if THIS node uses
							// dynamic activation or not. The prime example of
							// this is the Spare gate.
							// It also depends on the sending Node of the current
							// activation rule (otherNode). Both have to use
							// dynamic activation, because otherwise a Spare gate
							// will not activate a child if it's activated by a
							// static node, which is what is required.
							// The fundamental issue is that the semantics of
							// "using" or "claiming" a node is implicitely within
							// activating it. This should be covered more
							// thoroughly in the theory first, thus:
							/// @todo Fix "using" semantics in activation"
							if(node.usesDynamicActivation() && otherNode->usesDynamicActivation()) {


								// If the THIS node used dynamic activation, the
								// node is activated by an other parent.

								// Thus, add a synchronization item to the
								// existing synchronization rule, specifying that
								// the THIS node also receives a sent Activate.
								otherRuleRP->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,false)) );
								cc->reportAction3("THIS node added, repaired listening synchronization",VERBOSITY_RULEORIGINS);

								// This is not enough, because the other way
								// around also has to be added: the other node
								// wants to listen to Activates of the THIS node
								// as well.
								// Thus, we add a synchronization item to the
								// new rule we create for the THIS node, specifying
								// the other node wants to listen to activates of
								// the THIS node.
								ruleRP->label.insert( pair<unsigned int,EXPSyncItem*>(otherNodeID,syncActivate(otherLocalNodeID,false)) );
								cc->reportAction3("Detected (other) dynamic repaired `" + otherNode->getName() + "', added to sync rule",VERBOSITY_RULEORIGINS);

								// TODO: primary is a special case??????
							} else {

								// If the THIS node does not use dynamic
								// activation, we can simply add the THIS node
								// as a sender to the other synchronization rule.
								// FIXME: Possibly this is actually never wanted,
								// as it could allow multiple senders to synchronize
								// with each other.
								//otherRuleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,true)) );
							}
						}
					}

					// Add the child Node to the synchronization rule
					// Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
					ruleRP->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncRepaired(0,false)) );
					cc->reportAction3("Child added to sync rule",VERBOSITY_RULEORIGINS);

					{
						std::stringstream report;
						report << "Added new repaired sync rule: ";
						printSyncLineShort(report,*ruleRP);
						cc->reportAction2(report.str(),VERBOSITY_RULES);
					}
					repairedRules.push_back(ruleRP);
				}

				}

			}

			/* Generate node-specific rules for the node */

			switch(node.getType()) {
			case DFT::Nodes::BasicEventType: {
	//			const DFT::Nodes::BasicEvent* be = static_cast<const DFT::Nodes::BasicEvent*>(&node);
	//			createSyncRuleBE(syncRules,*be,nodeID);
				break;
			}
			case DFT::Nodes::GateType: {
				cc->reportError("A gate should have a specialized type, not the general GateType");
				break;
			}
			case DFT::Nodes::GatePhasedOrType: {
				cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
				break;
			}
			case DFT::Nodes::GateOrType: {
				const DFT::Nodes::GateOr* g = static_cast<const DFT::Nodes::GateOr*>(&node);
				createSyncRuleGateOr(activationRules,failRules,*g,nodeID);
				break;
			}
			case DFT::Nodes::GateAndType: {
				const DFT::Nodes::GateAnd* g = static_cast<const DFT::Nodes::GateAnd*>(&node);
				createSyncRuleGateAnd(activationRules,failRules,*g,nodeID);
				break;
			}
            case DFT::Nodes::GateSAndType: {
                const DFT::Nodes::GateSAnd* g = static_cast<const DFT::Nodes::GateSAnd*>(&node);
                createSyncRuleGateSAnd(activationRules,failRules,*g,nodeID);
                break;
            }
			case DFT::Nodes::GateHSPType: {
				cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
				break;
			}
			case DFT::Nodes::GateWSPType: {
				const DFT::Nodes::GateWSP* g = static_cast<const DFT::Nodes::GateWSP*>(&node);
				createSyncRuleGateWSP(activationRules,failRules,*g,nodeID);
				break;
			}
			case DFT::Nodes::GateCSPType: {
				cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
				break;
			}
			case DFT::Nodes::GatePAndType: {
				const DFT::Nodes::GatePAnd* g = static_cast<const DFT::Nodes::GatePAnd*>(&node);
				createSyncRuleGatePAnd(activationRules,failRules,*g,nodeID);
				break;
			}
            case DFT::Nodes::GatePorType: {
                const DFT::Nodes::GatePor* g = static_cast<const DFT::Nodes::GatePor*>(&node);
                createSyncRuleGatePor(activationRules,failRules,*g,nodeID);
                break;
            }
			case DFT::Nodes::GateSeqType: {
				cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
				break;
			}
			case DFT::Nodes::GateVotingType: {
				const DFT::Nodes::GateVoting* g = static_cast<const DFT::Nodes::GateVoting*>(&node);
				createSyncRuleGateVoting(activationRules,failRules,*g,nodeID);
				break;
			}
			case DFT::Nodes::GateFDEPType: {
				const DFT::Nodes::GateFDEP* g = static_cast<const DFT::Nodes::GateFDEP*>(&node);
				createSyncRuleGateFDEP(activationRules,failRules,*g,nodeID);
				break;
			}
			case DFT::Nodes::GateTransferType: {
				cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
				break;
			}
			case DFT::Nodes::RepairUnitType: {
				const DFT::Nodes::RepairUnit* g = static_cast<const DFT::Nodes::RepairUnit*>(&node);
				createSyncRuleRepairUnit(repairRules,repairedRules,repairingRules,*g,nodeID);
				break;
			}
			case DFT::Nodes::RepairUnitFcfsType: {
				const DFT::Nodes::RepairUnit* g = static_cast<const DFT::Nodes::RepairUnit*>(&node);
				createSyncRuleRepairUnit(repairRules,repairedRules,repairingRules,*g,nodeID);
				break;
			}
			case DFT::Nodes::RepairUnitNdType: {
				const DFT::Nodes::RepairUnit* g = static_cast<const DFT::Nodes::RepairUnit*>(&node);
				createSyncRuleRepairUnit(repairRules,repairedRules,repairingRules,*g,nodeID);
				break;
			}
			case DFT::Nodes::RepairUnitPrioType: {
				const DFT::Nodes::RepairUnit* g = static_cast<const DFT::Nodes::RepairUnit*>(&node);
				createSyncRuleRepairUnit(repairRules,repairedRules,repairingRules,*g,nodeID);
				break;
			}
			default: {
				cc->reportError("UnknownNode");
				break;
			}
			}

			return 0;
    }


    int DFT::DFTreeEXPBuilder::createSyncRule(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
                                               vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules, vector<DFT::EXPSyncRule*>& inspectionRules, vector<DFT::EXPSyncRule*>& inspectedRules, vector<DFT::EXPSyncRule*>& resetRules, const DFT::Nodes::Gate& node, unsigned int nodeID) {
        
        /* Generate non-specific rules for the node */
        
        // Go through all the children
        for(size_t n = 0; n<node.getChildren().size(); ++n) {
            
            // Get the current child and associated childID
            const DFT::Nodes::Node& child = *node.getChildren().at(n);
            
            std::map<const DFT::Nodes::Node*, unsigned int>::iterator it = nodeIDs.find(&child);
            assert( (it != nodeIDs.end()) && "createSyncRule() was looking for nonexistent node");
            unsigned int childID = it->second;
            
            cc->reportAction2("Child `" + child.getName() + "'" + (child.usesDynamicActivation()?" (dynact)":"") + " ...",VERBOSITY_RULES);
            
            // ask if we have a repair unit (if it is the case we don't have to handle activation and fail) same for inspection and replacement
            if(!DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitType) && !DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitFcfsType)
               && !DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitPrioType) && !DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitNdType)
               && !DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::InspectionType) && !DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::ReplacementType))
            {
                
                /** ACTIVATION RULE **/
                {
                    // Create labelTo string
                    std::stringstream ss;
                    ss << "a_" << node.getTypeStr() << nodeID << "_" << child.getTypeStr() << childID;
                    EXPSyncRule* ruleA = new EXPSyncRule(ss.str());
                    
                    std::stringstream report;
                    report << "New EXPSyncRule " << ss.str();
                    cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
                    
                    // Set synchronization node
                    ruleA->syncOnNode = &child;
                    
                    // Add synchronization of THIS node to the synchronization rule
                    ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,true)) );
                    cc->reportAction3("THIS node added to sync rule",VERBOSITY_RULEORIGINS);
                    
                    // Go through all the existing activation rules
                    std::vector<DFT::EXPSyncRule*>::iterator ita = activationRules.begin();
                    for(;ita != activationRules.end();++ita) {
                        
                        // If there is a rule that also synchronizes on the same node,
                        // we have come across a child with another parent.
                        if((*ita)->syncOnNode == &child) {
                            EXPSyncRule* otherRuleA = *ita;
                            std::stringstream report;
                            report << "Detected earlier activation rule: ";
                            printSyncLineShort(report,*otherRuleA);
                            cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
                            
                            // First, we look up the sending Node of the
                            // current activation rule...
                            int otherNodeID = -1;
                            int otherLocalNodeID = -1;
                            for(auto& syncItem: otherRuleA->label) {
                                if(syncItem.second->getArg(1)) {
                                    otherNodeID = syncItem.first;
                                    otherLocalNodeID = syncItem.second->getArg(0);
                                    break;
                                }
                            }
                            if(otherNodeID<0) {
                                cc->reportError("Could not find sender of the other rule, bailing...");
                                return 1;
                            }
                            const DFT::Nodes::Node* otherNode = getNodeWithID(otherNodeID);
                            assert(otherNode);
                            
                            // The synchronization depends if THIS node uses
                            // dynamic activation or not. The prime example of
                            // this is the Spare gate.
                            // It also depends on the sending Node of the current
                            // activation rule (otherNode). Both have to use
                            // dynamic activation, because otherwise a Spare gate
                            // will not activate a child if it's activated by a
                            // static node, which is what is required.
                            // The fundamental issue is that the semantics of
                            // "using" or "claiming" a node is implicitely within
                            // activating it. This should be covered more
                            // thoroughly in the theory first, thus:
                            /// @todo Fix "using" semantics in activation"
                            if(node.usesDynamicActivation() && otherNode->usesDynamicActivation()) {
                                
                                
                                // If the THIS node used dynamic activation, the
                                // node is activated by an other parent.
                                
                                // Thus, add a synchronization item to the
                                // existing synchronization rule, specifying that
                                // the THIS node also receives a sent Activate.
                                otherRuleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,false)) );
                                cc->reportAction3("THIS node added, activation listening synchronization",VERBOSITY_RULEORIGINS);
                                
                                // This is not enough, because the other way
                                // around also has to be added: the other node
                                // wants to listen to Activates of the THIS node
                                // as well.
                                // Thus, we add a synchronization item to the
                                // new rule we create for the THIS node, specifying
                                // the other node wants to listen to activates of
                                // the THIS node.
                                ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(otherNodeID,syncActivate(otherLocalNodeID,false)) );
                                cc->reportAction3("Detected (other) dynamic activator `" + otherNode->getName() + "', added to sync rule",VERBOSITY_RULEORIGINS);
                                
                                // TODO: primary is a special case??????
                            } else {
                                
                                // If the THIS node does not use dynamic
                                // activation, we can simply add the THIS node
                                // as a sender to the other synchronization rule.
                                // FIXME: Possibly this is actually never wanted,
                                // as it could allow multiple senders to synchronize
                                // with each other.
                                //otherRuleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,true)) );
                            }
                        }
                    }
                    
                    // Add the child Node to the synchronization rule
                    // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                    ruleA->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncActivate(0,false)) );
                    cc->reportAction3("Child added to sync rule",VERBOSITY_RULEORIGINS);
                    
                    {
                        std::stringstream report;
                        report << "Added new activation sync rule: ";
                        printSyncLineShort(report,*ruleA);
                        cc->reportAction2(report.str(),VERBOSITY_RULES);
                    }
                    activationRules.push_back(ruleA);
                }
                
                /** FAIL RULE **/
                {
                    // Go through all the existing fail rules
                    std::vector<EXPSyncRule*>::iterator itf = failRules.begin();
                    bool areOtherRules = false;
                    for(;itf != failRules.end();++itf) {
                        
                        // If there is a rule that also synchronizes on the same node,
                        // we have come across a child with another parent.
                        if((*itf)->syncOnNode == &child) {
                            cc->reportAction3("Detected earlier fail rule",VERBOSITY_RULEORIGINS);
                            if(areOtherRules) {
                                cc->reportError("There should be only one FAIL rule per node, bailing...");
                                return 1;
                            }
                            areOtherRules = true;
                            // We can simply add the THIS node as a sender to
                            // the other synchronization rule.
                            // FIXME: Possibly this is actually never wanted,
                            // as it could allow multiple senders to synchronize
                            // with each other.
                            (*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(n+1)) );
                            cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
                        }
                    }
                    
                    // If there are other rules that synchronize on the same node,
                    // we do not need to synchronize any further
                    // FIXME: This is probably not true!
                    if(!areOtherRules) {
                        // Add the child Node to the synchronization rule
                        // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                        std::stringstream ss;
                        ss << "f_" << child.getTypeStr() << childID;
                        EXPSyncRule* ruleF = new EXPSyncRule(ss.str());
                        ruleF->syncOnNode = &child;
                        ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(n+1)) );
                        ruleF->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncFail(0)) );
                        {
                            std::stringstream report;
                            report << "Added new fail       sync rule: ";
                            printSyncLineShort(report,*ruleF);
                            cc->reportAction2(report.str(),VERBOSITY_RULES);
                        }
                        failRules.push_back(ruleF);
                    }
                }
                
                /** ONLINE Rules **/
                {
                    if(node.isRepairable()) {
                        // Go through all the existing fail rules
                        std::vector<EXPSyncRule*>::iterator itf = onlineRules.begin();
                        bool areOtherRules = false;
                        for(;itf != onlineRules.end();++itf) {
                            
                            // If there is a rule that also synchronizes on the same node,
                            // we have come across a child with another parent.
                            if((*itf)->syncOnNode == &child) {
                                cc->reportAction3("Detected earlier online rule",VERBOSITY_RULEORIGINS);
                                if(areOtherRules) {
                                    cc->reportError("There should be only one ONLINE rule per node, bailing...");
                                    return 1;
                                }
                                areOtherRules = true;
                                // We can simply add the THIS node as a sender to
                                // the other synchronization rule.
                                // FIXME: Possibly this is actually never wanted,
                                // as it could allow multiple senders to synchronize
                                // with each other.
                                (*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncFail(n+1)) );
                                cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
                            }
                        }
                        
                        // If there are other rules that synchronize on the same node,
                        // we do not need to synchronize any further
                        // FIXME: This is probably not true!
                        if(!areOtherRules) {
                            // Add the child Node to the synchronization rule
                            // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                            std::stringstream ss;
                            ss << "o_" << child.getTypeStr() << childID;
                            EXPSyncRule* ruleO = new EXPSyncRule(ss.str());
                            ruleO->syncOnNode = &child;
                            ruleO->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncOnline(n+1)) );
                            ruleO->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncOnline(0)) );
                            {
                                std::stringstream report;
                                report << "Added new online     sync rule: ";
                                printSyncLineShort(report,*ruleO);
                                cc->reportAction2(report.str(),VERBOSITY_RULES);
                            }
                            onlineRules.push_back(ruleO);
                        }
                    }
                }
                
            }else if(!DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::InspectionType) && !DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::ReplacementType)){
                
                /* REPAIR rule */
                {
                    // Go through all the existing repair rules
                    std::vector<EXPSyncRule*>::iterator itf = repairRules.begin();
                    bool areOtherRules = false;
                    for(;itf != repairRules.end();++itf) {
                        
                        // If there is a rule that also synchronizes on the same node,
                        // we have come across a child with another parent.
                        if((*itf)->syncOnNode == &child) {
                            cc->reportAction3("Detected earlier repair rule",VERBOSITY_RULEORIGINS);
                            if(areOtherRules) {
                                cc->reportError("There should be only one REPAIR rule per node, bailing...");
                                return 1;
                            }
                            areOtherRules = true;
                            // We can simply add the THIS node as a sender to
                            // the other synchronization rule.
                            // FIXME: Possibly this is actually never wanted,
                            // as it could allow multiple senders to synchronize
                            // with each other.
                            (*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepair(n+1)) );
                            cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
                        }
                    }
                    
                    // If there are other rules that synchronize on the same node,
                    // we do not need to synchronize any further
                    // FIXME: This is probably not true!
                    if(!areOtherRules) {
                        // Add the child Node to the synchronization rule
                        // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                        std::stringstream ss;
                        ss << "r_" << child.getTypeStr() << childID;
                        EXPSyncRule* ruleR = new EXPSyncRule(ss.str());
                        ruleR->syncOnNode = &child;
                        ruleR->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepair(n+1)) );
                        ruleR->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncRepair(0)) );
                        {
                            std::stringstream report;
                            report << "Added new repair     sync rule: ";
                            printSyncLineShort(report,*ruleR);
                            cc->reportAction2(report.str(),VERBOSITY_RULES);
                        }
                        repairRules.push_back(ruleR);
                    }
                }
                
                {
                    /** REPAIRING RULE for ND **/
                    if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::RepairUnitNdType)) {
                        // Create labelTo string
                        std::stringstream ss;
                        ss << "REPAIRING_" << child.getTypeStr() << childID;
                        EXPSyncRule* ruleRnd = new EXPSyncRule(ss.str(),false);
                        
                        std::stringstream report;
                        report << "New EXPSyncRule " << ss.str();
                        cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
                        
                        ss << DFT::DFTreeBCGNodeBuilder::GATE_REPAIRING;
                        if(!nameTop.empty()) ss << "_" << nameTop;
                        ruleRnd->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepairing(n+1)) );
                        repairingRules.push_back(ruleRnd);
                    }
                }
                
                /** REPAIRED RULE **/
                {
                    // Create labelTo string
                    std::stringstream ss;
                    ss << "rp_" << node.getTypeStr() << nodeID << "_" << child.getTypeStr() << childID;
                    EXPSyncRule* ruleRP = new EXPSyncRule(ss.str());
                    
                    std::stringstream report;
                    report << "New EXPSyncRule " << ss.str();
                    cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
                    
                    // Set synchronization node
                    ruleRP->syncOnNode = &child;
                    
                    // Add synchronization of THIS node to the synchronization rule
                    ruleRP->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepaired(n+1,true)) );
                    cc->reportAction3("THIS node added to sync rule",VERBOSITY_RULEORIGINS);
                    
                    // Go through all the existing repaired rules
                    std::vector<DFT::EXPSyncRule*>::iterator ita = repairedRules.begin();
                    for(;ita != repairedRules.end();++ita) {
                        
                        // If there is a rule that also synchronizes on the same node,
                        // we have come across a child with another parent.
                        // Note: this can be interesting if we allow assining several RUs to a BE
                        if((*ita)->syncOnNode == &child) {
                            EXPSyncRule* otherRuleRP = *ita;
                            std::stringstream report;
                            report << "Detected earlier maintenance rule: ";
                            printSyncLineShort(report,*otherRuleRP);
                            cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
                            
                            // First, we look up the sending Node of the
                            // current activation rule...
                            int otherNodeID = -1;
                            int otherLocalNodeID = -1;
                            for(auto& syncItem: otherRuleRP->label) {
                                if(syncItem.second->getArg(1)) {
                                    otherNodeID = syncItem.first;
                                    otherLocalNodeID = syncItem.second->getArg(0);
                                    break;
                                }
                            }
                            if(otherNodeID<0) {
                                cc->reportError("Could not find sender of the other rule, bailing...");
                                return 1;
                            }
                            const DFT::Nodes::Node* otherNode = getNodeWithID(otherNodeID);
                            assert(otherNode);
                            
                            // The synchronization depends if THIS node uses
                            // dynamic activation or not. The prime example of
                            // this is the Spare gate.
                            // It also depends on the sending Node of the current
                            // activation rule (otherNode). Both have to use
                            // dynamic activation, because otherwise a Spare gate
                            // will not activate a child if it's activated by a
                            // static node, which is what is required.
                            // The fundamental issue is that the semantics of
                            // "using" or "claiming" a node is implicitely within
                            // activating it. This should be covered more
                            // thoroughly in the theory first, thus:
                            /// @todo Fix "using" semantics in activation"
                            if(node.usesDynamicActivation() && otherNode->usesDynamicActivation()) {
                                
                                
                                // If the THIS node used dynamic activation, the
                                // node is activated by an other parent.
                                
                                // Thus, add a synchronization item to the
                                // existing synchronization rule, specifying that
                                // the THIS node also receives a sent Activate.
                                otherRuleRP->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,false)) );
                                cc->reportAction3("THIS node added, repaired listening synchronization",VERBOSITY_RULEORIGINS);
                                
                                // This is not enough, because the other way
                                // around also has to be added: the other node
                                // wants to listen to Activates of the THIS node
                                // as well.
                                // Thus, we add a synchronization item to the
                                // new rule we create for the THIS node, specifying
                                // the other node wants to listen to activates of
                                // the THIS node.
                                ruleRP->label.insert( pair<unsigned int,EXPSyncItem*>(otherNodeID,syncActivate(otherLocalNodeID,false)) );
                                cc->reportAction3("Detected (other) dynamic repaired `" + otherNode->getName() + "', added to sync rule",VERBOSITY_RULEORIGINS);
                                
                                // TODO: primary is a special case??????
                            } else {
                                
                                // If the THIS node does not use dynamic
                                // activation, we can simply add the THIS node
                                // as a sender to the other synchronization rule.
                                // FIXME: Possibly this is actually never wanted,
                                // as it could allow multiple senders to synchronize
                                // with each other.
                                //otherRuleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,true)) );
                            }
                        }
                    }
                    
                    // Add the child Node to the synchronization rule
                    // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                    ruleRP->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncRepaired(0,false)) );
                    cc->reportAction3("Child added to sync rule",VERBOSITY_RULEORIGINS);
                    
                    {
                        std::stringstream report;
                        report << "Added new repaired sync rule: ";
                        printSyncLineShort(report,*ruleRP);
                        cc->reportAction2(report.str(),VERBOSITY_RULES);
                    }
                    repairedRules.push_back(ruleRP);
                }
                
            } else if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::InspectionType)){
                /* INSPECTION rules */
                {
                    // Go through all the existing repair rules
                    std::vector<EXPSyncRule*>::iterator itf = inspectionRules.begin();
                    bool areOtherRules = false;
                    for(;itf != inspectionRules.end();++itf) {
                        
                        // If there is a rule that also synchronizes on the same node,
                        // we have come across a child with another parent.
                        if((*itf)->syncOnNode == &child) {
                            cc->reportAction3("Detected earlier inspection rule",VERBOSITY_RULEORIGINS);
                            if(areOtherRules) {
                                cc->reportError("There should be only one INSPECTION rule per node, bailing...");
                                return 1;
                            }
                            areOtherRules = true;
                            // We can simply add the THIS node as a sender to
                            // the other synchronization rule.
                            // FIXME: Possibly this is actually never wanted,
                            // as it could allow multiple senders to synchronize
                            // with each other.
                            (*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncInspection(n+1)) );
                            cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
                        }
                    }
                    
                    // If there are other rules that synchronize on the same node,
                    // we do not need to synchronize any further
                    // FIXME: This is probably not true!
                    if(!areOtherRules) {
                        // Add the child Node to the synchronization rule
                        // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                        std::stringstream ss;
                        ss << "insp_" << child.getTypeStr() << childID;
                        EXPSyncRule* ruleI = new EXPSyncRule(ss.str());
                        ruleI->syncOnNode = &child;
                        ruleI->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncInspection(n+1)) );
                        ruleI->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncInspection(0)) );
                        {
                            std::stringstream report;
                            report << "Added new inspection sync rule: ";
                            printSyncLineShort(report,*ruleI);
                            cc->reportAction2(report.str(),VERBOSITY_RULES);
                        }
                        inspectionRules.push_back(ruleI);
                    }
                }
                
                /* INSPECTED rules */
                {
                    // Go through all the existing repair rules
                    std::vector<EXPSyncRule*>::iterator itf = inspectedRules.begin();
                    bool areOtherRules = false;
                    for(;itf != inspectedRules.end();++itf) {
                        
                        // If there is a rule that also synchronizes on the same node,
                        // we have come across a child with another parent.
                        if((*itf)->syncOnNode == &child) {
                            cc->reportAction3("Detected earlier inspection rule",VERBOSITY_RULEORIGINS);
                            if(areOtherRules) {
                                cc->reportError("There should be only one INSPECTED rule per node, bailing...");
                                return 1;
                            }
                            areOtherRules = true;
                            // We can simply add the THIS node as a sender to
                            // the other synchronization rule.
                            // FIXME: Possibly this is actually never wanted,
                            // as it could allow multiple senders to synchronize
                            // with each other.
                            (*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncInspected(n+1)) );
                            cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
                        }
                    }
                    
                    // If there are other rules that synchronize on the same node,
                    // we do not need to synchronize any further
                    // FIXME: This is probably not true!
                    if(!areOtherRules) {
                        // Add the child Node to the synchronization rule
                        // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                        std::stringstream ss;
                        ss << "insp_done_" << child.getTypeStr() << childID;
                        EXPSyncRule* ruleInsp = new EXPSyncRule(ss.str());
                        ruleInsp->syncOnNode = &child;
                        ruleInsp->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncInspected(n+1)) );
                        ruleInsp->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncInspected(0)) );
                        {
                            std::stringstream report;
                            report << "Added new inspected sync rule: ";
                            printSyncLineShort(report,*ruleInsp);
                            cc->reportAction2(report.str(),VERBOSITY_RULES);
                        }
                        inspectedRules.push_back(ruleInsp);
                    }
                }
                
                /* RESET rules */
                {
                    // Go through all the existing repair rules
                    std::vector<EXPSyncRule*>::iterator itf = resetRules.begin();
                    bool areOtherRules = false;
                    for(;itf != resetRules.end();++itf) {
                        
                        // If there is a rule that also synchronizes on the same node,
                        // we have come across a child with another parent.
                        if((*itf)->syncOnNode == &child) {
                            cc->reportAction3("Detected earlier reset rule",VERBOSITY_RULEORIGINS);
                            if(areOtherRules) {
                                cc->reportError("There should be only one RESET rule per node, bailing...");
                                return 1;
                            }
                            areOtherRules = true;
                            // We can simply add the THIS node as a sender to
                            // the other synchronization rule.
                            // FIXME: Possibly this is actually never wanted,
                            // as it could allow multiple senders to synchronize
                            // with each other.
                            (*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncReset(n+1)) );
                            cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
                        }
                    }
                    
                    // If there are other rules that synchronize on the same node,
                    // we do not need to synchronize any further
                    // FIXME: This is probably not true!
                    if(!areOtherRules) {
                        // Add the child Node to the synchronization rule
                        // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                        std::stringstream ss;
                        ss << "reset_" << node.getTypeStr() << nodeID;
                        EXPSyncRule* ruleReset = new EXPSyncRule(ss.str());
                        ruleReset->syncOnNode = &child;
                        ruleReset->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncReset(0)) );
                        {
                            std::stringstream report;
                            report << "Added new reset sync rule: ";
                            printSyncLineShort(report,*ruleReset);
                            cc->reportAction2(report.str(),VERBOSITY_RULES);
                        }
                        resetRules.push_back(ruleReset);
                    }
                }



            } else if(DFT::Nodes::Node::typeMatch(node.getType(),DFT::Nodes::ReplacementType)){
                
                /* REPAIR rule */
                {
                    // Go through all the existing repair rules
                    std::vector<EXPSyncRule*>::iterator itf = repairRules.begin();
                    bool areOtherRules = false;
                    for(;itf != repairRules.end();++itf) {
                        
                        // If there is a rule that also synchronizes on the same node,
                        // we have come across a child with another parent.
                        if((*itf)->syncOnNode == &child) {
                            cc->reportAction3("Detected earlier repair rule",VERBOSITY_RULEORIGINS);
                            if(areOtherRules) {
                                cc->reportError("There should be only one REPAIR rule per node, bailing...");
                                return 1;
                            }
                            areOtherRules = true;
                            // We can simply add the THIS node as a sender to
                            // the other synchronization rule.
                            // FIXME: Possibly this is actually never wanted,
                            // as it could allow multiple senders to synchronize
                            // with each other.
                            (*itf)->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepair(n+1)) );
                            cc->reportAction3("THIS Node added to existing sync rule",VERBOSITY_RULEORIGINS);
                        }
                    }
                    
                    // If there are other rules that synchronize on the same node,
                    // we do not need to synchronize any further
                    // FIXME: This is probably not true!
                    if(!areOtherRules) {
                        // Add the child Node to the synchronization rule
                        // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                        std::stringstream ss;
                        ss << "r_" << child.getTypeStr() << childID;
                        EXPSyncRule* ruleR = new EXPSyncRule(ss.str());
                        ruleR->syncOnNode = &child;
                        ruleR->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepair(n+1)) );
                        ruleR->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncRepair(0)) );
                        {
                            std::stringstream report;
                            report << "Added new repair     sync rule: ";
                            printSyncLineShort(report,*ruleR);
                            cc->reportAction2(report.str(),VERBOSITY_RULES);
                        }
                        repairRules.push_back(ruleR);
                    }
                }
                
                /** REPAIRED RULE **/
                {
                    // Create labelTo string
                    std::stringstream ss;
                    ss << "rp_" << node.getTypeStr() << nodeID << "_" << child.getTypeStr() << childID;
                    EXPSyncRule* ruleRP = new EXPSyncRule(ss.str());
                    
                    std::stringstream report;
                    report << "New EXPSyncRule " << ss.str();
                    cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
                    
                    // Set synchronization node
                    ruleRP->syncOnNode = &child;
                    
                    // Add synchronization of THIS node to the synchronization rule
                    ruleRP->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncRepaired(n+1,true)) );
                    cc->reportAction3("THIS node added to sync rule",VERBOSITY_RULEORIGINS);
                    
                    // Go through all the existing repaired rules
                    std::vector<DFT::EXPSyncRule*>::iterator ita = repairedRules.begin();
                    for(;ita != repairedRules.end();++ita) {
                        
                        // If there is a rule that also synchronizes on the same node,
                        // we have come across a child with another parent.
                        // Note: this can be interesting if we allow assining several RUs to a BE
                        if((*ita)->syncOnNode == &child) {
                            EXPSyncRule* otherRuleRP = *ita;
                            std::stringstream report;
                            report << "Detected earlier maintenance rule: ";
                            printSyncLineShort(report,*otherRuleRP);
                            cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);
                            
                            // First, we look up the sending Node of the
                            // current activation rule...
                            int otherNodeID = -1;
                            int otherLocalNodeID = -1;
                            for(auto& syncItem: otherRuleRP->label) {
                                if(syncItem.second->getArg(1)) {
                                    otherNodeID = syncItem.first;
                                    otherLocalNodeID = syncItem.second->getArg(0);
                                    break;
                                }
                            }
                            if(otherNodeID<0) {
                                cc->reportError("Could not find sender of the other rule, bailing...");
                                return 1;
                            }
                            const DFT::Nodes::Node* otherNode = getNodeWithID(otherNodeID);
                            assert(otherNode);
                            
                            // The synchronization depends if THIS node uses
                            // dynamic activation or not. The prime example of
                            // this is the Spare gate.
                            // It also depends on the sending Node of the current
                            // activation rule (otherNode). Both have to use
                            // dynamic activation, because otherwise a Spare gate
                            // will not activate a child if it's activated by a
                            // static node, which is what is required.
                            // The fundamental issue is that the semantics of
                            // "using" or "claiming" a node is implicitely within
                            // activating it. This should be covered more
                            // thoroughly in the theory first, thus:
                            /// @todo Fix "using" semantics in activation"
                            if(node.usesDynamicActivation() && otherNode->usesDynamicActivation()) {
                                
                                
                                // If the THIS node used dynamic activation, the
                                // node is activated by an other parent.
                                
                                // Thus, add a synchronization item to the
                                // existing synchronization rule, specifying that
                                // the THIS node also receives a sent Activate.
                                otherRuleRP->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,false)) );
                                cc->reportAction3("THIS node added, repaired listening synchronization",VERBOSITY_RULEORIGINS);
                                
                                // This is not enough, because the other way
                                // around also has to be added: the other node
                                // wants to listen to Activates of the THIS node
                                // as well.
                                // Thus, we add a synchronization item to the
                                // new rule we create for the THIS node, specifying
                                // the other node wants to listen to activates of
                                // the THIS node.
                                ruleRP->label.insert( pair<unsigned int,EXPSyncItem*>(otherNodeID,syncActivate(otherLocalNodeID,false)) );
                                cc->reportAction3("Detected (other) dynamic repaired `" + otherNode->getName() + "', added to sync rule",VERBOSITY_RULEORIGINS);
                                
                                // TODO: primary is a special case??????
                            } else {
                                
                                // If the THIS node does not use dynamic
                                // activation, we can simply add the THIS node
                                // as a sender to the other synchronization rule.
                                // FIXME: Possibly this is actually never wanted,
                                // as it could allow multiple senders to synchronize
                                // with each other.
                                //otherRuleA->label.insert( pair<unsigned int,EXPSyncItem*>(nodeID,syncActivate(n+1,true)) );
                            }
                        }
                    }
                    
                    // Add the child Node to the synchronization rule
                    // Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
                    ruleRP->label.insert( pair<unsigned int,EXPSyncItem*>(childID,syncRepaired(0,false)) );
                    cc->reportAction3("Child added to sync rule",VERBOSITY_RULEORIGINS);
                    
                    {
                        std::stringstream report;
                        report << "Added new repaired sync rule: ";
                        printSyncLineShort(report,*ruleRP);
                        cc->reportAction2(report.str(),VERBOSITY_RULES);
                    }
                    repairedRules.push_back(ruleRP);
                }
                
            }

            
        }
        
        /* Generate node-specific rules for the node */
        
        switch(node.getType()) {
            case DFT::Nodes::BasicEventType: {
                //			const DFT::Nodes::BasicEvent* be = static_cast<const DFT::Nodes::BasicEvent*>(&node);
                //			createSyncRuleBE(syncRules,*be,nodeID);
                break;
            }
            case DFT::Nodes::GateType: {
                cc->reportError("A gate should have a specialized type, not the general GateType");
                break;
            }
            case DFT::Nodes::GatePhasedOrType: {
                cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
                break;
            }
            case DFT::Nodes::GateOrType: {
                const DFT::Nodes::GateOr* g = static_cast<const DFT::Nodes::GateOr*>(&node);
                createSyncRuleGateOr(activationRules,failRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::GateAndType: {
                const DFT::Nodes::GateAnd* g = static_cast<const DFT::Nodes::GateAnd*>(&node);
                createSyncRuleGateAnd(activationRules,failRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::GateSAndType: {
                const DFT::Nodes::GateSAnd* g = static_cast<const DFT::Nodes::GateSAnd*>(&node);
                createSyncRuleGateSAnd(activationRules,failRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::GateHSPType: {
                cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
                break;
            }
            case DFT::Nodes::GateWSPType: {
                const DFT::Nodes::GateWSP* g = static_cast<const DFT::Nodes::GateWSP*>(&node);
                createSyncRuleGateWSP(activationRules,failRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::GateCSPType: {
                cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
                break;
            }
            case DFT::Nodes::GatePAndType: {
                const DFT::Nodes::GatePAnd* g = static_cast<const DFT::Nodes::GatePAnd*>(&node);
                createSyncRuleGatePAnd(activationRules,failRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::GatePorType: {
                const DFT::Nodes::GatePor* g = static_cast<const DFT::Nodes::GatePor*>(&node);
                createSyncRuleGatePor(activationRules,failRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::GateSeqType: {
                cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
                break;
            }
            case DFT::Nodes::GateVotingType: {
                const DFT::Nodes::GateVoting* g = static_cast<const DFT::Nodes::GateVoting*>(&node);
                createSyncRuleGateVoting(activationRules,failRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::GateFDEPType: {
                const DFT::Nodes::GateFDEP* g = static_cast<const DFT::Nodes::GateFDEP*>(&node);
                createSyncRuleGateFDEP(activationRules,failRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::GateTransferType: {
                cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
                break;
            }
            case DFT::Nodes::RepairUnitType: {
                const DFT::Nodes::RepairUnit* g = static_cast<const DFT::Nodes::RepairUnit*>(&node);
                createSyncRuleRepairUnit(repairRules,repairedRules,repairingRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::RepairUnitFcfsType: {
                const DFT::Nodes::RepairUnit* g = static_cast<const DFT::Nodes::RepairUnit*>(&node);
                createSyncRuleRepairUnit(repairRules,repairedRules,repairingRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::RepairUnitNdType: {
                const DFT::Nodes::RepairUnit* g = static_cast<const DFT::Nodes::RepairUnit*>(&node);
                createSyncRuleRepairUnit(repairRules,repairedRules,repairingRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::RepairUnitPrioType: {
                const DFT::Nodes::RepairUnit* g = static_cast<const DFT::Nodes::RepairUnit*>(&node);
                createSyncRuleRepairUnit(repairRules,repairedRules,repairingRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::InspectionType: {
                const DFT::Nodes::Inspection* g = static_cast<const DFT::Nodes::Inspection*>(&node);
                createSyncRuleInspection(inspectionRules,repairRules,repairedRules,*g,nodeID);
                break;
            }
            case DFT::Nodes::ReplacementType: {
                const DFT::Nodes::Replacement* g = static_cast<const DFT::Nodes::Replacement*>(&node);
                createSyncRuleReplacement(repairRules,repairedRules,*g,nodeID);
                break;
            }
            default: {
                cc->reportError("UnknownNode");
                break;
            }
        }
        
        return 0;
    }

    void DFT::DFTreeEXPBuilder::calculateColumnWidths(vector<unsigned int>& columnWidths,const vector<DFT::EXPSyncRule*>& syncRules) {
		for(size_t s=0; s<syncRules.size(); ++s) {
			const DFT::EXPSyncRule& rule = *syncRules.at(s);
			std::map<unsigned int,EXPSyncItem*>::const_iterator it = rule.label.begin();
			for(;it!=rule.label.end();++it) {
				assert( (0<=it->first) );
				assert( (it->first<columnWidths.size()) );
				std::string s = it->second->toStringQuoted();
				if(s.length() > columnWidths[it->first]) {
					columnWidths[it->first] = s.length();
				}
			}
		}
	}
