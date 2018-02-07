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
		if (syncIdx.first > 0) {
			const DFT::Nodes::Node* node = getNodeWithID(syncIdx.first);
			if(node)
				stream << node->getName();
			else
				stream << "error";
		} else {
			stream << "TopLevel";
		}
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
	} else if (be.getLambda()>0 || be.getProb() > 0) {
		double l = be.getLambda();
		double rateFailSafe = 0;
		if (l < 0) {
			/* Purely probabilistic BE. Assign arbitrary rate
			 * since only time-unbounded properties make sense
			 * anyway.
			 */
			l = be.getProb();
			rateFailSafe = 1 - l;
		} else {
			rateFailSafe = l / be.getProb() - l;
		}

		ss << "total rename ";
		// Insert lambda value
		ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !1 !2\" -> \"rate " << l << "\"";
		if (rateFailSafe != 0)
			ss << ", \"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !2 !2\" -> \"rate " << rateFailSafe << "\"";
	
		// Insert mu value (only for non-cold BE's)
		if(be.getMu()>0) {
			double mu = be.getMu();
			rateFailSafe = mu / be.getProb() - mu;
			ss << ", \"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !1 !1\" -> \"rate " << mu << "\"";
			if (rateFailSafe != 0)
				ss << ", \"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !2 !1\" -> \"rate " << rateFailSafe << "\"";
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
    ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_PERIOD << " !1 !" << "\" -> \"rate " << rep.getLambda() << "\"";
    
    ss << " in \"";
    ss << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(rep);
    ss << ".bcg\" end rename";
    
    return ss.str();
}

void DFT::DFTreeEXPBuilder::printSyncLine(const EXPSyncRule& rule, const vector<unsigned int>& columnWidths) {
	std::map<unsigned int,std::shared_ptr<EXPSyncItem>>::const_iterator it = rule.label.begin();
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
	while(c<dft->getNodes().size() + 1) {
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
		nodeIDs.insert(pair<const DFT::Nodes::Node*, unsigned int>(node, i + 1));
	}

	if(ok) {
		// Build the EXP file
		svl_header.clearAll();
		svl_body.clearAll();
		exp_header.clearAll();
		exp_body.clearAll();
		
		vector<DFT::EXPSyncRule> activationRules;
		vector<DFT::EXPSyncRule> failRules;
		// new rules for repair, repaired and online
		vector<DFT::EXPSyncRule> repairRules;
		vector<DFT::EXPSyncRule> repairedRules;
		vector<DFT::EXPSyncRule> repairingRules;
		vector<DFT::EXPSyncRule> onlineRules;
        // extra inspection rules
        vector<DFT::EXPSyncRule> inspectionRules;
		// Rules for detection of modelling errors.
        vector<DFT::EXPSyncRule> impossibleRules;

        parseDFT(impossibleRules, activationRules,failRules,repairRules,repairedRules,repairingRules,onlineRules,inspectionRules);
        buildEXPBody(impossibleRules, activationRules,failRules,repairRules,repairedRules,repairingRules,onlineRules,inspectionRules);

		// Build SVL file
		svl_body << "%EXP_OPEN_OPTIONS=\"-rate\";" << svl_body.applypostfix;
		svl_body << "%BCG_MIN_OPTIONS=\"-rate -self -epsilon 5e-324\";" << svl_body.applypostfix;
		svl_body << "\"" << nameBCG << "\" = smart stochastic branching reduction of \"" << nameEXP << "\";" << svl_body.applypostfix;
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
	assert(0 < id && id <= dft->getNodes().size());
	return dft->getNodes().at(id - 1);
}

int DFT::DFTreeEXPBuilder::parseDFT(
			vector<DFT::EXPSyncRule>& impossibleRules,
			vector<DFT::EXPSyncRule>& activationRules,
			vector<DFT::EXPSyncRule>& failRules,
			vector<DFT::EXPSyncRule>& repairRules,
			vector<DFT::EXPSyncRule>& repairedRules,
			vector<DFT::EXPSyncRule>& repairingRules,
			vector<DFT::EXPSyncRule>& onlineRules,
			vector<DFT::EXPSyncRule>& inspectionRules)
{
    
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
                createSyncRule(impossibleRules, activationRules,failRules,repairRules,repairedRules,repairingRules,onlineRules,inspectionRules,gate,getIDOfNode(gate));
            } else {
				unsigned int nodeID = getIDOfNode(**it);
				/* Impossible (model-error) rule */
				std::string iName("IMPOSSIBLE_");
				iName += (*it)->getTypeStr();
				iName += std::to_string(nodeID);
				DFT::EXPSyncRule ruleI(iName);
				ruleI.insertLabel(nodeID, syncImpossible());
				impossibleRules.push_back(ruleI);
			}
        }
    }
    return 0;
}

void DFT::DFTreeEXPBuilder::writeHideLines(vector<DFT::EXPSyncRule>& rules)
{
	for(size_t s = 0; s < rules.size(); ++s) {
		EXPSyncRule& rule = rules[s];
		if(rule.hideToLabel) {
			exp_body << exp_body.applyprefix << rule.toLabel << "," << exp_body.applypostfix;
		}
	}
}

void DFT::DFTreeEXPBuilder::writeRules(vector<DFT::EXPSyncRule>& rules,
									   vector<unsigned int> columnWidths)
{
	for(size_t s = 0; s < rules.size(); ++s) {
		exp_body << exp_body.applyprefix;
		printSyncLine(rules[s],columnWidths);
		exp_body << ",";
		exp_body << exp_body.applypostfix;
	}
}

int DFT::DFTreeEXPBuilder::buildEXPBody(
			vector<DFT::EXPSyncRule>& impossibleRules,
			vector<DFT::EXPSyncRule>& activationRules,
			vector<DFT::EXPSyncRule>& failRules,
			vector<DFT::EXPSyncRule>& repairRules,
			vector<DFT::EXPSyncRule>& repairedRules,
			vector<DFT::EXPSyncRule>& repairingRules,
			vector<DFT::EXPSyncRule>& onlineRules,
			vector<DFT::EXPSyncRule>& inspectionRules)
{
    /* Generate the EXP based on the generated synchronization rules */
    exp_body.clearAll();
    exp_body << exp_body.applyprefix << "(* Number of rules: " << (activationRules.size()+failRules.size()+repairRules.size()+repairedRules.size()+repairingRules.size()+onlineRules.size()+inspectionRules.size());
   	exp_body << "*)" << exp_body.applypostfix;
    exp_body << exp_body.applyprefix << "hide" << exp_body.applypostfix;
    exp_body.indent();
    
	writeHideLines(activationRules);
	writeHideLines(repairRules);
	writeHideLines(repairedRules);
	writeHideLines(onlineRules);
	writeHideLines(inspectionRules);
	/* Fail rules are special since we need to omit the last trailing
	 * comma.
	 */
    for(size_t s=0; s<failRules.size(); ++s) {
        EXPSyncRule& rule = failRules.at(s);
        if(rule.hideToLabel) {
            exp_body << exp_body.applyprefix << rule.toLabel;
            if(s<failRules.size()-1) exp_body << ",";
            exp_body << exp_body.applypostfix;
        }
    }
    
    exp_body.outdent();
    exp_body.appendLine("in");
    exp_body.indent();

	// Synchronization rules
	vector<unsigned int> columnWidths(dft->getNodes().size() + 1, 0);
	calculateColumnWidths(columnWidths,impossibleRules);
	calculateColumnWidths(columnWidths,activationRules);
	calculateColumnWidths(columnWidths,failRules);
	calculateColumnWidths(columnWidths,repairRules);
	calculateColumnWidths(columnWidths,repairedRules);
	calculateColumnWidths(columnWidths,onlineRules);
	calculateColumnWidths(columnWidths,inspectionRules);
	
	exp_body << exp_body.applyprefix << "label par using" << exp_body.applypostfix;
	exp_body << exp_body.applyprefix << "(*\t";
	exp_body.outlineLeftNext(columnWidths[0],' ');
	exp_body << exp_body._push << "tle" << exp_body._pop;

	std::vector<DFT::Nodes::Node*>::const_iterator it = dft->getNodes().begin();
	for(int c = 0; it!=dft->getNodes().end(); ++it,++c) {
		exp_body << "   ";
		exp_body.outlineLeftNext(columnWidths[c + 1],' ');
		exp_body << exp_body._push << (*it)->getTypeStr() << getIDOfNode(**it)
			<< exp_body._pop;
	}
	exp_body << " *)" << exp_body.applypostfix;
	
	exp_body.indent();
	writeRules(activationRules, columnWidths);
	writeRules(repairRules, columnWidths);
	writeRules(repairingRules, columnWidths);
	writeRules(repairedRules, columnWidths);
	writeRules(onlineRules, columnWidths);
	writeRules(inspectionRules, columnWidths);
	writeRules(impossibleRules, columnWidths);
	// Generate fail rules (again omitting last trailing comma).
	for(size_t s=0; s<failRules.size(); ++s) {
		exp_body << exp_body.applyprefix;
		printSyncLine(failRules.at(s),columnWidths);
		if(s<failRules.size()-1)
			exp_body << ",";
		exp_body << exp_body.applypostfix;
	}
	exp_body.outdent();
	
	// Generate the parallel composition of all the nodes
	exp_body << exp_body.applyprefix << "in" << exp_body.applypostfix;
	exp_body.indent();
	exp_body << exp_body.applyprefix;
	exp_body << "\"" << bcgRoot << "toplevel.bcg\"";
	exp_body << exp_body.applypostfix;
	int c=0;
	it = dft->getNodes().begin();
	for(it = dft->getNodes().begin(); it!=dft->getNodes().end(); ++it, ++c) {
		const DFT::Nodes::Node& node = **it;
		exp_body << exp_body.applyprefix << "||" << exp_body.applypostfix;
		if(node.isBasicEvent()) {
			const DFT::Nodes::BasicEvent *be;
		   	be = static_cast<const DFT::Nodes::BasicEvent*>(&node);
			exp_body << exp_body.applyprefix << getBEProc(*be)
			         << exp_body.applypostfix;
		} else if (node.isGate()) {
			if (DFT::Nodes::Node::typeMatch(node.getType(),
											DFT::Nodes::RepairUnitType)
				|| DFT::Nodes::Node::typeMatch(node.getType(),
											   DFT::Nodes::RepairUnitFcfsType)
				|| DFT::Nodes::Node::typeMatch(node.getType(),
											   DFT::Nodes::RepairUnitPrioType)
				|| DFT::Nodes::Node::typeMatch(node.getType(),
											   DFT::Nodes::RepairUnitNdType))
			{
				const DFT::Nodes::Gate *ru;
			   	ru = static_cast<const DFT::Nodes::Gate*>(&node);
				exp_body << exp_body.applyprefix << getRUProc(*ru)
				         << exp_body.applypostfix;
			} else if (DFT::Nodes::Node::typeMatch(node.getType(),
												   DFT::Nodes::InspectionType))
			{
				const DFT::Nodes::Inspection *insp;
			   	insp = static_cast<const DFT::Nodes::Inspection*>(&node);
				exp_body << exp_body.applyprefix << getINSPProc(*insp)
				         << exp_body.applypostfix;
			} else if (DFT::Nodes::Node::typeMatch(node.getType(),
												   DFT::Nodes::ReplacementType))
			{
				const DFT::Nodes::Replacement *rep;
			   	rep = static_cast<const DFT::Nodes::Replacement*>(&node);
				exp_body << exp_body.applyprefix << getREPProc(*rep)
				         << exp_body.applypostfix;
			} else {
				exp_body << exp_body.applyprefix << "\"" << bcgRoot
				         << DFT::DFTreeBCGNodeBuilder::getFileForNode(node)
				         << ".bcg\"" << exp_body.applypostfix;
			}
		} else {
			assert(0 && "buildEXPBody(): Unknown node type");
		}
	}
	exp_body.outdent();
	exp_body << exp_body.applyprefix << "end par" << exp_body.applypostfix;
    exp_body.outdent();
    exp_body.appendLine("end hide");
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
int DFT::DFTreeEXPBuilder::createSyncRuleGateFDEP(vector<DFT::EXPSyncRule>& activationRules, vector<DFT::EXPSyncRule>& failRules, const DFT::Nodes::GateFDEP& node, unsigned int nodeID) {

	// Loop over all the dependers
	cc->reportAction3("FDEP Dependencies of THIS node...",VERBOSITY_RULEORIGINS);
	for(int dependerLocalID=0; dependerLocalID<(int)node.getDependers().size(); ++dependerLocalID) {
		DFT::Nodes::Node* depender = node.getDependers()[dependerLocalID];
		unsigned int dependerID = getIDOfNode(*depender);

		cc->reportAction3("Depender `" + depender->getName() + "'",VERBOSITY_RULEORIGINS);

		// Create a new failSyncRule
		std::stringstream ss;
		ss << "f_" << node.getTypeStr() << nodeID << "_" << depender->getTypeStr() << dependerID;
		EXPSyncRule ruleF(ss.str());

		// Add the depender to the synchronization (+2, because in LNT the depender list starts at 2)
		ruleF.insertLabel(nodeID, syncFail(dependerLocalID + 2));
		cc->reportAction3("Added fail rule, notifying:",VERBOSITY_RULEORIGINS);

		// Loop over the parents of the depender
		for(DFT::Nodes::Node* depParent: depender->getParents()) {
			// Obtain the localChildID of the depender seen from this parent
			unsigned int parentID = getIDOfNode(*depParent);
			int localChildID = getLocalIDOfNode(depParent,depender);
			assert(localChildID>=0 && "depender is not a child of its parent");

			// Add the parent to the synchronization rule, hooking into the FAIL of the depender,
			// making it appear to the parent that the child failed (+1, because in LNT the child list starts at 1)
			ruleF.insertLabel(parentID, syncFail(localChildID + 1));
			cc->reportAction3("  Node `" + depParent->getName() + "'",VERBOSITY_RULEORIGINS);
		}

		// Add it to the list of rules
		failRules.push_back(ruleF);
		{
			std::stringstream report;
			report << "Added new fail       sync rule: ";
			printSyncLineShort(report,ruleF);
			cc->reportAction2(report.str(),VERBOSITY_RULES);
		}
	}

	return 0;
}

int DFT::DFTreeEXPBuilder::createSyncRuleTop(
		vector<DFT::EXPSyncRule>& activationRules,
		vector<DFT::EXPSyncRule>& failRules,
		vector<DFT::EXPSyncRule>& onlineRules)
{
	std::stringstream ss;

	unsigned int topNode = nodeIDs[dft->getTopNode()];

	// Generate the Top Node Activate rule
	ss << DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE;
	if(!nameTop.empty())
		ss << "_" << nameTop;
	DFT::EXPSyncRule ruleA(ss.str(), true);

	ruleA.insertLabel(topNode, syncActivate(0, false));
	ruleA.insertLabel(0, syncActivate(0, true));

	// Generate the FDEP Node Activate rule
	int c=0;
	std::vector<DFT::Nodes::Node*>::iterator it;
	for(it = dft->getNodes().begin(); it!=dft->getNodes().end();++it,++c) {
		const DFT::Nodes::Node& node = **it;
		if(node.isGate()) {
			if(node.matchesType(DFT::Nodes::GateFDEPType)) {
				ruleA.insertLabel(c, syncActivate(0, false));
			}
		}
	}

	std::stringstream report;
	report << "New EXPSyncRule " << ss.str();
	cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);

	// Generate the Top Node Fail rule
	ss.str(std::string());
	ss << DFT::DFTreeBCGNodeBuilder::GATE_FAIL;
	if(!nameTop.empty())
		ss << "_" << nameTop;
	DFT::EXPSyncRule ruleF(ss.str(),false);
	ruleF.syncOnNode = dft->getTopNode();
	ruleF.insertLabel(topNode, syncFail(0));

	cc->reportAction("Creating synchronization rules for Top node",VERBOSITY_FLOW);
	if(dft->getTopNode()->isRepairable()){
		// Generate the Top Node Online rule
		ss.str("");
		ss << DFT::DFTreeBCGNodeBuilder::GATE_ONLINE;
		DFT::EXPSyncRule ruleO(ss.str(),false);
		if(!nameTop.empty())
			ss << "_" << nameTop;
		ruleO.insertLabel(topNode, syncOnline(0));
		onlineRules.push_back(ruleO);
		ss.str("Added new online     sync rule: ");
		printSyncLineShort(ss, ruleO);
		cc->reportAction2(ss.str(), VERBOSITY_RULES);
	}

	// Add the generated rules to the lists
	activationRules.push_back(ruleA);
	failRules.push_back(ruleF);

	ss.str("Added new activation sync rule: ");
	printSyncLineShort(ss, ruleA);
	cc->reportAction2(ss.str(), VERBOSITY_RULES);

	ss.str("Added new fail       sync rule: ");
	printSyncLineShort(ss, ruleF);
	cc->reportAction2(ss.str(), VERBOSITY_RULES);

	return 0;
}

/** Add a rule between a child and any of its parents. */
void DFT::DFTreeEXPBuilder::addAnycastRule(vector<DFT::EXPSyncRule> &rules,
											 const DFT::Nodes::Gate &node,
											 EXPSyncItem *nodeSignal,
											 EXPSyncItem *childSignal,
											 std::string name_prefix,
											 unsigned int childNum)
{
	const DFT::Nodes::Node &child = *node.getChildren().at(childNum);
	unsigned int childID = nodeIDs[&child];
	unsigned int nodeID = nodeIDs[&node];

	std::stringstream ss;
	ss << name_prefix << '_' << child.getTypeStr() << childID;
	EXPSyncRule rule(ss.str());
	rule.syncOnNode = &child;
	rule.insertLabel(nodeID, nodeSignal);
	rule.insertLabel(childID, childSignal);
	std::stringstream report("Added new broadcast sync rule: ");
	printSyncLineShort(report, rule);
	cc->reportAction2(report.str(),VERBOSITY_RULES);
	rules.push_back(rule);
}
/** Add a rule broadcast from a child to all its parents. */
void DFT::DFTreeEXPBuilder::addBroadcastRule(vector<DFT::EXPSyncRule> &rules,
					     const DFT::Nodes::Gate &node,
					     EXPSyncItem *nodeSignal,
					     EXPSyncItem *childSignal,
					     std::string name_prefix,
					     unsigned int childNum)
{
	const DFT::Nodes::Node *child = node.getChildren().at(childNum);
	unsigned int nodeID = nodeIDs[&node];
	for (auto &rule : rules) {
		// If there is a rule that also synchronizes on the same node,
		// we have come across a child with another parent.
		if(rule.syncOnNode == child) {
			cc->reportAction3("Found earlier fail rule",VERBOSITY_RULEORIGINS);
			rule.insertLabel(nodeID, nodeSignal);
			return;
		}
	}

	addAnycastRule(rules, node, nodeSignal, childSignal, name_prefix, childNum);
}

int DFT::DFTreeEXPBuilder::createSyncRule(
			vector<DFT::EXPSyncRule>& impossibleRules,
			vector<DFT::EXPSyncRule>& activationRules,
			vector<DFT::EXPSyncRule>& failRules,
			vector<DFT::EXPSyncRule>& repairRules,
			vector<DFT::EXPSyncRule>& repairedRules,
			vector<DFT::EXPSyncRule>& repairingRules,
			vector<DFT::EXPSyncRule>& onlineRules,
			vector<DFT::EXPSyncRule>& inspectionRules,
			const DFT::Nodes::Gate& node,
			unsigned int nodeID)
{
	// Go through all the children
	for(size_t n = 0; n<node.getChildren().size(); ++n) {
		// Get the current child and associated childID
		const DFT::Nodes::Node *child = node.getChildren().at(n);
		unsigned int childID = nodeIDs[child];

		cc->reportAction2("Child `" + child->getName() + "'"
						  + (child->usesDynamicActivation()?" (dynact)":"")
						  + " ...",
						  VERBOSITY_RULES);

		// ask if we have a repair unit (if it is the case we don't have to handle activation and fail) same for inspection and replacement
		if (!node.matchesType(DFT::Nodes::RepairUnitType)
			&& !node.matchesType(DFT::Nodes::RepairUnitFcfsType)
			&& !node.matchesType(DFT::Nodes::RepairUnitPrioType)
			&& !node.matchesType(DFT::Nodes::RepairUnitNdType)
			&& !node.matchesType(DFT::Nodes::InspectionType)
			&& !node.matchesType(DFT::Nodes::ReplacementType))
		{
			/** ACTIVATION RULE **/
			std::stringstream ss;
			ss << node.getTypeStr() << nodeID << "_" << child->getTypeStr()
			   << childID;
			EXPSyncRule ruleA("a_" + ss.str());
			EXPSyncRule ruleD("d_" + ss.str());

			std::stringstream report;
			report << "New EXPSyncRule " << ss.str();
			cc->reportAction3(report.str(),VERBOSITY_RULEORIGINS);

			// Set synchronization node
			ruleA.syncOnNode = child;
			ruleD.syncOnNode = child;

			// Add synchronization of THIS node to the synchronization rule
			ruleA.insertLabel(nodeID, syncActivate(n+1, true));
			ruleD.insertLabel(nodeID, syncDeactivate(n+1, true));
			cc->reportAction3("THIS node added to sync rule",
							  VERBOSITY_RULEORIGINS);

			// Go through all the existing activation rules
			for (auto &otherRule : activationRules) {
				// If there is a rule that also synchronizes on the same node,
				// we have come across a child with another parent.
				if(otherRule.syncOnNode != child)
					continue;
				std::stringstream report;
				report << "Detected earlier activation rule: ";
				printSyncLineShort(report, otherRule);
				cc->reportAction3(report.str(), VERBOSITY_RULEORIGINS);

				// First, we look up the sending Node of the
				// current activation rule...
				int otherNodeID = -1;
				int otherLocalNodeID = -1;
				for(auto& syncItem: otherRule.label) {
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
				if(node.usesDynamicActivation()
				   && otherNode->usesDynamicActivation())
				{
					// If the THIS node used dynamic activation, the
					// node is activated by an other parent.
					// Thus, add a synchronization item to the
					// existing synchronization rule, specifying that
					// the THIS node also receives a sent Activate.
					if (otherRule.toLabel[0] == 'd')
						otherRule.insertLabel(nodeID, syncDeactivate(n+1, false));
					else
						otherRule.insertLabel(nodeID, syncActivate(n+1, false));
					cc->reportAction3("THIS node added, activation listening synchronization",VERBOSITY_RULEORIGINS);

					// This is not enough, because the other way
					// around also has to be added: the other node
					// wants to listen to Activates of the THIS node
					// as well.
					// Thus, we add a synchronization item to the
					// new rule we create for the THIS node, specifying
					// the other node wants to listen to activates of
					// the THIS node.
					ruleA.insertLabel(otherNodeID, syncActivate(otherLocalNodeID, false));
					ruleD.insertLabel(otherNodeID, syncDeactivate(otherLocalNodeID, false));
					cc->reportAction3("Detected (other) dynamic activator `" + otherNode->getName() + "', added to sync rule",VERBOSITY_RULEORIGINS);

					// TODO: primary is a special case??????
				}
			}

			// Add the child Node to the synchronization rule
			// Create synchronization rules a_<nodetype><nodeid>_<childtype><childid>
			ruleA.insertLabel(childID, syncActivate(0, false));
			ruleD.insertLabel(childID, syncDeactivate(0, false));
			cc->reportAction3("Child added to sync rule",VERBOSITY_RULEORIGINS);
			report.str("Added new activation sync rule: ");
			printSyncLineShort(report,ruleA);
			cc->reportAction2(report.str(),VERBOSITY_RULES);
			activationRules.push_back(ruleA);
			activationRules.push_back(ruleD);

			addBroadcastRule(failRules, node, syncFail(n + 1), syncFail(0),
							 "f_", n);
			/** ONLINE Rules **/
			if (node.isRepairable()) {
				addBroadcastRule(onlineRules, node, syncOnline(n + 1),
								 syncOnline(0), "o_", n);
			}
		} else if (!node.matchesType(DFT::Nodes::InspectionType)
				   && !node.matchesType(DFT::Nodes::ReplacementType))
		{
			addBroadcastRule(repairRules, node, syncRepair(n + 1),
							 syncRepair(0), "rep_", n);
			/** REPAIRING RULE for ND **/
			if(node.matchesType(DFT::Nodes::RepairUnitNdType)) {
				addBroadcastRule(repairingRules, node, syncRepairing(n + 1),
								 syncRepairing(0), "repi_", n);
			}
			addBroadcastRule(repairedRules, node, syncRepaired(n + 1, true),
							 syncRepaired(0, false), "repd_", n);
		} else if(node.matchesType(DFT::Nodes::InspectionType)) {
			addBroadcastRule(inspectionRules, node, syncInspection(n+1),
							 syncInspection(0), "insp_", n);
			addBroadcastRule(failRules, node, syncInspection(n+1),
							 syncFail(0), "inspf_", n);
			addAnycastRule(repairRules, node, syncRepair(n + 1),
						   syncRepaired(0, false), "rep_", n);

		} else if(DFT::Nodes::Node::typeMatch(node.getType(),
											  DFT::Nodes::ReplacementType))
		{
			addAnycastRule(repairRules, node, syncRepair(n + 1),
						   syncRepair(0), "rep_", n);
			addBroadcastRule(repairedRules, node, syncRepaired(n+1, true),
							 syncRepaired(0, false), "rpd_", n);
		}
	}

	/* Generate node-specific rules for the node */

	switch(node.getType()) {
		const DFT::Nodes::GateFDEP* fdep;
	case DFT::Nodes::GateType:
		cc->reportError("A gate should have a specialized type, not the general GateType");
		break;
	case DFT::Nodes::GatePhasedOrType:
		cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
		break;
	case DFT::Nodes::GateHSPType:
		cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
		break;
	case DFT::Nodes::GateCSPType:
		cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
		break;
	case DFT::Nodes::GateFDEPType:
		fdep = static_cast<const DFT::Nodes::GateFDEP*>(&node);
		createSyncRuleGateFDEP(activationRules,failRules,*fdep,nodeID);
		break;
	case DFT::Nodes::GateTransferType:
		cc->reportErrorAt(node.getLocation(),"DFTreeEXPBuilder: unsupported gate: " + node.getTypeStr());
		break;
	default:
		break;
	}

	return 0;
}

void DFT::DFTreeEXPBuilder::calculateColumnWidths(
		vector<unsigned int>& columnWidths,
		const vector<DFT::EXPSyncRule>& syncRules)
{
	for (const DFT::EXPSyncRule &rule : syncRules) {
		std::map<unsigned int, std::shared_ptr<EXPSyncItem>>::const_iterator it = rule.label.begin();
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
