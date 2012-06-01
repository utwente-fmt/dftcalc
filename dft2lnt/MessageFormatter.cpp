/*
 * MessageFormatter.cpp
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#include "MessageFormatter.h"

const MessageFormatter::MessageType MessageFormatter::MessageType::Message(MessageType::MESSAGE);
const MessageFormatter::MessageType MessageFormatter::MessageType::Notify (MessageType::NOTIFY);
const MessageFormatter::MessageType MessageFormatter::MessageType::NotifyH(MessageType::NOTIFYH);
const MessageFormatter::MessageType MessageFormatter::MessageType::Action (MessageType::ACTION);
const MessageFormatter::MessageType MessageFormatter::MessageType::Action2(MessageType::ACTION2);
const MessageFormatter::MessageType MessageFormatter::MessageType::Action3(MessageType::ACTION3);
const MessageFormatter::MessageType MessageFormatter::MessageType::Warning(MessageType::WARNING);
const MessageFormatter::MessageType MessageFormatter::MessageType::Success(MessageType::SUCCESS);
const MessageFormatter::MessageType MessageFormatter::MessageType::Error  (MessageType::ERR);
const MessageFormatter::MessageType MessageFormatter::MessageType::File   (MessageType::FILE);
const MessageFormatter::MessageType MessageFormatter::MessageType::Title  (MessageType::TITLE);

const int MessageFormatter::VERBOSITY_DEFAULT = 0;

void MessageFormatter::print(const Location& loc, const std::string& str, const MessageType& mType) {

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
		if(mType==MessageFormatter::MessageType::Notify)
			consoleWriter << ConsoleWriter::Color::Notify2;
		else
			consoleWriter << ConsoleWriter::Color::NotifyH;
	} else if (mType.isAction() || mType.isTitle()) {
		if(mType==MessageFormatter::MessageType::Action) {
			consoleWriter << " > ";
			consoleWriter << ConsoleWriter::Color::Notify2;
		} else if(mType==MessageFormatter::MessageType::Action2) {
			consoleWriter << "   > ";
			consoleWriter << ConsoleWriter::Color::Notify2;
		} else {
			consoleWriter << "   - ";
			consoleWriter << ConsoleWriter::Color::Reset;
		}
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


void MessageFormatter::reportErrorAt(Location loc, const std::string& str, const int& verbosityLevel) {
	messageAt(loc,str,MessageType::Error,verbosityLevel);
	errors++;
}

void MessageFormatter::reportWarningAt(Location loc, const std::string& str, const int& verbosityLevel) {
	messageAt(loc,str,MessageType::Warning,verbosityLevel);
	warnings++;
}

void MessageFormatter::reportActionAt(Location loc, const std::string& str, const int& verbosityLevel) {
	messageAt(loc,str,MessageType::Action,verbosityLevel);
}

void MessageFormatter::reportAction2At(Location loc, const std::string& str, const int& verbosityLevel) {
	messageAt(loc,str,MessageType::Action2,verbosityLevel);
}

void MessageFormatter::reportAction3At(Location loc, const std::string& str, const int& verbosityLevel) {
	messageAt(loc,str,MessageType::Action3,verbosityLevel);
}

void MessageFormatter::reportError(const std::string& str, const int& verbosityLevel) {
	message(str,MessageType::Error,verbosityLevel);
	errors++;
}

void MessageFormatter::reportWarning(const std::string& str, const int& verbosityLevel) {
	message(str,MessageType::Warning,verbosityLevel);
	warnings++;
}

void MessageFormatter::reportAction(const std::string& str, const int& verbosityLevel) {
	message(str,MessageType::Action,verbosityLevel);
}

void MessageFormatter::reportAction2(const std::string& str, const int& verbosityLevel) {
	message(str,MessageType::Action2,verbosityLevel);
}

void MessageFormatter::reportAction3(const std::string& str, const int& verbosityLevel) {
	message(str,MessageType::Action3,verbosityLevel);
}

void MessageFormatter::reportFile(const std::string& fileName, const std::string& contents, const int& verbosityLevel) {
	message(fileName,MessageType::Title,verbosityLevel);
	message(contents,MessageType::File,verbosityLevel);
}

void MessageFormatter::reportSuccess(const std::string& str, const int& verbosityLevel) {
	message(str,MessageType::Success,verbosityLevel);
}

void MessageFormatter::notify(const std::string& str, const int& verbosityLevel) {
	message(str,MessageType::Notify,verbosityLevel);
}

void MessageFormatter::notifyHighlighted(const std::string& str, const int& verbosityLevel) {
	message(str,MessageType::NotifyH,verbosityLevel);
}

void MessageFormatter::message(const std::string& str, const int& verbosityLevel) {
	message(str,MessageType::Message,verbosityLevel);
}

void MessageFormatter::message(const std::string& str, const MessageType& mType, const int& verbosityLevel) {
	messageAt(Location(),str,mType,verbosityLevel);
}


void MessageFormatter::messageAt(Location loc, const std::string& str, const MessageType& mType, const int& verbosityLevel) {
	static int n=1;

	if(verbosityLevel>verbosity && !mType.isError() && !mType.isWarning()) {
		return;
	}

	if(m_autoFlush) {
		flush();
		print(loc,str,mType);
	} else {
		messages.insert(MessageFormatter::MSG(n++,loc,str,mType));
	}
}

void MessageFormatter::reportErrors(const int& verbosityLevel) {
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

void MessageFormatter::flush() {
	std::set<MSG>::iterator it = messages.begin();
	for(;it!=messages.end(); ++it) {
		print(it->loc,it->message,it->type);
	}
	messages.clear();
}

