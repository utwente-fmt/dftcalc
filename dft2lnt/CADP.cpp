#include "CADP.h"

const YAML::Node& operator>>(const YAML::Node& node, DFT::CADP::BCGInfo& bcgInfo) {
	if(const YAML::Node* itemNode = node.FindValue("states")) {
		*itemNode >> bcgInfo.states;
	}
	if(const YAML::Node* itemNode = node.FindValue("transitions")) {
		*itemNode >> bcgInfo.transitions;
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::CADP::BCGInfo& bcgInfo) {
	out << YAML::BeginMap;
	if(bcgInfo.states>0) out << YAML::Key << "states"  << YAML::Value << bcgInfo.states;
	if(bcgInfo.transitions>0) out << YAML::Key << "transitions"  << YAML::Value << bcgInfo.transitions;
	out << YAML::EndMap;
	return out;
}