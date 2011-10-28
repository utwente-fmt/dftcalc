#include <stdio.h>

#include "dft2lnt.h"

void DFT::CompilerContext::reportErrorAt(Location loc, std::string str) {
	out << loc.filename << ":" << loc.first_line << ":" << "error: " << str << endl;
	errors++;
}

void DFT::CompilerContext::reportWarningAt(Location loc, std::string str) {
	out << loc.filename << ":" << loc.first_line << ":warning: " << str << endl;
	warnings++;
}

void DFT::CompilerContext::reportError(std::string str) {
	out << "error: " << str << endl;
	errors++;
}

void DFT::CompilerContext::reportWarning(std::string str) {
	out << "warning: " << str << endl;
	warnings++;
}

void DFT::CompilerContext::message(std::string str) {
	out << ":: " << str << endl;
	warnings++;
}

bool DFT::CompilerContext::testWritable(std::string fileName) {
	FILE* f = fopen(fileName.c_str(),"a");
	if(f) {
		fclose(f);
		return true;
	} else {
		reportError("unable to open outputfile '" + fileName + "'");
		return false;
	}
}
