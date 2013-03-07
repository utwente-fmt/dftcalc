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

IMCA::T_Chance IMCA::T_Chance_Default;

int IMCA::FileHandler::readOutputFile(const File& file) {
	if(!FileSystem::exists(file)) {
		return 1;
	}

	FILE* fp;
	long len, n;
	char* buffer;

	fp = fopen(file.getFileRealPath().c_str(),"rb");
	fseek(fp,0,SEEK_END);
	len = ftell(fp)+1;
	fseek(fp,0,SEEK_SET);
	buffer = (char *)malloc(len);
	memset(buffer, 0, len);
	n = fread(buffer,len,1,fp); //read into buffer
	fclose(fp);

	//fprintf(stdout, "imca buffer info (%ld,%ld):\n", len, n);
	//fprintf(stdout, "imca buffer\n%*s\n", (int)n, buffer);

	// for each line:
	char *p = buffer;
	char *k;
	char tb_needle[] = "tb=";
	char prob_needle[] = "probability: ";
	char et_needle[] = "Maximal expected time: ";
	if((k=strstr(p, et_needle)) != 0) {
		char *et = k + strlen(et_needle);
		char *et_e = strchr(et, '\n');
		if (et_e != 0) {
			*et_e = '\0';
			p = et_e + 1;
		} else {
			p = et;
		}
		double et_res = 0;
                int r_et  = sscanf(et,"%lf",&et_res);
		if (r_et ==  1){
			results.push_back(std::pair<std::string,IMCA::T_Chance>("?",et_res));
			i_isCalculated = true;
		}
	} else {
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
			double prob_res = 0;
			char *prob = k + strlen(prob_needle);
			char *prob_e = strchr(prob, '\n');
			if (prob_e != 0) {
				*prob_e = '\0';
				p = prob_e + 1;
			} else {
				p = prob;
			}
                	r_prob  = sscanf(prob,"%lf",&prob_res);

			// look for tb= value, from start of line
			// we will not cross into next line, because end-of-line is turned into \0
			int r_tb = 0;
			double tb_res = 0;
			char *tb;
			if ((k = strstr(line_start, tb_needle)) != 0) {
				//fprintf(stderr, "tb found, p=%p, k=%p, ep=%p, line_start=%p\n", p, k, ep, line_start);
				tb = k + strlen(tb_needle);
				char *tb_e = strchr(tb, ' ');
				if (tb_e != 0) {
					*tb_e = '\0';
					//p = tb_e + 1;
				} else {
					//p = tb;
				}
                		r_tb  = sscanf(tb,"%lf",&tb_res);
			}
			//fprintf(stderr, "r_prob=%d r_tb=%d\n", r_prob, r_tb);

			if (r_tb ==  1 && r_prob == 1){
				string tb_str(tb);
				results.push_back(std::pair<std::string,IMCA::T_Chance>(tb_str,prob_res));
				i_isCalculated = true;
			} else if (r_prob == 1){
				results.push_back(std::pair<std::string,IMCA::T_Chance>("?",prob_res));
				i_isCalculated = true;
			}
		}
	}

	free(buffer);
	return 0;
}

IMCA::T_Chance IMCA::FileHandler::getResult() {
	if(results.size()<1) {
		return T_Chance_Default;
	}
	return results[0].second;
}

std::vector<std::pair<std::string,IMCA::T_Chance>> IMCA::FileHandler::getResults() {
	return results;
}
