/*
 * modest.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Enno Ruijters
 */

#include "modest.h"

#include "FileSystem.h"
#include "FileWriter.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>

std::vector<std::string> ModestRunner::getCommandOptions(Query q)
{
	std::vector<std::string> ret;
	ret.push_back("-E");
	if (q.type == TIMEBOUND) {
		ret.push_back("T=" + q.upperBound.str() + ",L=" + q.lowerBound.str());
	} else {
		ret.push_back("T=1,L=0");
	}
	if (q.errorBoundSet) {
		ret.push_back("--epsilon");
		ret.push_back(q.errorBound.str());
	}
	return ret;
}

static std::string getQuery(Query q)
{
	std::string ret;
	switch (q.type) {
	case EXPECTEDTIME:
		if (q.min)
			return "min_MTTF";
		return "max_MTTF";
	case STEADY:
		if (q.min)
			return "min_Unavailability";
		return "max_Unavailability";
	case TIMEBOUND:
		if (q.min)
			return "TBmin_Unreliability";
		if (q.lowerBound == 0)
			return "TBmax_Unreliability";
		return "TBLmax_Unreliability";
	case UNBOUNDED:
		if (q.min)
			return "UBmin_Unreliability";
		return "UBmax_Unreliability";
	default:
		throw std::logic_error("Unsupported query type for Modest: " + std::to_string(q.type));
	}
	return ret;
}

static int readOutputFile(File file, DFT::DFTCalculationResultItem &it) {
	if(!FileSystem::exists(file))
		return -1;

	std::string needle = "+ Property " + getQuery(it.query);
	std::ifstream input(file.getFileRealPath());
	std::string line;
	std::getline(input, line);
	while (input.good()) {
		if (line.find(needle) == 0) {
			std::getline(input, line);
			std::getline(input, line);
			if (!input.good())
				break;
			size_t start = line.find('[');
			if (start == std::string::npos)
				break;
			line.erase(0, start + 1);
			size_t end = line.find(',');
			std::string low = line.substr(0, end);
			std::string up = line.substr(end + 2);
			end = up.find(']');
			up = up.substr(0, end);
			it.lowerBound = decnumber<>(low);
			it.upperBound = decnumber<>(up);
			it.exactBounds = 0;
			return 0;
		}
		std::getline(input, line);
	}
	return -1;
}

std::vector<DFT::DFTCalculationResultItem> ModestRunner::analyze(std::vector<Query> queries)
{
	std::vector<DFT::DFTCalculationResultItem> ret;
	expandRangeQueries(queries);
	messageFormatter->reportAction("Calculating probability with Modest", DFT::VERBOSITY_FLOW);
	for (Query q : queries) {
		std::vector<std::string> arguments = getCommandOptions(q);
		arguments.push_back(janiFile.getFileRealPath());
		DFT::DFTCalculationResultItem it(q);
		int result;
		if (q.type != TIMEBOUND || q.upperBound != 0) {
			std::string of = exec->runCommand(modestCmd, arguments, "modest");
			if (of == "")
				return ret; /* Exec should have reported already */
			result = readOutputFile(of, it);
			if (result == -1) {
				messageFormatter->reportError("Could not calculate.");
				return ret;
			}
		} else {
			/* Incorrect for models with prob= BEs, but needed since
			 * Modest doesn't handle the query otherwise.
			 */
			it.exactBounds = 1;
			it.lowerBound = it.upperBound = (uintmax_t)0;
			result = 0;
		}
		if (result == 0 && !it.exactBounds) {
			decnumber<> margin = q.errorBound;
			if (q.errorBoundSet)
				margin *= 0.5;
			else /* MRMC default error bound 1e-6 */
				margin = decnumber<>("1e-6") * 0.5;
			it.upperBound = it.lowerBound + margin;
			it.lowerBound = it.lowerBound - margin;
			if (it.upperBound > decnumber<>(1)
				&& (q.type == TIMEBOUND
					|| q.type == STEADY
					|| q.type == UNBOUNDED))
			{
				it.upperBound = (intmax_t)1;
			}
			if (it.lowerBound < decnumber<>(0) && q.type != CUSTOM)
				it.lowerBound = (intmax_t)0;
		}
		ret.push_back(it);
	}
	messageFormatter->reportAction("Done calculating probability with Modest", DFT::VERBOSITY_FLOW);
	return ret;
}
