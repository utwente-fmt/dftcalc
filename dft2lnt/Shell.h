#include <string>
#include "MessageFormatter.h"
#include "FileSystem.h"

class Shell {
public:
	
	class SystemOptions {
	public:
		std::string command;
		std::string cwd;
		std::string outFile;
		std::string errFile;
		std::string reportFile;
		int verbosity;
	};
	
	static MessageFormatter* messageFormatter;
	
	static bool getenv(std::string& var) {
		const char* varResult = ::getenv(var.c_str());
		if(varResult) {
			var = string(varResult);
		}
		return !varResult;
	}
	
	static int system(std::string command, std::string cwd=".", std::string outFile="", std::string errFile="") {
		SystemOptions sysOps;
		sysOps.command = command;
		sysOps.cwd = cwd;
		sysOps.outFile = outFile;
		sysOps.errFile = errFile;
		return system(sysOps);
	}
	static int system(const SystemOptions& options) {
		std::string command = options.command;
		
		command += " > ";
		if(options.outFile.empty()) {
#ifdef WIN32
			command += "NUL";
#else
			command += "/dev/null";
#endif
		} else {
			command += options.outFile;
		}
		command += " 2> ";
		if(options.errFile.empty()) {
#ifdef WIN32
			command += "NUL";
#else
			command += "/dev/null";
#endif
		} else {
			command += options.errFile;
		}
		string realCWD = FileSystem::getRealPath(options.cwd);
		PushD dir(realCWD);
		if(messageFormatter) messageFormatter->reportAction("Entering directory: `" + realCWD + "'", options.verbosity);
		int result = 0;
		if(messageFormatter) messageFormatter->reportAction("Executing: " + command, options.verbosity);
		result = ::system( command.c_str() );
		dir.popd();
		if(messageFormatter) messageFormatter->reportAction("Exiting directory: `" + realCWD + "'", options.verbosity);
		return result;
	}
};