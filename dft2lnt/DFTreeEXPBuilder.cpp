/*
 * DFTreeEXPBuilder.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
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
		else stream << " ¦ ";
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
		ss << "\"A\" -> \"" << DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE << " !0 !FALSE\"";
		ss << ", ";
		ss << "\"F\" -> \"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << " !0\"";
		//ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE << " !0 !FALSE\" -> \"A\"";
		//ss << ", ";
		//ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_FAIL << " !0\" -> \"F\"";
		ss << " in \"";
		ss << be.getFileToEmbed();
		ss << "\" end rename";
	} else {
		ss << "total rename ";
		// Insert lambda value
		ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !1 !2\" -> \"rate " << be.getLambda() << "\"";
	
		// Insert mu value (only for non-cold BE's)
		if(be.getMu()>0) {
			ss << ", ";
			ss << "\"" << DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL << " !1 !1\" -> \"rate " << be.getMu()     << "\"";
		}
		ss << " in \"";
		ss << bcgRoot << DFT::DFTreeBCGNodeBuilder::getFileForNode(be);
		ss << ".bcg\" end rename";
	}
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
		
		parseDFT(activationRules,failRules);
		buildEXPHeader(activationRules,failRules);
		buildEXPBody(activationRules,failRules);
		
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

		exp_body << exp_body.applyprefix << "label par" << exp_body.applypostfix;
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

	int DFT::DFTreeEXPBuilder::createSyncRuleBE(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::BasicEvent& node, unsigned int nodeID) {
		return 0;
	}
	int DFT::DFTreeEXPBuilder::createSyncRuleGateOr(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateOr& node, unsigned int nodeID) {
		return 0;
	}
	int DFT::DFTreeEXPBuilder::createSyncRuleGateAnd(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateAnd& node, unsigned int nodeID) {
		return 0;
	}
	int DFT::DFTreeEXPBuilder::createSyncRuleGateWSP(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateWSP& node, unsigned int nodeID) {
		return 0;
	}
	int DFT::DFTreeEXPBuilder::createSyncRuleGatePAnd(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GatePAnd& node, unsigned int nodeID) {
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
