#ifndef CADP_H
#define CADP_H

#include <fstream>

#include "Shell.h"
#include "yaml-cpp/yaml.h"

namespace DFT {

class CADP {
public:
	
	class BCGInfo {
	public:
		int states;
		int transitions;
		BCGInfo():
			states(0),
			transitions(0) {
		}
		
		bool willWriteSomething() const {
			return states>0
				|| transitions>0
				;
		}
		
	};
	
	static bool readStatsFromSVLLog(File logFile, Shell::RunStatistics& stats) {
		if(!FileSystem::exists(logFile)) {
			return true;
		}
		
		std::ifstream log(logFile.getFileRealPath());
		if(!log.is_open()) {
			return true;
		}
		
		char buffer[200];
		Shell::RunStatistics statsTemp;
		while(log.getline(buffer,200)) {
			if(!Shell::readMemtimeStatistics(buffer,statsTemp)) {
				stats.addTimeMaxMem(statsTemp);
			}
		}
		return false;
	}
	
	static bool BCG_Info(File bcgFile, BCGInfo& bcgInfo) {
		if(!FileSystem::exists(bcgFile)) {
			return true;
		}
		
		File statFile;
		do {
			char buffer[L_tmpnam];
			tmpnam(buffer);
			statFile = File(string(buffer));
		} while(FileSystem::exists(statFile));

		Shell::system("bcg_info " + bcgFile.getFileRealPath(),".",statFile.getFileRealPath(),"",5,NULL);
		
		std::ifstream log(statFile.getFileRealPath());
		if(!log.is_open()) {
			return true;
		}
		
		char buffer[300];
		while(log.getline(buffer,300)) {
			if(buffer[0]=='\t') {
				const char* c = buffer+1;
				while(*c && isdigit(*c)) c++;
				if(!strncmp(" states",c,7)) {
					sscanf(buffer,"	%i",&bcgInfo.states);
				} else if(!strncmp(" transitions",c,12)) {
					sscanf(buffer,"	%i",&bcgInfo.transitions);
				}
			}
		}
		
		return false;
	}
};

} // Namespace: DFT

const YAML::Node& operator>>(const YAML::Node& node, DFT::CADP::BCGInfo& bcgInfo);
YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::CADP::BCGInfo& bcgInfo);

#endif