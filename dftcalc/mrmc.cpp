/*
 * mrmc.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include "mrmc.h"

#include "FileSystem.h"
#include "FileWriter.h"
#include "DFTCalculationResult.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <iterator>
#include <stdlib.h>
#include "query.h"

static std::string getQuery(Query q, std::string goalLabel) {
	std::string ret;
	std::string minMax = q.min ? "{>0}" : "{<1}";
	if (q.errorBoundSet)
		ret = "set error_bound " + q.errorBound.str() + "\n";
	else
		ret = "set error_bound 1e-6\n";

	switch (q.type) {
	case EXPECTEDTIME:
		ret += "M" + minMax + "[" + goalLabel + "]";
		break;
	case STEADY:
		ret += "S" + minMax + "[" + goalLabel + "]";
		break;
	case TIMEBOUND:
		ret = "P" + minMax + "[tt U[";
		if (q.step == -1)
			ret += q.lowerBound.str() + ", " + q.upperBound.str();
		else
			throw std::logic_error("Time-stepped queries should have been transformed to individual times before reaching the MRMC query converter.");
		ret += "] " + goalLabel + "]";
		break;
	case UNBOUNDED:
		ret = "P" + minMax + "[tt U " + goalLabel + "]";
		break;
	default:
		throw std::logic_error("Unsupported query type for IMCA.");
	}
	ret += "\n";
	return ret;
}

static std::pair<decnumber<>, decnumber<>> readOutputFile(const File& file) {
	std::pair<decnumber<>, decnumber<>> ret(-1, -1);

	FILE* fp;
	long len;
	char* buffer;

	fp = fopen(file.getFileRealPath().c_str(),"rb");
	if (!fp)
		return ret;
	fseek(fp,0,SEEK_END);
	len = ftell(fp);
	fseek(fp,0,SEEK_SET);
	buffer = (char *)malloc(len + 1);
	if (fread(buffer, len, 1, fp) != 1) {
		fclose(fp);
		free(buffer);
		throw std::ios_base::failure("Error reading IMRMC output file");
	}
	buffer[len] = 0;
	fclose(fp);

	const char *resultString, *c;
	resultString = strstr(buffer, "$MIN_RESULT");
	if (resultString)
		c = resultString + 15;
	if (!resultString) {
		resultString = strstr(buffer, "$MAX_RESULT");
		if (resultString)
			c = resultString + 15;
	}
	if (!resultString) {
		resultString = strstr(buffer, "$RESULT[1]");
		if (resultString)
			c = resultString + 13;
	}
	if (!resultString) {
		resultString = strstr(buffer, "$RESULT:");
		if (resultString)
			c = resultString + 11;
	}

	if(!resultString)
		return ret;

	const char *end = c;
	while (*end && *end != '\n')
		end++;
	std::string res(c, end - c);
	if (res.find('[') == std::string::npos) {
		end = c;
		while (*end && *end != '\n')
			end++;
		res = std::string(c, end - c);
		decnumber<> val(res);
		ret = std::pair<decnumber<>, decnumber<>>(val, val);
	} else {
		size_t pos = res.find('[');
		std::string pre = res.substr(0, pos);
		res = res.substr(pos + 1);
		pos = res.find(']');
		std::string post = res.substr(pos + 1);
		res = res.substr(0, pos);
		pos = post.find(',');
		if (pos != std::string::npos)
			post = post.substr(0, pos);
		pos = res.find(';');
		if (pos == std::string::npos)
			pos = res.find(',');
		std::string low = pre + res.substr(0, pos) + post;
		std::string up = pre + res.substr(pos + 2) + post;
		ret = std::pair<decnumber<>, decnumber<>>(low, up);
	}

	free(buffer);
	return ret;
}

void expandRangeQueries(std::vector<Query> &queries) {
	std::vector<Query> newQueries;
	auto it = queries.begin();
	while (it != queries.end()) {
		Query q = *it;
		if (q.type == TIMEBOUND && q.step != -1) {
			decnumber<> cur = q.lowerBound;
			while (cur <= q.upperBound) {
				Query nq = q;
				nq.step = (intmax_t)-1;
				nq.lowerBound = (intmax_t)0;
				nq.upperBound = cur;
				it = queries.insert(it, nq) + 1;
				cur += q.step;
			}
			it = queries.erase(it);
		} else {
			it++;
		}
	}
}

std::vector<DFT::DFTCalculationResultItem> MRMCRunner::analyze(
		std::vector<Query> queries)
{
	std::vector<DFT::DFTCalculationResultItem> ret;
	expandRangeQueries(queries);
	messageFormatter->reportAction("Calculating probability with " + mrmcExec.getFileName(), DFT::VERBOSITY_FLOW);
	for (Query q : queries) {
		File inputFile = exec->genInputFile("query");
		std::string qText = getQuery(q, goalLabel);
		std::ofstream out(inputFile.getFileRealPath());
		if(!out.is_open()) {
			messageFormatter->reportError("Could not open "
			                + inputFile.getFileRealPath());
			return ret;
		}
		out << "set print off\n";
		out << qText;
		out << "$RESULT[1]\n";
		out << "quit\n";
		out.close();
		std::vector<std::string> arguments;
		arguments.push_back(isCtmdp ? "ctmdpi" : "ctmc");
		arguments.push_back(modelFile.getFileRealPath());
		arguments.push_back(labFile.getFileRealPath());

		std::string res = exec->runCommand(
				mrmcExec.getFilePath(),
				arguments,
				mrmcExec.getFileName(),
				std::vector<File>(),
				&inputFile);
		if (res == "")
			return ret; /* Exec should have reported already */
		auto result = readOutputFile(res);
		if (result.first == -1) {
			messageFormatter->reportError("Could not calculate.");
			return ret;
		}
		DFT::DFTCalculationResultItem it(q);
		if (mrmcExec.getFileName() == "mrmc") {
			decnumber<> errorBound = q.errorBound;
			it.exactBounds = 0;
			if (q.errorBoundSet)
				errorBound *= 0.5;
			else /* MRMC default error bound 1e-6 */
				errorBound = decnumber<>("1e-6") * 0.5;
			decnumber<> margin = errorBound;
			it.lowerBound = result.first - margin;
			it.upperBound = result.first + margin;
		} else { /* IMRMC */
			it.exactBounds = 1;
			it.lowerBound = result.first;
			it.upperBound = result.second;
		}
		ret.push_back(it);
	}
	return ret;
}
