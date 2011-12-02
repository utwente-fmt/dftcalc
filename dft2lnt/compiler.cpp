#include <stdio.h>

#include "compiler.h"

const CompilerContext::MessageType CompilerContext::MessageType::Message(MessageType::MESSAGE);
const CompilerContext::MessageType CompilerContext::MessageType::Notify (MessageType::NOTIFY);
const CompilerContext::MessageType CompilerContext::MessageType::Action (MessageType::ACTION);
const CompilerContext::MessageType CompilerContext::MessageType::Warning(MessageType::WARNING);
const CompilerContext::MessageType CompilerContext::MessageType::Error  (MessageType::ERR);
const CompilerContext::MessageType CompilerContext::MessageType::File   (MessageType::FILE);

void CompilerContext::print(const Location& loc, const std::string& str, const MessageType& mType) {

	if(mType.isError()) {
		consoleWriter << ConsoleWriter::Color::Error;
	} else if (mType.isWarning()) {
		consoleWriter << ConsoleWriter::Color::Warning;
	} else if (mType.isNotify()) {
		consoleWriter << ConsoleWriter::Color::Notify;
	} else if (mType.isAction()) {
		consoleWriter << ConsoleWriter::Color::Action;
	} else if (mType.isFile()) {
		consoleWriter << ConsoleWriter::Color::Proper;
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
	} else if (mType.isAction()) {
		consoleWriter << " > ";
		consoleWriter << ConsoleWriter::Color::Reset;
	} else if (mType.isMessage()) {
	} else if (mType.isFile()) {
	} else {
		consoleWriter << ":";
	}

	consoleWriter << str;
	consoleWriter << consoleWriter.applypostfix;

	consoleWriter << ConsoleWriter::Color::Reset;
}


void CompilerContext::reportErrorAt(Location loc, std::string str) {
	messageAt(loc,str,MessageType::Error);
	errors++;
}

void CompilerContext::reportWarningAt(Location loc, std::string str) {
	messageAt(loc,str,MessageType::Warning);
	warnings++;
}

void CompilerContext::reportActionAt(Location loc, std::string str) {
	messageAt(loc,str,MessageType::Action);
}

void CompilerContext::reportError(std::string str) {
	message(str,MessageType::Error);
	errors++;
}

void CompilerContext::reportWarning(std::string str) {
	message(str,MessageType::Warning);
	warnings++;
}

void CompilerContext::reportAction(std::string str) {
	message(str,MessageType::Action);
}

void CompilerContext::reportFile(std::string str) {
	message(str,MessageType::File);
}

void CompilerContext::notify(std::string str) {
	message(str,MessageType::Notify);
}

void CompilerContext::message(std::string str) {
	message(str,MessageType::Message);
}

void CompilerContext::message(std::string str, const MessageType& mType) {
	messageAt(Location(),str,mType);
}


void CompilerContext::messageAt(Location loc, std::string str, const MessageType& mType) {
	static int n=1;
//	print(loc,"ADDED: " + str, mType);
	messages.insert(CompilerContext::MSG(n++,loc,str,mType));
}

void CompilerContext::reportErrors() {
	consoleWriter << consoleWriter.applyprefix;
	consoleWriter << ConsoleWriter::Color::Notify << ":: ";
	consoleWriter << ConsoleWriter::Color::Reset  << "Finished. " << errors << " errors and " << warnings << " warnings.";
	consoleWriter << consoleWriter.applypostfix;
}

void CompilerContext::flush() {
	std::set<MSG>::iterator it = messages.begin();
	for(;it!=messages.end(); ++it) {
		print(it->loc,it->message,it->type);
	}
	messages.clear();
}

bool CompilerContext::testWritable(std::string fileName) {
	FILE* f = fopen(fileName.c_str(),"a");
	if(f) {
		fclose(f);
		return true;
	} else {
		reportError("unable to open outputfile '" + fileName + "'");
		return false;
	}
}
