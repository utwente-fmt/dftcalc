/*
 * MessageFormatter.h
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#ifndef MESSAGEFORMATTER_H
#define MESSAGEFORMATTER_H

#include "dft_parser_location.h"
#include "ConsoleWriter.h"
#include <set>

class MessageFormatter {
public:
	class MessageType {
	public:
		enum MType {

			MESSAGE=1,
			NOTIFY,
			NOTIFYH,
			ACTION,
			ACTION2,
			ACTION3,
			WARNING,
			ERR,
			SUCCESS,
			FILE,
			TITLE,

			MESSAGE_FIRST = MESSAGE,
			MESSAGE_LAST  = MESSAGE,
			NOTIFY_FIRST  = NOTIFY,
			NOTIFY_LAST   = NOTIFYH,
			ACTION_FIRST  = ACTION,
			ACTION_LAST   = ACTION3,
			WARNING_FIRST = WARNING,
			WARNING_LAST  = WARNING,
			ERROR_FIRST   = ERR,
			ERROR_LAST    = ERR,
			SUCCESS_FIRST = SUCCESS,
			SUCCESS_LAST  = SUCCESS,
			FILE_FIRST    = FILE,
			FILE_LAST     = FILE,
			TITLE_FIRST   = TITLE,
			TITLE_LAST    = TITLE,

			NUMBEROF
		};
	private:
		MType type;
		MessageType(MType type): type(type) {}
	public:
		static const MessageType Message;
		static const MessageType Notify;
		static const MessageType NotifyH;
		static const MessageType Action;
		static const MessageType Action2;
		static const MessageType Action3;
		static const MessageType Warning;
		static const MessageType Error;
		static const MessageType Success;
		static const MessageType File;
		static const MessageType Title;
		bool isMessage() const { return MESSAGE_FIRST <= type && type <= MESSAGE_LAST; }
		bool isNotify()  const { return NOTIFY_FIRST  <= type && type <= NOTIFY_LAST; }
		bool isAction()  const { return ACTION_FIRST  <= type && type <= ACTION_LAST; }
		bool isWarning() const { return WARNING_FIRST <= type && type <= WARNING_LAST; }
		bool isError()   const { return ERROR_FIRST   <= type && type <= ERROR_LAST; }
		bool isSuccess() const { return SUCCESS_FIRST <= type && type <= SUCCESS_LAST; }
		bool isFile()    const { return FILE_FIRST    <= type && type <= FILE_LAST; }
		bool isTitle()   const { return TITLE_FIRST   <= type && type <= TITLE_LAST; }
		const MType& getType() const {return type;}
		bool operator==(const MessageType& other) const {
			return this->type == other.type;
		}
	};

	static const int VERBOSITY_DEFAULT;

private:

	class MSG {
	public:
		MSG(unsigned int id, Location loc, std::string message, MessageType type): id(id), loc(loc), message(message), type(type) {
		}
		unsigned int id;
		Location loc;
		std::string message;
		MessageType type;
		bool operator<(const MSG& other) const {
			if(loc.getFileName().length()>0 && loc.getFileName() == other.loc.getFileName()) {
				if(loc.getFirstLine()   < other.loc.getFirstLine()  ) return true;
				if(loc.getFirstLine()   > other.loc.getFirstLine()  ) return false;
				if(loc.getFirstColumn() < other.loc.getFirstColumn()) return true;
				if(loc.getFirstColumn() > other.loc.getFirstColumn()) return false;
				if(id                   < other.id                  ) return true;
				if(id                   > other.id                  ) return false;
				return false;
			} else {
				return id < other.id;
			}
		}
		bool operator==(const MSG& other) const {
			return id == other.id;
		}
	};

	ConsoleWriter consoleWriter;
	bool m_useColoredMessages;
	std::set<MSG> messages;
	unsigned int errors;
	unsigned int warnings;
	bool m_autoFlush;
	int verbosity;
	void print(const Location& l, const std::string& str, const MessageType& mType);
public:

	ConsoleWriter& getConsoleWriter() {
		return consoleWriter;
	}
	
	MessageFormatter(std::ostream& out): consoleWriter(out), m_useColoredMessages(false), errors(0), warnings(0), m_autoFlush(false), verbosity(VERBOSITY_DEFAULT) {
		
	}
	
	virtual ~MessageFormatter() {
	}
	
	/**
	 * Report the specified error string at the specified location.
	 * The output format is: <file>:<<line>:error:<error>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The error string to report.
	 */
	virtual void reportErrorAt(Location loc, const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified warning string at the specified location.
	 * The output format is: <file>:<line>:warning:<error>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The error string to report.
	 */
	virtual void reportWarningAt(Location loc, const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified error, without a location.
	 * The output format is: error:<error>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The error string to report.
	 */
	virtual void reportError(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified error, without a location.
	 * The output format is: error:<error>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The error string to report.
	 */
	virtual void reportWarning(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified action at the specified location.
	 * The output format is:  > <action>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The action string to report.
	 */
	virtual void reportActionAt(Location loc, const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified action, without a location.
	 * The output format is:  > <action>
	 * @param str The action string to report.
	 */
	virtual void reportAction(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified action at the specified location.
	 * The output format is:    > <action>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The action string to report.
	 */
	virtual void reportAction2At(Location loc, const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified action, without a location.
	 * The output format is:    > <action>
	 * @param str The action string to report.
	 */
	virtual void reportAction2(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	/**
	 * Report the specified action at the specified location.
	 * The output format is:    - <action>
	 * The action is in the default colour instead of white.
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The action string to report.
	 */
	virtual void reportAction3At(Location loc, const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified action, without a location.
	 * The output format is:    - <action>
	 * The action is in the default colour instead of white.
	 * @param str The action string to report.
	 */
	virtual void reportAction3(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	/**
	 * Report the specified filename and file contents, without a location.
	 * The output format is:  > <filename>:\n<contents>\n
	 * @param fileName The fileName to report.
	 * @param contents The contents to report.
	 */
	virtual void reportFile(const std::string& fileName, const std::string& contents, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified success string, without a location.
	 * The output format is:  o <success>
	 * @param str The success string to report.
	 */
	virtual void reportSuccess(const std::string&  str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified notification string, without a location.
	 * The output format is: :: <notification>
	 * @param str The notification string to report.
	 */
	virtual void notify(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified notification string, without a location.
	 * The output format is: :: <notification>
	 * @param str The notification string to report.
	 */
	virtual void notifyHighlighted(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	virtual void message(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	virtual void message(const std::string& str, const MessageType& mType, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	virtual void messageAt(Location loc, const std::string& str, const MessageType& mType, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	/**
	 * Flush all pending messages to the output.
	 */
	virtual void flush();
	
	/**
	 * Set whether or not to use coloured messages.
	 * @param useColoredMessages true/false: whether or not to use coloured messages.
	 */
	virtual void useColoredMessages(bool useColoredMessages) {
		m_useColoredMessages = useColoredMessages;
		consoleWriter.setIgnoreColors(!useColoredMessages);
	}
	
	/**
	 * Returns whether or not this MessageFormatter is using coloured messages.
	 * @return Whether or not this MessageFormatter is using coloured messages.
	 */
	virtual const bool& usingColoredMessages() const { return m_useColoredMessages; }

	/**
	 * Displays the number of errors and warnings gathered until now.
	 * Does not reset anything.
	 */
	virtual void reportErrors(const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Returns the set level of verbosity.
	 * @return The set level of verbosity.
	 */
	virtual const int& getVerbosity() const {
		return verbosity;
	}
	
	/**
	 * Sets the level of verbosity.
	 * Messages following this call will NOT be printed if their specified
	 * verbosity if higher than this level of verbosity.
	 * @param verbosity The minimum verbosity level messages should have to be printed.
	 */
	virtual void setVerbosity(const int& verbosity) {
		this->verbosity = verbosity;
	}
	
	/**
	 * Set whether or not to use auto flush.
	 * When autoflush is enabled: a flush is performed after each message.
	 * @param autoFlush: true/false: whether or not to use auto flush.
	 */
	virtual void setAutoFlush(bool autoFlush) {
		m_autoFlush = autoFlush;
	}
	
	/**
	 * Returns the number of reported errors.
	 * @return The number of reported errors.
	 */
	unsigned int getErrors() {
		return errors;
	}
	
	/**
	 * Returns the number of reported warnings.
	 * @return The number of reported warnings.
	 */
	unsigned int getWarnings() {
		return warnings;
	}
};

#endif
