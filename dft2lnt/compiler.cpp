#include <stdio.h>

#include "compiler.h"

const CompilerContext::MessageType CompilerContext::MessageType::Message(MessageType::MESSAGE);
const CompilerContext::MessageType CompilerContext::MessageType::Notify (MessageType::NOTIFY);
const CompilerContext::MessageType CompilerContext::MessageType::Action (MessageType::ACTION);
const CompilerContext::MessageType CompilerContext::MessageType::Warning(MessageType::WARNING);
const CompilerContext::MessageType CompilerContext::MessageType::Success(MessageType::SUCCESS);
const CompilerContext::MessageType CompilerContext::MessageType::Error  (MessageType::ERR);
const CompilerContext::MessageType CompilerContext::MessageType::File   (MessageType::FILE);
const CompilerContext::MessageType CompilerContext::MessageType::Title  (MessageType::TITLE);

const int CompilerContext::VERBOSITY_DEFAULT = 0;

void CompilerContext::print(const Location& loc, const std::string& str, const MessageType& mType) {

	if(mType.isError()) {
		consoleWriter << ConsoleWriter::Color::Error;
	} else if (mType.isWarning()) {
		consoleWriter << ConsoleWriter::Color::Warning;
	} else if (mType.isNotify()) {
		consoleWriter << ConsoleWriter::Color::Notify;
	} else if (mType.isAction() || mType.isTitle()) {
		consoleWriter << ConsoleWriter::Color::Action;
	} else if (mType.isSuccess()) {
		consoleWriter << ConsoleWriter::Color::Proper;
	} else if (mType.isFile()) {
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
		consoleWriter << ConsoleWriter::Color::Notify2;
	} else if (mType.isAction() || mType.isTitle()) {
		consoleWriter << " > ";
		consoleWriter << ConsoleWriter::Color::Notify2;
	} else if (mType.isMessage()) {
	} else if (mType.isSuccess()) {
		consoleWriter << " o ";
		consoleWriter << ConsoleWriter::Color::Reset;
	} else if (mType.isFile()) {
	} else {
		consoleWriter << ":";
	}

	consoleWriter << str;

	consoleWriter << ConsoleWriter::Color::Reset;

	if (mType.isTitle()) {
		consoleWriter << ":";
	}
	
	consoleWriter << consoleWriter.applypostfix;

}


void CompilerContext::reportErrorAt(Location loc, std::string str, const int& verbosityLevel) {
	messageAt(loc,str,MessageType::Error,verbosityLevel);
	errors++;
}

void CompilerContext::reportWarningAt(Location loc, std::string str, const int& verbosityLevel) {
	messageAt(loc,str,MessageType::Warning,verbosityLevel);
	warnings++;
}

void CompilerContext::reportActionAt(Location loc, std::string str, const int& verbosityLevel) {
	messageAt(loc,str,MessageType::Action,verbosityLevel);
}

void CompilerContext::reportError(std::string str, const int& verbosityLevel) {
	message(str,MessageType::Error,verbosityLevel);
	errors++;
}

void CompilerContext::reportWarning(std::string str, const int& verbosityLevel) {
	message(str,MessageType::Warning,verbosityLevel);
	warnings++;
}

void CompilerContext::reportAction(std::string str, const int& verbosityLevel) {
	message(str,MessageType::Action,verbosityLevel);
}

void CompilerContext::reportFile(std::string fileName, std::string contents, const int& verbosityLevel) {
	message(fileName,MessageType::Title,verbosityLevel);
	message(contents,MessageType::File,verbosityLevel);
}

void CompilerContext::reportSuccess(std::string  str, const int& verbosityLevel) {
	message(str,MessageType::Success,verbosityLevel);
}

void CompilerContext::notify(std::string str, const int& verbosityLevel) {
	message(str,MessageType::Notify,verbosityLevel);
}

void CompilerContext::message(std::string str, const int& verbosityLevel) {
	message(str,MessageType::Message,verbosityLevel);
}

void CompilerContext::message(std::string str, const MessageType& mType, const int& verbosityLevel) {
	messageAt(Location(),str,mType,verbosityLevel);
}


void CompilerContext::messageAt(Location loc, std::string str, const MessageType& mType, const int& verbosityLevel) {
	static int n=1;

	if(verbosityLevel>verbosity) {
		return;
	}

//	print(loc,"ADDED: " + str, mType);
		messages.insert(CompilerContext::MSG(n++,loc,str,mType));
}

void CompilerContext::reportErrors(const int& verbosityLevel) {
	if(verbosityLevel>verbosity) {
		return;
	}

	consoleWriter << consoleWriter.applyprefix;
	consoleWriter << ConsoleWriter::Color::Notify << ":: ";
	consoleWriter << ConsoleWriter::Color::Notify2  << "Finished. ";
	{
		if(errors==0) {
			consoleWriter << ConsoleWriter::Color::Proper;
		} else {
			consoleWriter << ConsoleWriter::Color::Error;
		}
		consoleWriter << errors << " errors";
	}
	consoleWriter << ConsoleWriter::Color::Notify2  << " and ";
	{
		if(warnings==0) {
			consoleWriter << ConsoleWriter::Color::Proper;
		} else {
			consoleWriter << ConsoleWriter::Color::Warning;
		}
		consoleWriter << warnings << " warnings";
	}
	consoleWriter << ConsoleWriter::Color::Notify2 << "." << consoleWriter.applypostfix;
	consoleWriter << ConsoleWriter::Color::Reset;
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
