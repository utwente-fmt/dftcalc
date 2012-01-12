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
			WARNING,
			ERR,
			SUCCESS,
			FILE,
			TITLE,

			MESSAGE_FIRST = MESSAGE,
			MESSAGE_LAST  = MESSAGE,
			NOTIFY_FIRST  = NOTIFY,
			NOTIFY_LAST   = NOTIFY,
			NOTIFYH_FIRST = NOTIFYH,
			NOTIFYH_LAST  = NOTIFYH,
			ACTION_FIRST  = ACTION,
			ACTION_LAST   = ACTION,
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
	public:
		MessageType(MType type): type(type) {}
		static const MessageType Message;
		static const MessageType Notify;
		static const MessageType NotifyH;
		static const MessageType Action;
		static const MessageType Warning;
		static const MessageType Error;
		static const MessageType Success;
		static const MessageType File;
		static const MessageType Title;
		bool isMessage() const { return MESSAGE_FIRST <= type && type <= MESSAGE_LAST; }
		bool isNotify()  const { return NOTIFY_FIRST  <= type && type <= NOTIFY_LAST; }
		bool isNotifyH() const { return NOTIFYH_FIRST <= type && type <= NOTIFYH_LAST; }
		bool isAction()  const { return ACTION_FIRST  <= type && type <= ACTION_LAST; }
		bool isWarning() const { return WARNING_FIRST <= type && type <= WARNING_LAST; }
		bool isError()   const { return ERROR_FIRST   <= type && type <= ERROR_LAST; }
		bool isSuccess() const { return SUCCESS_FIRST <= type && type <= SUCCESS_LAST; }
		bool isFile()    const { return FILE_FIRST    <= type && type <= FILE_LAST; }
		bool isTitle()   const { return TITLE_FIRST   <= type && type <= TITLE_LAST; }
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

	MessageFormatter(std::ostream& out): consoleWriter(out), m_useColoredMessages(false), errors(0), warnings(0), m_autoFlush(false), verbosity(VERBOSITY_DEFAULT) {
		
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

	virtual void reportActionAt(Location loc, const std::string&, const int& verbosityLevel = VERBOSITY_DEFAULT);
	virtual void reportAction(const std::string&, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	virtual void reportFile(const std::string& fileName, const std::string& contents, const int& verbosityLevel = VERBOSITY_DEFAULT);

	virtual void reportSuccess(const std::string&  str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	virtual void notify(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);
	virtual void notifyHighlighted(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	virtual void message(const std::string& str, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	virtual void message(const std::string& str, const MessageType& mType, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	virtual void messageAt(Location loc, const std::string& str, const MessageType& mType, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	virtual void flush();
	
	virtual void useColoredMessages(bool useColoredMessages) {
		m_useColoredMessages = useColoredMessages;
		consoleWriter.setIgnoreColors(!useColoredMessages);
	}
	virtual const bool& usingColoredMessaged() const { return m_useColoredMessages; }

	virtual void reportErrors(const int& verbosityLevel = VERBOSITY_DEFAULT);

	virtual const int& getVerbosity() const {
		return verbosity;
	}
	
	virtual void setVerbosity(const int& verbosity) {
		this->verbosity = verbosity;
	}
	
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