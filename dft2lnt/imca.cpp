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
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

int IMCA::FileHandler::readOutputFile(const File& file) {
	if(!FileSystem::exists(file)) {
		return 1;
	}

	FILE* fp;
	long len;
	char* buffer;

	fp = fopen(file.getFileRealPath().c_str(),"rb");
	fseek(fp,0,SEEK_END);
	len = ftell(fp)+1;
	fseek(fp,0,SEEK_SET);
	buffer = (char *)malloc(len);
	memset(buffer, 0, len);
	fread(buffer,len,1,fp); //read into buffer
	fclose(fp);

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
	int found = 0;
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
			if (r_et ==  1){
				results.push_back(std::pair<std::string,std::string>("?",res));
				i_isCalculated = true;
				found = 1;
				break;
			}
		}
	}
	if (found) {
		free(buffer);
		return 0;
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
				results.push_back(std::pair<std::string,std::string>("?",res));
				i_isCalculated = true;
				found = 1;
				break;
			}
		}
	}
	if (found) {
		free(buffer);
		return 0;
	}

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

		if (r_tb ==  1 && r_prob == 1){
			results.push_back(std::pair<std::string, std::string>(tb_res, prob_res));
			i_isCalculated = true;
		} else if (r_prob == 1){
			results.push_back(std::pair<std::string,std::string>("?",prob_res));
			i_isCalculated = true;
		}
	}

	free(buffer);
	return 0;
}

std::string IMCA::FileHandler::getResult() {
	if(results.size()<1) {
		return std::string();
	}
	return results[0].second;
}

std::vector<std::pair<std::string,std::string>> IMCA::FileHandler::getResults() {
	return results;
}
