#include <stdio.h>

#include "dft2lnt.h"

const DFT::CompilerContext::MessageType DFT::CompilerContext::MessageType::Message(MessageType::MESSAGE);
const DFT::CompilerContext::MessageType DFT::CompilerContext::MessageType::Notify (MessageType::NOTIFY);
const DFT::CompilerContext::MessageType DFT::CompilerContext::MessageType::Warning(MessageType::WARNING);
const DFT::CompilerContext::MessageType DFT::CompilerContext::MessageType::Error  (MessageType::ERR);

void DFT::CompilerContext::print(const Location& loc, const std::string& str, const MessageType& mType) {

	if(mType.isError()) {
		consoleWriter << ConsoleWriter::Color::Error;
	} else if (mType.isWarning()) {
		consoleWriter << ConsoleWriter::Color::Warning;
	} else if (mType.isNotify()) {
		consoleWriter << ConsoleWriter::Color::Notify;
	} else {
		consoleWriter << ConsoleWriter::Color::Message;
	}

//	consoleWriter << consoleWriter.applyprefix;
//	consoleWriter << loc.filename;

	loc.print(consoleWriter.ss());

	if(mType.isError()) {
		consoleWriter << ":error:";
	} else if (mType.isWarning()) {
		consoleWriter << ":warning:";
	} else if (mType.isNotify()) {
		consoleWriter << ":: ";
		consoleWriter << ConsoleWriter::Color::Reset;
	} else {
		consoleWriter << ":";
	}

	consoleWriter << str;
	consoleWriter << consoleWriter.applypostfix;

	consoleWriter << ConsoleWriter::Color::Reset;
}


void DFT::CompilerContext::reportErrorAt(Location loc, std::string str) {
	messageAt(loc,str,MessageType::Error);
	errors++;
}

void DFT::CompilerContext::reportWarningAt(Location loc, std::string str) {
	messageAt(loc,str,MessageType::Warning);
	warnings++;
}

void DFT::CompilerContext::reportError(std::string str) {
	message(str,MessageType::Error);
	errors++;
}

void DFT::CompilerContext::reportWarning(std::string str) {
	message(str,MessageType::Warning);
	warnings++;
}

void DFT::CompilerContext::notify(std::string str) {
	message(str,MessageType::Notify);
}

void DFT::CompilerContext::message(std::string str) {
	message(str,MessageType::Message);
}

void DFT::CompilerContext::message(std::string str, const MessageType& mType) {
	messageAt(Location(),str,mType);
}


void DFT::CompilerContext::messageAt(Location loc, std::string str, const MessageType& mType) {
	static int n=1;
//	print(loc,"ADDED: " + str, mType);
	messages.insert(DFT::CompilerContext::MSG(n++,loc,str,mType));
}

void DFT::CompilerContext::reportErrors() {
	consoleWriter << consoleWriter.applyprefix;
	consoleWriter << ConsoleWriter::Color::Notify << ":: ";
	consoleWriter << ConsoleWriter::Color::Reset  << "Finished. " << errors << " errors and " << warnings << " warnings.";
	consoleWriter << consoleWriter.applypostfix;
}

void DFT::CompilerContext::flush() {
	std::set<MSG>::iterator it = messages.begin();
	for(;it!=messages.end(); ++it) {
		print(it->loc,it->message,it->type);
	}
	messages.clear();
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
