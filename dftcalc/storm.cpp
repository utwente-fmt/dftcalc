/*
 * storm.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Enno Ruijters
 */

#include "storm.h"

#include "FileSystem.h"
#include "FileWriter.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>

static const char needle[] = "Result (for initial states): ";

std::string StormRunner::getCommandOptions(Query q)
{
	std::string ret = " --constants T=1,L=0 ";
	if (q.errorBoundSet)
		ret = " --precision " + q.errorBound.str();
	if (runExact) {
		if (q.type == TIMEBOUND) {
			messageFormatter->reportWarning("Storm does not compute exact time-bounded properties, defaulting to approximate");
		} else {
			ret += " --exact ";
		}
	}
	if (q.type == STEADY && !runExact)
		ret += " --to-nondet ";
	return ret;
}

static std::string getQuery(Query q)
{
	std::string ret;
	switch (q.type) {
	case EXPECTEDTIME:
		ret = "T";
		ret += std::string(q.min ? "min" : "max");
		ret += "=? [F marked = 1]";
		break;
	case STEADY:
		ret = "LRA";
		ret += std::string(q.min ? "min" : "max");
		ret += "=? [marked = 1]";
		break;
	case TIMEBOUND:
		ret = "P";
		ret += std::string(q.min ? "min" : "max");
		ret += "=? [F";
		if (q.step == -1 && q.lowerBound == 0)
			ret += "<=" + q.upperBound.str();
		else if (q.step == -1)
			ret += "[" + q.lowerBound.str() + ", " + q.upperBound.str() + "]";
		else
			throw std::logic_error("Time-stepped queries should have been transformed to individual times before reaching the Storm query converter.");
		ret += " (marked = 1)]";
		break;
	case UNBOUNDED:
		ret = "P";
		ret += std::string(q.min ? "min" : "max");
		ret += "=? [F marked=1]";
		break;
	default:
		throw std::logic_error("Unsupported query type for Storm.");
	}
	return ret;
}

static int readOutputFile(File file, DFT::DFTCalculationResultItem &it) {
	if(!FileSystem::exists(file))
		return -1;

	std::ifstream input(file.getFileRealPath());
	std::string line;
	std::getline(input, line);
	while (input.good()) {
		if (line.find(needle) != std::string::npos) {
			line.erase(0, strlen(needle));
			size_t appr = line.find(" (approx.");
			if (appr != std::string::npos) {
				it.exactBounds = 1;
				line.erase(appr);
			} else {
				it.exactBounds = 0;
			}
			if (line.find('/') != std::string::npos) {
				it.exactString = line;
				return 1;
			}
			it.lowerBound = decnumber<>(line);
			return 0;
		}
		std::getline(input, line);
	}
	return -1;
}

std::vector<DFT::DFTCalculationResultItem> StormRunner::analyze(std::vector<Query> queries)
{
	std::vector<DFT::DFTCalculationResultItem> ret;
	expandRangeQueries(queries);
	messageFormatter->reportAction("Calculating probability with " + stormExec.getFileName(), DFT::VERBOSITY_FLOW);
	for (Query q : queries) {
		std::string qText = getQuery(q);
		std::string cmd = stormExec.getFilePath()
		                  + getCommandOptions(q)
		                  + " --prop \"" + qText + "\""
		                  + " --jani \"" + janiFile.getFileRealPath()+ "\"";
		DFT::DFTCalculationResultItem it(q);
		int result;
		std::string of;
		if (q.type == TIMEBOUND && q.upperBound == 0) {
			/* Special case since Storm fails to compute otherwise. */
			result = 0;
		} else {
			of = exec->runCommand(cmd, stormExec.getFileName());
			if (of == "") {
				messageFormatter->reportError("Could not calculate.");
				return ret;
			}
			result = readOutputFile(of, it);
		}
		if (result == -1) {
			messageFormatter->reportError("Could not calculate.");
			return ret;
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
	return ret;
}
