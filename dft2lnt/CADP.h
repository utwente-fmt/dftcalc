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
	
	static std::string getRoot() {
		char* root = getenv((const char*)"CADP");
		std::string cadp = root?string(root):"";
		
		// \ to /
		{
			char buf[cadp.length()+1];
			for(int i=cadp.length();i--;) {
				if(cadp[i]=='\\')
					buf[i] = '/';
				else
					buf[i] = cadp[i];
			}
			buf[cadp.length()] = '\0';
			if(buf[cadp.length()-1]=='/') {
				buf[cadp.length()-1] = '\0';
			}
			cadp = string(buf);
		}
		return cadp;
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