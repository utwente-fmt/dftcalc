/*
 * DFTCalculationResult.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#ifndef DFTCALCULATIONRESULT_H
#define DFTCALCULATIONRESULT_H

#include "Shell.h"
#include "yaml-cpp/yaml.h"
#include "decnumber.h"
#include "query.h"
#include "CADP.h"

namespace DFT {
class DFTCalculationResultItem {
	public:
		std::string missionTime;
		std::string mrmcCommand;
		Query query;
		std::string exactString;
		decnumber<> lowerBound, upperBound;
		bool exactBounds;
		DFTCalculationResultItem() : lowerBound(0), upperBound(1) { }
		DFTCalculationResultItem(Query query) : lowerBound(0), upperBound(1) {
			mrmcCommand = query.toString();
			missionTime = query.upperBound.str();
		}
		std::string valStr(size_t deltaDigits = 3) const;
};

class DFTCalculationResult {
	public:
		Shell::RunStatistics stats;
		std::vector<DFTCalculationResultItem> failProbs;
};
} // Namespace: DFT

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResult& result);
YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResult& result);

const YAML::Node& operator>>(const YAML::Node& node, map<std::string,DFT::DFTCalculationResult>& resultMap);
YAML::Emitter& operator<<(YAML::Emitter& out, const map<std::string,DFT::DFTCalculationResult>& resultMap);

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResultItem& result);
YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResultItem& result);

const YAML::Node& operator>>(const YAML::Node& node, vector<DFT::DFTCalculationResultItem>& resultVector);
YAML::Emitter& operator<<(YAML::Emitter& out, const vector<DFT::DFTCalculationResultItem>& resultVector);

#endif
