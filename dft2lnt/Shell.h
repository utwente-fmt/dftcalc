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
	
	/**
	 * Gets the contents of the specified environment variable.
	 * @param var The contents will be written to this string.
	 * @return whether or not the environment variable exists.
	 */
	static bool getenv(std::string& var) {
		const char* varResult = ::getenv(var.c_str());
		if(varResult) {
			var = string(varResult);
		}
		return !varResult;
	}
	
	/**
	 * Execute the specified command, in the specified working directory.
	 * Pipe stdout to the file specified by outFile and stderr to the file
	 * specified by errFile.
	 * Returns the return code of the command or some error code in case of an error.
	 * @param command The command to execute.
	 * @param cwd The directory in which the command will be executed.
	 * @param outFile The file to which stdout will be piped.
	 * @param errFile The file to which stderr will be piped.
	 * @return The return code of the command or some error code in case of an error.
	 */
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
		
		// Pipe stdout to /dev/null if no outFile is specified
		command += " > ";
		if(options.outFile.empty()) {
#ifdef WIN32
			command += "NUL";
#else
			command += "/dev/null";
#endif

		// If there is an outFile specified, pipe stdout to it
		} else {
			command += options.outFile;
		}
		command += " 2> ";

		// Pipe stderr to /dev/null if no errFile is specified
		if(options.errFile.empty()) {
#ifdef WIN32
			command += "NUL";
#else
			command += "/dev/null";
#endif

		// If there is an errFile specified, pipe stderr to it
		} else {
			command += options.errFile;
		}
		
		// Obtain the real path of the specified cwd and enter it
		string realCWD = FileSystem::getRealPath(options.cwd);
		PushD dir(realCWD);
		if(messageFormatter) messageFormatter->reportAction("Entering directory: `" + realCWD + "'", options.verbosity);
		
		// Execute the command
		int result = 0;
		if(messageFormatter) messageFormatter->reportAction("Executing: " + command, options.verbosity);
		result = ::system( command.c_str() );
		
		// Leave the cwd
		dir.popd();
		if(messageFormatter) messageFormatter->reportAction("Exiting directory: `" + realCWD + "'", options.verbosity);
		
		// Return the result of the command
		return result;
	}
};