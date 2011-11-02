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

class EXPSyncItem {
protected:
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
	int getArg(unsigned int n) const { return args.at(n); }
	void setArg(unsigned int n, int v) {
		while(args.size()<n) {
			args.push_back(0);
		}
		args.at(n) = v;
	}
	const std::string& getName() const { return name; }
	
	virtual std::string toString() const {
		std::stringstream ss;
		ss << name;
		for(size_t n=0; n<args.size(); ++n) {
			ss << " !" << args.at(n);
		}
		return ss.str();
	}
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
	virtual std::string toString() const {
		assert( (args.size()==2) && "EXPSyncItemIB should have two arguments");
		std::stringstream ss;
		ss << name;
		ss << " !" << args.at(0);
		ss << " !" << (args.at(1)?"TRUE":"FALSE");
		return ss.str();
	}
};

class EXPSyncRule {
public:
	std::map<unsigned int,EXPSyncItem*> label;
	std::string toLabel;
	bool hideToLabel;
	const DFT::Nodes::Node* syncOnNode;
public:
	EXPSyncRule(const std::string& toLabel, bool hideToLabel=true):
		toLabel(toLabel),
		hideToLabel(hideToLabel) {
	}
	~EXPSyncRule() {
	}
};

class DFTreeEXPBuilder {
private:
	std::string root;
	std::string tmp;
	std::string nameBCG;
	std::string nameEXP;
	DFT::DFTree* dft;
	CompilerContext* cc;
	
	FileWriter svl_header;
	FileWriter svl_body;
	FileWriter exp_header;
	FileWriter exp_body;

	std::vector<DFT::Nodes::BasicEvent*> basicEvents;
	std::vector<DFT::Nodes::Gate*> gates;
	std::set<DFT::Nodes::NodeType> neededFiles;
	std::map<const DFT::Nodes::Node*, unsigned int> nodeIDs;
	
	int validateReferences();
	void printSyncLine(const EXPSyncRule& rule, const vector<unsigned int>& columnWidths);

public:

	/**
	 * Constructs a new DFTreeEXPBuilder using the specified DFT and
	 * CompilerContext.
	 * @param tmp The root dft2lnt folder
	 * @param tmp The folder in which temporary files will be put in.
	 * @param The name of the SVL script to generate.
	 * @param dft The DFT to be validated.
	 * @param cc The CompilerConstruct used for eg error reports.
	 */
	DFTreeEXPBuilder(std::string root, std::string tmp, std::string nameBCG, std::string nameEXP, DFT::DFTree* dft, CompilerContext* cc);
	virtual ~DFTreeEXPBuilder() {
	}

	/**
	 * Returns the Lotos NT File needed for the specified node.
	 * @return he Lotos NT File needed for the specified node.
	 */
	std::string getFileForNode(const DFT::Nodes::Node& node) const;

	std::string getBEProc(const DFT::Nodes::BasicEvent& be) const;
	
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
	int buildEXPHeader();

	/**
	 * Builds the actual composition script from the DFT specification
	 * Affects FileWriters: exp_body
	 * @return UNDECIDED
	 */
	int buildEXPBody();
	
	unsigned int getIDOfNode(const DFT::Nodes::Node& node) const;

	EXPSyncItem* syncActivate(unsigned int localNodeID, bool sending) {
		return new EXPSyncItemIB("A",localNodeID,sending);
	}

	EXPSyncItem* syncFail(unsigned int localNodeID) {
		return new EXPSyncItem("F",localNodeID);
	}

	int createSyncRuleBE(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::BasicEvent& node, unsigned int nodeID);
	int createSyncRuleGateOr(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateOr& node, unsigned int nodeID);
	int createSyncRuleGateAnd(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateAnd& node, unsigned int nodeID);
	int createSyncRuleGateWSP(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::GateWSP& node, unsigned int nodeID);
	int createSyncRuleTop(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules);
	int createSyncRule(vector<DFT::EXPSyncRule*>& activationRules, vector<DFT::EXPSyncRule*>& failRules, const DFT::Nodes::Gate& node, unsigned int nodeID);

	void calculateColumnWidths(vector<unsigned int>& columnWidths, const vector<DFT::EXPSyncRule*>& syncRules);

	void printEXP(std::ostream& out);
	void printSVL(std::ostream& out);
};

} // Namespace: DFT

#endif // DFTREEEXPBUILDER_H
