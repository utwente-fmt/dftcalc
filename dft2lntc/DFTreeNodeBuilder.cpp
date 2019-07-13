#include "DFTreeNodeBuilder.h"
#include "automata/automata.h"
#include <string>

std::string DFT::DFTreeNodeBuilder::getNodeName(const DFT::Nodes::Node& node) {
	std::stringstream ss;

	if(node.getType()==DFT::Nodes::GateVotingType)
		ss << "v";
	else if(node.getType()==DFT::Nodes::InspectionType)
		ss << "i";
	else if(node.getType()==DFT::Nodes::ReplacementType)
		ss << "r";
	else if (node.getType() == DFT::Nodes::RepairUnitNdType) {
		const DFT::Nodes::Gate& gate = *static_cast<const DFT::Nodes::Gate*>(&node);
		ss << "ru_nd_" << gate.getChildren().size();
		return ss.str();
	}

	ss << node.getTypeStr();
	if(node.isBasicEvent()) {
		const DFT::Nodes::BasicEvent& be = *static_cast<const DFT::Nodes::BasicEvent*>(&node);
		if(be.getMu().is_zero()) {
			ss << "_cold";
		}
		if(be.getMaintain()>0) {
			ss << "_maintain";
		}
		if(!be.hasRepairModule()) {
			if (be.hasInspectionModule())
				ss << "_im";
			else if (be.isRepairable())
				ss << "_repair";
		} else if(be.isRepairable()) {
			if (be.hasInspectionModule())
				ss << "_imrm";
			else
				ss << "_rm";
		}
		if(be.getLambda().is_zero()) {
			ss << "_dummy";
		}
		if(be.getFailed()) {
			ss << "_failed";
		}
		if(be.getPhases()>1){
			ss << "_erl" << be.getPhases();
		}
		if(be.getInterval()>0){
			ss << "_interval" << be.getInterval();
		}
		if (be.isAlwaysActive())
			ss << "_aa";
	} else if(node.isGate()) {
		const DFT::Nodes::Gate& gate = *static_cast<const DFT::Nodes::Gate*>(&node);
		ss << "_c" << gate.getChildren().size();
		if(node.getType()==DFT::Nodes::GateVotingType) {
			const DFT::Nodes::GateVoting& gateVoting = *static_cast<const DFT::Nodes::GateVoting*>(&node);
			ss << "_t" << gateVoting.getThreshold();
		} if(node.getType()==DFT::Nodes::GateFDEPType) {
			const DFT::Nodes::GateFDEP& gateFDEP = *static_cast<const DFT::Nodes::GateFDEP*>(&node);
			ss << "_d" << gateFDEP.getDependers().size();
		} if(node.getType()==DFT::Nodes::ReplacementType) {
			const DFT::Nodes::Replacement& replacement = *static_cast<const DFT::Nodes::Replacement*>(&node);
			ss << "_p" << replacement.getPhases();
		}
		if(node.isRepairable()) {
			ss << "_r";
		}
		if (node.isAlwaysActive())
			ss << "_aa";
	} else {
		assert(0 && "getNodeName(): Unknown node type");
	}

	return ss.str();
}
