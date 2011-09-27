#include <stdio.h>

#include "dft2lnt.h"

void DFT::CompilerContext::reportErrorAt(Location loc, std::string str) {
	printf("%s:%i:error: %s\n",loc.filename.c_str(),loc.first_line,str.c_str());
	errors++;
}

void DFT::CompilerContext::reportWarningAt(Location loc, std::string str) {
	printf("%s:%i:warning: %s\n",loc.filename.c_str(),loc.first_line,str.c_str());
	warnings++;
}

void DFT::CompilerContext::reportError(std::string str) {
	printf("error: %s\n",str.c_str());
	errors++;
}

void DFT::CompilerContext::reportWarning(std::string str) {
	printf("warning: %s\n",str.c_str());
	warnings++;
}
