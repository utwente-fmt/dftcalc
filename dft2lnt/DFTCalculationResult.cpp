#include "DFTCalculationResult.h"

std::string DFT::DFTCalculationResultItem::valStr(void) const {
	if (lowerBound == upperBound)
		return lowerBound.str();
	std::string lower = lowerBound.str(), upper = upperBound.str();
	std::string ret = "";
	std::string lexp = "", uexp = "";
	if (lower.find('e') != std::string::npos) {
		lexp = lower.substr(lower.find('e'));
		lower = lower.substr(0, upper.find('e'));
	}
	if (upper.find('e') != std::string::npos) {
		uexp = upper.substr(upper.find('e'));
		upper = upper.substr(0, upper.find('e'));
	}
	if (lexp != uexp) {
		return '[' + lower + lexp + "; " + upper + uexp + ']';
	}
	while (!lower.empty() && lower[0] == upper[0]) {
		ret += lower[0];
		lower = lower.substr(1);
		upper = upper.substr(1);
	}
	if (!lower.empty() || !upper.empty()) {
		ret += '[';
		ret += lower;
		ret += "; ";
		ret += upper;
		ret += ']';
	}
	ret += lexp;
	return ret;
}

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResult& result) {
	if(const YAML::Node itemNode = node["failProbs"]) {
		std::vector<DFT::DFTCalculationResultItem> failProbs;
		itemNode >> failProbs;
		result.failProbs = failProbs;
	}
	if(const YAML::Node itemNode = node["stats"]) {
		Shell::RunStatistics stats;
		itemNode >> stats;
		result.stats = stats;
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResult& result) {
	out << YAML::BeginMap;
	out << YAML::Key << "failProbs"  << YAML::Value << result.failProbs;
	// for dfttest, when we have one item, show it under the key that dfttest expects
	// dfttest currently only has support for the "failProb" key, not for "failProbs"
	for(auto it: result.failProbs) {
		out << YAML::Key << "failProb"  << YAML::Value << it.lowerBound.str();
		break;
	}
	out << YAML::Key << "stats"  << YAML::Value << result.stats;
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, map<std::string,DFT::DFTCalculationResult>& resultMap) {
	for(YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
		std::string dft;
		DFT::DFTCalculationResult result;
		dft = it->first.as<std::string>();
		it->second >> result;
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
	if(const YAML::Node itemNode = node["missionTime"]) {
		result.missionTime = itemNode.as<std::string>();
	}
	if(const YAML::Node itemNode = node["mrmcCommand"]) {
		result.mrmcCommand = itemNode.as<std::string>();
	}
	if(const YAML::Node itemNode = node["lowerBound"]) {
		result.lowerBound = decnumber<>(itemNode.as<std::string>());
	}
	if(const YAML::Node itemNode = node["upperBound"]) {
		result.upperBound = decnumber<>(itemNode.as<std::string>());
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResultItem& result) {
	out << YAML::BeginMap;
	out << YAML::Key << "missionTime"  << YAML::Value << result.missionTime;
	out << YAML::Key << "mrmcCommand"  << YAML::Value << result.mrmcCommand;
	out << YAML::Key << "lowerBound"  << YAML::Value << result.lowerBound.str();
	out << YAML::Key << "upperBound"  << YAML::Value << result.upperBound.str();
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, vector<DFT::DFTCalculationResultItem>& resultVector) {
	for(YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
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
