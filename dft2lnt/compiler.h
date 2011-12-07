#ifndef COMPILER_H
#define COMPILER_H

// Maximum number of nested files using include statements
#define MAX_FILE_NESTING 100

#include <iostream>
#include <vector>
#include <set>
#include <stack>
#include <map>
#include "ConsoleWriter.h"
#include "dft_parser_location.h"

/**
 * Every file is associated with an instance of this struct
 */
typedef struct FileContext {
	FILE* fileHandle;
	std::string filename;
	Location loc;
	
	FileContext():
		fileHandle(NULL),
		filename("") {
	}
	FileContext(FILE* fileHandle, std::string& filename, Location& loc):
		fileHandle(fileHandle),
		filename(filename),
		loc(loc) {
	}
} FileContext;

/**
 * The compiler context contains settings and data related to the compilation
 * of a file and its dependencies.
 */
class CompilerContext {
public:
	class MessageType {
	public:
		enum MType {

			MESSAGE=1,
			NOTIFY,
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
		static const MessageType Action;
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
	unsigned int errors;
	unsigned int warnings;
	std::string name;
	bool m_useColoredMessages;
	std::set<MSG> messages;
	int verbosity;
	
	void print(const Location& l, const std::string& str, const MessageType& mType);
public:
	map<string,string> types;
	FileContext fileContext[MAX_FILE_NESTING]; // max file nesting of MAX_FILE_NESTING allowed
	int fileContexts;

	/**
	 * Creates a new compiler context with a specific name.
	 */
	CompilerContext(std::ostream& out): consoleWriter(out), errors(0), warnings(0), name(""), verbosity(VERBOSITY_DEFAULT) {
	}

	~CompilerContext() {
	}

	/**
	 * Push the specified FileContext on the stack.
	 * @param The FileContext to be pushed on the stack.
	 * @return false: success, true: error
	 */
	bool pushFileContext(FileContext context) {
		if(fileContexts>=MAX_FILE_NESTING) {
			reportError("Max file nesting (100) surpassed");
			return true;
		} else {
			fileContext[fileContexts] = context;
			++fileContexts;
		}
		return false;
	}
	
	/**
	 * Pop the top FileContext from the stack.
	 * @return false: success, true: error
	 */
	bool popFileContext() {
		--fileContexts;
		return false;
	}
	
	/**
	 * Returns the current number of file contexts.
	 * @return The current number of file contexts.
	 */
	int getFileContexts() const {
		return fileContexts;
	}
	
	/**
	 * Returns a reference to the FileContext at the specified location
	 * on the stack.
	 * @param i The position in the stack of the FileContext that will be
	 *          returned.
	 * @return The FileContext at the specified location.
	 */
	FileContext& getFileContext(int i) {
		assert(0<=i && i < MAX_FILE_NESTING && "getFileContext(i): invalid i");
		return fileContext[i];
	}
	
	/**
	 * Report the specified error string at the specified location.
	 * The output format is: <file>:<<line>:error:<error>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The error string to report.
	 */
	void reportErrorAt(Location loc, std::string str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified warning string at the specified location.
	 * The output format is: <file>:<line>:warning:<error>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The error string to report.
	 */
	void reportWarningAt(Location loc, std::string str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified error, without a location.
	 * The output format is: error:<error>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The error string to report.
	 */
	void reportError(std::string str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	/**
	 * Report the specified error, without a location.
	 * The output format is: error:<error>
	 * @param loc The location (file, line number, etc) where the error
	 *            originated from.
	 * @param str The error string to report.
	 */
	void reportWarning(std::string str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	void reportActionAt(Location loc, std::string, const int& verbosityLevel = VERBOSITY_DEFAULT);
	void reportAction(std::string, const int& verbosityLevel = VERBOSITY_DEFAULT);

	void reportFile(std::string fileName, std::string contents, const int& verbosityLevel = VERBOSITY_DEFAULT);

	void reportSuccess(std::string  str, const int& verbosityLevel = VERBOSITY_DEFAULT);

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
	
	void notify(std::string str, const int& verbosityLevel = VERBOSITY_DEFAULT);

	void message(std::string str, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	void message(std::string str, const MessageType& mType, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	void messageAt(Location loc, std::string str, const MessageType& mType, const int& verbosityLevel = VERBOSITY_DEFAULT);
	
	void flush();
	
	bool testWritable(std::string fileName);

	void useColoredMessages(bool useColoredMessages) {
		m_useColoredMessages = useColoredMessages;
	}
	const bool& usingColoredMessaged() const { return m_useColoredMessages; }

	void reportErrors(const int& verbosityLevel = VERBOSITY_DEFAULT);

	const int& getVerbosity() const {
		return verbosity;
	}
	
	void setVerbosity(const int& verbosity) {
		this->verbosity = verbosity;
	}
};

#endif // COMPILER_H
