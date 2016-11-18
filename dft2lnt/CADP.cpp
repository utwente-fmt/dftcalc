#include "CADP.h"

const YAML::Node& operator>>(const YAML::Node& node, DFT::CADP::BCGInfo& bcgInfo) {
	if(const YAML::Node itemNode = node["states"]) {
		bcgInfo.states = itemNode.as<int>();
	}
	if(const YAML::Node itemNode = node["transitions"]) {
		bcgInfo.transitions = itemNode.as<int>();
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
