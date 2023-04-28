/*
 * imca.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Dennis Guck, extended by Axel Belinfante
 */

#include "imca.h"

#include "FileSystem.h"
#include "FileWriter.h"
#include "MessageFormatter.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

using std::vector;

static void getOptions(Query q, std::vector<std::string> &options) {
	if (q.min)
		options.push_back("-min");
	else
		options.push_back("-max");
	if (q.errorBoundSet) {
		options.push_back("-e");
		options.push_back(q.errorBound.str());
	}
	switch (q.type) {
	case EXPECTEDTIME:
		options.push_back("-et");
		break;
	case TIMEBOUND:
		options.push_back("-tb");
		options.push_back("-T");
		options.push_back(q.upperBound.str());
		if (q.lowerBound != 0) {
			options.push_back("-F");
			options.push_back(q.lowerBound.str());
		}
		if (q.step != -1) {
			options.push_back("-i");
			options.push_back(q.step.str());
		}
		break;
	case UNBOUNDED:
		options.push_back("-ub");
		break;
	default:
		throw std::logic_error("Unsupported query type for IMCA.");
	}
}

bool parseOutputFile(File file, Query q,
                     vector<DFT::DFTCalculationResultItem> &ret)
{
	/* This function assumes that the answer to only the given query is
	 * in the result file. It is entirely possible in IMCA to have
	 * multiple queries in one file, and if we ever need to include that
	 * optimization, we need to change this parser.
	 */
	FILE* fp;
	long len;
	char* buffer;

	fp = fopen(file.getFileRealPath().c_str(),"rb");
	if (!fp)
		return 0;
	fseek(fp,0,SEEK_END);
	len = ftell(fp)+1;
	fseek(fp,0,SEEK_SET);
	buffer = (char *)malloc(len);
	if (buffer)
		len = fread(buffer,len,1,fp); //read into buffer

	fclose(fp);

	if (len != 1)
		return 0;

	decnumber<> margin = q.errorBound;
	if (q.errorBoundSet)
		margin *= 0.5;
	else
		margin = decnumber<>("1e-6") * 0.5;

	//fprintf(stdout, "imca buffer info (%ld,%ld):\n", len, n);
	//fprintf(stdout, "imca buffer\n%*s\n", (int)n, buffer);

	// for each line:
	char *p = buffer;
	char *k;
	char tb_needle[] = "tb=";
	char prob_needle[] = "probability: ";
	char et_max_needle[] = "Maximal expected time: ";
	char et_min_needle[] = "Minimal expected time: ";
	char *et_needles[] = { et_max_needle, et_min_needle, 0 };
	char ub_max_needle[] = "Maximal unbounded reachability: ";
	char ub_min_needle[] = "Minimal unbounded reachability: ";
	char *ub_needles[] = { ub_max_needle, ub_min_needle, 0 };
	for(int i=0; et_needles[i] != 0; i++) {
		if((k=strstr(p, et_needles[i])) != 0) {
			char *et = k + strlen(et_needles[i]);
			char *et_e = strchr(et, '\n');
			if (et_e != 0) {
				*et_e = '\0';
				p = et_e + 1;
			} else {
				p = et;
			}
			std::string res(et, et_e - et);
			double et_res;
                	int r_et  = sscanf(et,"%lf",&et_res);
			if (r_et ==  1) {
				DFT::DFTCalculationResultItem it(q);
				decnumber<> dres(res);
				it.exactBounds = 0;
				it.lowerBound = dres - margin;
				it.upperBound = dres + margin;
				ret.push_back(it);
				free(buffer);
				return 1;
			}
		}
	}

	for(int i=0; ub_needles[i] != 0; i++) {
		if((k=strstr(p, ub_needles[i])) != 0) {
			char *ub = k + strlen(ub_needles[i]);
			char *ub_e = strchr(ub, '\n');
			if (ub_e != 0) {
				*ub_e = '\0';
				p = ub_e + 1;
			} else {
				p = ub;
			}
			std::string res(ub, ub_e - ub);
			double ub_res;
			int r_ub  = sscanf(ub,"%lf",&ub_res);
			if (r_ub ==  1){
				DFT::DFTCalculationResultItem it(q);
				decnumber<> dres(res);
				it.exactBounds = 0;
				it.lowerBound = dres - margin;
				it.upperBound = dres + margin;
				ret.push_back(it);
				free(buffer);
				return 1;
			}
		}
	}

	bool found = 0;
	p = buffer;
	while((k= strstr(p, prob_needle)) != 0) {
		// find start of line, by looking for end of previous line (either '\n' or '\0')
		char *ep = k;
		while(ep > p && *ep != '\0' && *ep != '\n')
			ep--;
		char *line_start;
		if (*ep == '\0' || *ep == '\n')
			line_start = ep + 1;
		else
			line_start = p;
		//fprintf(stderr, "prob found, p=%p, k=%p, ep=%p, line_start=%p\n", p, k, ep, line_start);

		// get probability value, turning end-of-line into \0
		int r_prob = 0;
		double tmp;
		char *prob = k + strlen(prob_needle);
		char *prob_e = strchr(prob, '\n');
		if (prob_e != 0) {
			*prob_e = '\0';
			p = prob_e + 1;
		} else {
			p = prob;
		}
		char *prob_v = strchr(prob, ' ');
		if (prob_v)
			prob_e = prob_v;
		std::string prob_res(prob, prob_e - prob);
		r_prob  = sscanf(prob,"%lf",&tmp);

		// look for tb= value, from start of line
		// we will not cross into next line, because end-of-line is turned into \0
		int r_tb = 0;
		std::string tb_res;
		if ((k = strstr(line_start, tb_needle)) != 0) {
			//fprintf(stderr, "tb found, p=%p, k=%p, ep=%p, line_start=%p\n", p, k, ep, line_start);
			char *tb = k + strlen(tb_needle);
			char *tb_e = strchr(tb, ' ');
			if (tb_e != 0) {
				*tb_e = '\0';
				//p = tb_e + 1;
			} else {
				//p = tb;
			}
			tb_res = std::string(tb, tb_e - tb);
			r_tb  = sscanf(tb,"%lf",&tmp);
		}
		//fprintf(stderr, "r_prob=%d r_tb=%d\n", r_prob, r_tb);

		decnumber<> dres(prob_res);
		if (r_tb ==  1 && r_prob == 1){
			Query qt = q;
			qt.lowerBound = qt.upperBound = decnumber<>(tb_res);
			DFT::DFTCalculationResultItem it(qt);
			it.exactBounds = 0;
			it.lowerBound = dres - margin;
			it.upperBound = dres + margin;
			ret.push_back(it);
		} else if (r_prob == 1){
			DFT::DFTCalculationResultItem it(q);
			it.exactBounds = 0;
			it.lowerBound = dres - margin;
			it.upperBound = dres + margin;
			ret.push_back(it);
		}
		found = 1;
	}

	free(buffer);
	return found;
}

std::vector<DFT::DFTCalculationResultItem> IMCARunner::analyze(vector<Query> queries)
{
	std::vector<DFT::DFTCalculationResultItem> ret;
	messageFormatter->reportAction("Calculating probability with IMCA...",DFT::VERBOSITY_FLOW);
	for(Query query: queries) {
		// imca -> calculation
		std::vector<std::string> arguments;
		arguments.push_back(modelFile.getFileRealPath());
		getOptions(query, arguments);

		std::string out = exec->runCommand(imcaExec.getFilePath(),
				arguments, "imca");
		if (out == "")
			return ret;

		File outFile(out);
		if (!parseOutputFile(outFile, query, ret)) {
			messageFormatter->reportError("Could not calculate");
			return ret;
		}
	}
	return ret;
}
