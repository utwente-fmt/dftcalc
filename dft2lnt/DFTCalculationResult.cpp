#include "DFTCalculationResult.h"

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResult& result) {
	if(const YAML::Node* itemNode = node.FindValue("dft")) {
		std::string dft;
		*itemNode >> dft;
		result.dftFile = dft;
	}
	if(const YAML::Node* itemNode = node.FindValue("failprobs")) {
		std::vector<DFT::DFTCalculationResultItem> failprobs;
		*itemNode >> failprobs;
		result.failprobs = failprobs;
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
	out << YAML::Key << "failprobs"  << YAML::Value << result.failprobs;
	// for dfttest, when we have one item, show it under the key that dfttest expects
	// dfttest currently only has support for the "failprob" key, not for "failprobs"
	for(auto it: result.failprobs) {
		out << YAML::Key << "failprob"  << YAML::Value << it.failprob;
		break;
	}
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

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResultItem& result) {
	if(const YAML::Node* itemNode = node.FindValue("missionTime")) {
		std::string missionTime;
		*itemNode >> missionTime;
		result.missionTime = missionTime;
	}
	if(const YAML::Node* itemNode = node.FindValue("mrmcCommand")) {
		std::string mrmcCommand;
		*itemNode >> mrmcCommand;
		result.mrmcCommand = mrmcCommand;
	}
	if(const YAML::Node* itemNode = node.FindValue("failprob")) {
		double failprob;
		*itemNode >> failprob;
		result.failprob = failprob;
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResultItem& result) {
	out << YAML::BeginMap;
	out << YAML::Key << "missionTime"  << YAML::Value << result.missionTime;
	out << YAML::Key << "mrmcCommand"  << YAML::Value << result.mrmcCommand;
	out << YAML::Key << "failprob"  << YAML::Value << result.failprob;
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, vector<DFT::DFTCalculationResultItem>& resultVector) {
	for(YAML::Iterator it = node.begin(); it!=node.end(); ++it) {
		DFT::DFTCalculationResultItem result;
		*it >> result;
		resultVector.push_back(result);
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const vector<DFT::DFTCalculationResultItem>& resultVector) {
	out << YAML::BeginSeq;
	for(auto it: resultVector) {
		out << it;
	}
	out << YAML::EndSeq;
	return out;
}
