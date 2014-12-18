/*
 * DFTreeEXPBuilder.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg and extended by Dennis Guck
 */

namespace DFT {
class DFTreeEXPBuilder;
}

#ifndef DFTREEEXPBUILDER_H
#define DFTREEEXPBUILDER_H

#include <set>
#include <map>
#include <vector>
#include <iostream>

#include "DFTree.h"
#include "dft_parser.h"
#include "FileWriter.h"
#include "files.h"

namespace DFT {

/**
 * This class reflects how one Node is synchronized in a synchronization rule.
 * It consists of a gate name and a list of arguments.
 */
class EXPSyncItem {
public:
	std::string name;
	vector<int> args;
public:
	EXPSyncItem(std::string name, int arg1):
		name(name) {
		args.push_back(arg1);
	}
	EXPSyncItem(std::string name, int arg1, int arg2):
		name(name) {
		args.push_back(arg1);
		args.push_back(arg2);
	}

	/**
	 * Returns the arguments used to synchronize on.
	 * @return The arguments used to synchronize on.
	 */
	int getArg(unsigned int n) const { return args.at(n); }
	
	/**
	 * Sets the argument used to synchronize on at the specified index.
	 * If there are unspecified arguments before the specified index, they
	 * will be initialized to 0.
	 * @param n The index of the new argument used to syncrhonize on.
	 * @param v The new argument used to syncrhonize on.
	 */
	void setArg(unsigned int n, int v) {
		while(args.size()<n) {
			args.push_back(0);
		}
		args.at(n) = v;
	}
	
	/**
	 * Returns the name of the gate used to synchronize on.
	 * @return The name of the gate used to synchronize on.
	 */
	const std::string& getName() const { return name; }
	
	/**
	 * Returns the label to be synchronized on. This is a textual
	 * representation of this class. The format is:
	 *  <gate name> ( ' !' <arg> )+
	 * @return The label to synchronize on.
	 */
	virtual std::string toString() const {
		std::stringstream ss;
		ss << name;
		for(size_t n=0; n<args.size(); ++n) {
			ss << " !" << args.at(n);
		}
		return ss.str();
	}
	/**
	 * Returns the label to be synchronized on. This is a textual
	 * representation of this class. This is the same as toString(),
	 * with doublequotes added.
	 * @return The label to synchronize on.
	 */
	virtual std::string toStringQuoted() const {
		return "\"" + toString() + "\"";
	}
};

class EXPSyncItemIB: public EXPSyncItem {
public:
	EXPSyncItemIB(std::string name, int arg1):
		EXPSyncItem(name,arg1) {
	}
	EXPSyncItemIB(std::string name, int arg1, bool arg2):
		EXPSyncItem(name,arg1,arg2?1:0) {
	}
	
	/**
	 * Returns the label to be synchronized on. This is a textual
	 * representation of this class. The format is:
	 *  <gate name> ' !' arg0 ' !' arg1
	 * Node that arg0 is represented as Integer
	 * and that arg1 is represented as boolean (TRUE/FALSE)
	 * @return The label to synchronize on.
	 */
	virtual std::string toString() const {
		assert( (args.size()==2) && "EXPSyncItemIB should have two arguments");
		std::stringstream ss;
		ss << name;
		ss << " !" << args.at(0);
		ss << " !" << (args.at(1)?"TRUE":"FALSE");
		return ss.str();
	}
};

/**
 * This class reflects a single synchronization rule in the "rule table"
 * of a generated EXP file.
 * It is built out of various EXPSyncItem belonging to a Node, and a toLabel
 * string specifying what the label/transition is called after synchronization.
 * Also specified is whether the new label should be hidden after the
 * synchronization and what the synchronized Node is (this is to help
 * synchronize multiple parents of a node)
 */
class EXPSyncRule {
public:
	/// A mapping from NodeID to EXPSyncItem
	std::map<unsigned int,EXPSyncItem*> label;
	
	/// The renamed label name
	std::string toLabel;
	
	/// Whether the new label should be hidden after synchronization
	bool hideToLabel;
	
	/// The Node on which is synchronized
	const DFT::Nodes::Node* syncOnNode;
	
//	EXPSyncRule* deepCopy() {
//		EXPSyncRule* newRule = new EXPSyncRule(*this);
//		newRule->label.clear();
//		for(pair<unsigned int, EXPSyncItem*> thisItem: this->label) {
//			newRule->label.insert(thisItem->first,new
//		}
//		return newRule;
//	}
	
public:
	EXPSyncRule(const std::string& toLabel, bool hideToLabel=true):
		toLabel(toLabel),
		hideToLabel(hideToLabel),
		syncOnNode(NULL) {
	}
	~EXPSyncRule() {
	}
	
};

/**
 * This class handles the generation of EXP and SVL code out of a DFT.
 */
class DFTreeEXPBuilder {
private:
	std::string root;
	std::string bcgRoot;
	std::string tmp;
	std::string nameBCG;
	std::string nameEXP;
	std::string nameTop;
	DFT::DFTree* dft;
	CompilerContext* cc;
	
	FileWriter svl_header;
	FileWriter svl_body;
	FileWriter exp_header;
	FileWriter exp_body;

	std::vector<DFT::Nodes::BasicEvent*> basicEvents;
	std::vector<DFT::Nodes::Gate*> gates;
	std::map<const DFT::Nodes::Node*, unsigned int> nodeIDs;
	
	int validateReferences();
	void printSyncLine(const EXPSyncRule& rule, const vector<unsigned int>& columnWidths);
	void printSyncLineShort(std::ostream& stream, const EXPSyncRule& rule);

public:

	/**
	 * Constructs a new DFTreeEXPBuilder using the specified DFT and
	 * CompilerContext.
	 * @param root The root dft2lnt folder
	 * @param tmp The folder in which temporary files will be put in.
	 * @param The name of the SVL script to generate.
	 * @param dft The DFT to be validated.
	 * @param cc The CompilerConstruct used for eg error reports.
	 */
	DFTreeEXPBuilder(std::string root, std::string tmp, std::string nameBCG, std::string nameEXP, DFT::DFTree* dft, CompilerContext* cc);
	virtual ~DFTreeEXPBuilder() {
	}

	/**
	 * Returns an EXP formatted string reflecting the specified Basic Event.
	 * Takes into account the renaming of failure rates.
	 * @param be The Basic Event of which an EXP formatted strign is wanted.
	 */	 
	std::string getBEProc(const DFT::Nodes::BasicEvent& be) const;
	
	/**
	 * Returns an EXP formatted string reflecting the specified Repair Unit.
	 * Takes into account the renaming of failure rates.
	 * @param ru The Repair Unit of which an EXP formatted strign is wanted.
	 */
	std::string getRUProc(const DFT::Nodes::Gate& ru) const;
    
    /**
     * Returns an EXP formatted string reflecting the specified inspection.
     * Takes into account the renaming of inspection rates.
     * @param insp The inspection Unit of which an EXP formatted strign is wanted.
     */
    std::string getINSPProc(const DFT::Nodes::Inspection& insp) const;
    
    /**
     * Returns an EXP formatted string reflecting the specified replacement.
     * Takes into account the renaming of inspection rates.
     * @param rep The replacement Unit of which an EXP formatted strign is wanted.
     */
    std::string getREPProc(const DFT::Nodes::Replacement& rep) const;

	/**
	 * Start building EXP specification from the DFT specified
	 * in the constructor.
	 * @return UNDECIDED
	 */
	int build();

	/**
	 * Affects FileWriters: exp_header
	 * @return UNDECIDED
	 */
	int buildEXPHeader(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules);

	/**
	 * Builds the actual composition script from the DFT specification
	 * Affects FileWriters: exp_body
	 * @return UNDECIDED
	 */
	int buildEXPBody(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules);

	/**
	 * Builds the rule system for the to be generated EXP file. 
	 * Calls to buildEXPBody() and buildEXPHeader() should be valid after this.
	 * @return UNDECIDED
	 */
	int parseDFT(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules);
	
	/**
	 * Builds the rule system for the to be generated EXP file.
	 * Calls to buildEXPBody() and buildEXPHeader() should be valid after this.
	 * @return UNDECIDED
	 */
	int parseDFT(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules,
			vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules);
    
    /**
     * Builds the rule system for the to be generated EXP file.
     * Calls to buildEXPBody() and buildEXPHeader() should be valid after this.
     * @return UNDECIDED
     */
    int parseDFT(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules,
                 vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules, vector<DFT::EXPSyncRule*>& inspectionRules);

	/**
	 * Affects FileWriters: exp_header
	 * @return UNDECIDED
	 */
	int buildEXPHeader(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& onlineRules);
    
    /**
     * Affects FileWriters: exp_header
     * @return UNDECIDED
     */
    int buildEXPHeader(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& onlineRules, vector<DFT::EXPSyncRule*>& inspectionRules);

	/**
	 * Builds the actual composition script from the DFT specification
	 * Affects FileWriters: exp_body
	 * @return UNDECIDED
	 */
	int buildEXPBody(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules,
			vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules);
    
    /**
     * Builds the actual composition script from the DFT specification
     * Affects FileWriters: exp_body
     * @return UNDECIDED
     */
    int buildEXPBody(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules,
                     vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules, vector<DFT::EXPSyncRule*>& inspectionRules);

	/**
	 * Return the Node ID (index in the dft->getNodes() vector) of the
	 * specified Node.
	 * @param node The Node of which the Node ID is returned.
	 * @return The Node ID of the specified Node.
	 */
	unsigned int getIDOfNode(const DFT::Nodes::Node& node) const;

	int getLocalIDOfNode(const DFT::Nodes::Node* parent, const DFT::Nodes::Node* child) const;

	const DFT::Nodes::Node* getNodeWithID(unsigned int id);

	/**
	 * Create a new EXPSyncItem instance reflecting an Activate action
	 * based on the specified localNodeID and if this is the sendign action
	 * or not.
	 * @param localNodeID This is the ID of the Node seen from the actor.
	 * @param sending Whether this is a sending action or not.
	 * @return A new EXPSyncItem instance.
	 */
	EXPSyncItem* syncActivate(unsigned int localNodeID, bool sending) {
		return new EXPSyncItemIB(DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE,localNodeID,sending);
	}

	/**
	 * Create a new EXPSyncItem instance reflecting a Fail action
	 * based on the specified localNodeID.
	 * @param localNodeID This is the ID of the Node seen from the actor.
	 * @return A new EXPSyncItem instance.
	 */
	EXPSyncItem* syncFail(unsigned int localNodeID) {
		return new EXPSyncItem(DFT::DFTreeBCGNodeBuilder::GATE_FAIL,localNodeID);
	}

	/**
	 * Create a new EXPSyncItem instance reflecting a Repair action
	 * based on the specified localNodeID.
	 * @param localNodeID This is the ID of the Node seen from the actor.
	 * @return A new EXPSyncItem instance.
	 */
	EXPSyncItem* syncRepair(unsigned int localNodeID) {
		return new EXPSyncItem(DFT::DFTreeBCGNodeBuilder::GATE_REPAIR,localNodeID);
	}

	/**
	 * Create a new EXPSyncItem instance reflecting a Repairing action
	 * based on the specified localNodeID.
	 * @param localNodeID This is the ID of the Node seen from the actor.
	 * @return A new EXPSyncItem instance.
	 */
	EXPSyncItem* syncRepairing(unsigned int localNodeID) {
		return new EXPSyncItem(DFT::DFTreeBCGNodeBuilder::GATE_REPAIRING,localNodeID);
	}

	/**
	 * Create a new EXPSyncItem instance reflecting a Online action
	 * based on the specified localNodeID.
	 * @param localNodeID This is the ID of the Node seen from the actor.
	 * @return A new EXPSyncItem instance.
	 */
	EXPSyncItem* syncOnline(unsigned int localNodeID) {
		return new EXPSyncItem(DFT::DFTreeBCGNodeBuilder::GATE_ONLINE,localNodeID);
	}

	/**
	 * Create a new EXPSyncItem instance reflecting an Repaired action
	 * based on the specified localNodeID and if this is the sendign action
	 * or not.
	 * @param localNodeID This is the ID of the Node seen from the actor.
	 * @param sending Whether this is a sending action or not.
	 * @return A new EXPSyncItem instance.
	 */
	EXPSyncItem* syncRepaired(unsigned int localNodeID, bool sending) {
		return new EXPSyncItemIB(DFT::DFTreeBCGNodeBuilder::GATE_REPAIRED,localNodeID,sending);
	}
    
    /**
     * Create a new EXPSyncItem instance reflecting a Inspection action
     * based on the specified localNodeID.
     * @param localNodeID This is the ID of the Node seen from the actor.
     * @return A new EXPSyncItem instance.
     */
    EXPSyncItem* syncInspection(unsigned int localNodeID) {
        return new EXPSyncItem(DFT::DFTreeBCGNodeBuilder::GATE_INSPECT,localNodeID);
    }

	int createSyncRuleBE(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::BasicEvent& node, unsigned int nodeID);
	int createSyncRuleGateOr(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateOr& node, unsigned int nodeID);
	int createSyncRuleGateAnd(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateAnd& node, unsigned int nodeID);
	int createSyncRuleGateWSP(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateWSP& node, unsigned int nodeID);
	int createSyncRuleGatePAnd(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GatePAnd& node, unsigned int nodeID);
    int createSyncRuleGatePor(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GatePor& node, unsigned int nodeID);
	int createSyncRuleGateVoting(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateVoting& node, unsigned int nodeID);
	int createSyncRuleGateFDEP(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateFDEP& node, unsigned int nodeID);
	int createSyncRuleRepairUnit(vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, const DFT::Nodes::RepairUnit& node, unsigned int nodeID);
    int createSyncRuleInspection(vector<DFT::EXPSyncRule*>& inspectionRules, vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, const DFT::Nodes::Inspection& node, unsigned int nodeID);
    int createSyncRuleReplacement(vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, const DFT::Nodes::Replacement& node, unsigned int nodeID);

	/**
	 * Generate synchronization rules for the Top Node.
	 * This generates the Top Node Fail and Activate labels that will not
	 * be synchronized with during this step.
	 * @param activationRules Generated activation rules will go here.
	 * @param activationRules Generated fail rules will go here.
	 * @return 0.
	 */
	int createSyncRuleTop(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules);
	int createSyncRuleTop(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, vector<DFT::EXPSyncRule*>& onlineRules);
	int createSyncRule(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::Gate& node, unsigned int nodeID);
	int createSyncRule(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
			vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules, const DFT::Nodes::Gate& node, unsigned int nodeID);
    int createSyncRule(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules,
                       vector<DFT::EXPSyncRule*>& repairRules, vector<DFT::EXPSyncRule*>& repairedRules, vector<DFT::EXPSyncRule*>& repairingRules, vector<DFT::EXPSyncRule*>& onlineRules, vector<DFT::EXPSyncRule*>& inspectionRules, const DFT::Nodes::Gate& node, unsigned int nodeID);

	/**
	 * Modifies the specified columnWidths to reflect the width needed by the
	 * specified syncRules. This can be used for printing to a file in an
	 * aligned fashion, making it more readable.
	 * Elements of columnWidths are only updated if the
	 * existing value is smaller than the calculated value.
	 */
	void calculateColumnWidths(vector<unsigned int>& columnWidths, const vector<DFT::EXPSyncRule*>& syncRules);

	/**
	 * Print the generated EXP output to the specified stream.
	 * @param out The generated EXP output will be streamed to this stream.
	 */
	void printEXP(std::ostream& out);

	/**
	 * Print the generated SVL output to the specified stream.
	 * @param out The generated SVL output will be streamed to this stream.
	 */
	void printSVL(std::ostream& out);
	
	void setTopName(std::string name) {
		nameTop = name;
	}
	
	const std::string& getTopName() const {
		return nameTop;
	}
};

} // Namespace: DFT

std::ostream& operator<<(std::ostream& stream, const DFT::EXPSyncItem& item);

#endif // DFTREEEXPBUILDER_H
