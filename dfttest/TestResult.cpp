#include "TestResult.h"

const YAML::Node& Test::TestResult::readYAMLNode(const YAML::Node& node) {
	readYAMLNodeSpecific(node);
	if(const YAML::Node* itemNode = node.FindValue("stats")) {
		*itemNode >> stats;
	}
	return node;
}

YAML::Emitter& Test::TestResult::writeYAMLNode(YAML::Emitter& out) const {
	out << YAML::BeginMap;
	out << YAML::Key << "stats" << YAML::Value << stats;
	writeYAMLNodeSpecific(out);
	out << YAML::EndMap;
	return out;
}

