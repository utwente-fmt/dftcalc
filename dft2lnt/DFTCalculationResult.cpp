#include "DFTCalculationResult.h"

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResult& result) {
	if(const YAML::Node* itemNode = node.FindValue("dft")) {
		std::string dft;
		*itemNode >> dft;
		result.dftFile = dft;
	}
	if(const YAML::Node* itemNode = node.FindValue("failprob")) {
		double failprob;
		*itemNode >> failprob;
		result.failprob = failprob;
	}
	if(const YAML::Node* itemNode = node.FindValue("stats")) {
		Shell::RunStatistics stats;
		*itemNode >> stats;
		result.stats = stats;
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResult& result) {
	out << YAML::BeginMap;
	out << YAML::Key << "dft"  << YAML::Value << result.dftFile;
	out << YAML::Key << "failprob"  << YAML::Value << result.failprob;
	out << YAML::Key << "stats"  << YAML::Value << result.stats;
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, map<std::string,DFT::DFTCalculationResult>& resultMap) {
	for(YAML::Iterator it = node.begin(); it!=node.end(); ++it) {
		std::string dft;
		DFT::DFTCalculationResult result;
		it.first() >> dft;
		it.second() >> result;
		resultMap.insert(std::pair<std::string,DFT::DFTCalculationResult>(dft,result));
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const map<std::string,DFT::DFTCalculationResult>& resultMap) {
	out << YAML::BeginMap;
	for(auto it: resultMap) {
		out << YAML::Key   << it.first;
		out << YAML::Value << it.second;
	}
	out << YAML::EndMap;
	return out;
}
