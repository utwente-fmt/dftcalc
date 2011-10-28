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
