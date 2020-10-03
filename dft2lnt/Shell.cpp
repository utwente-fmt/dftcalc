/*
 * Shell.cpp
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#include "Shell.h"
#ifdef WIN32
# define WIN32_LEAN_AND_MEAN // Omit rarely-used and architecture-specific stuff from WIN32
# include <windows.h>
# include <iostream>
#endif

MessageFormatter* Shell::messageFormatter = NULL;

const YAML::Node& operator>>(const YAML::Node& node, Shell::RunStatistics& stats) {
	if(const YAML::Node itemNode = node["time_monraw"]) {
		stats.time_monraw = itemNode.as<float>();
	}
	if(const YAML::Node itemNode = node["time_user"]) {
		stats.time_user = itemNode.as<float>();
	}
	if(const YAML::Node itemNode = node["time_system"]) {
		stats.time_system = itemNode.as<float>();
	}
	if(const YAML::Node itemNode = node["time_elapsed"]) {
		stats.time_elapsed = itemNode.as<float>();
	}
	if(const YAML::Node itemNode = node["mem_virtual"]) {
		stats.mem_virtual = itemNode.as<float>();
	}
	if(const YAML::Node itemNode = node["mem_resident"]) {
		stats.mem_resident = itemNode.as<float>();
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const Shell::RunStatistics& stats) {
	out << YAML::BeginMap;
	if(stats.time_monraw>0)  out << YAML::Key << "time_monraw"  << YAML::Value << stats.time_monraw;
	if(stats.time_user>0)    out << YAML::Key << "time_user"    << YAML::Value << stats.time_user;
	if(stats.time_system>0)  out << YAML::Key << "time_system"  << YAML::Value << stats.time_system;
	if(stats.time_elapsed>0) out << YAML::Key << "time_elapsed" << YAML::Value << stats.time_elapsed;
	if(stats.mem_virtual>0)  out << YAML::Key << "mem_virtual"  << YAML::Value << stats.mem_virtual;
	if(stats.mem_resident>0) out << YAML::Key << "mem_resident" << YAML::Value << stats.mem_resident;
	out << YAML::EndMap;
	return out;
}

int Shell::system(std::string command, std::string cwd, std::string outFile, std::string errFile, int verbosity, RunStatistics* stats) {
	SystemOptions sysOps;
	sysOps.command = command;
	sysOps.cwd = cwd;
	sysOps.outFile = outFile;
	sysOps.errFile = errFile;
	sysOps.verbosity = verbosity;
	return system(sysOps,stats);
}

int Shell::system(std::string command, int verbosity, RunStatistics* stats) {
	return system(command,".","","",verbosity,stats);
}

static File getTempFile(void) {
#ifndef WIN32
	char buffer[] = P_tmpdir"/XXXXXX";
	int ret = mkstemp(buffer);
	if (ret < 0)
		throw std::runtime_error("Could not create temporary file");
	close(ret);
#else
	char buffer[MAX_PATH];
	char tmpDir[MAX_PATH + 1];
	std::cerr << "Creating temporary file\n";
	if (!GetTempPathA(sizeof(tmpDir), tmpDir))
		throw std::runtime_error("Could not get temporary directory");
	if (!GetTempFileNameA(tmpDir, "dft", 0, buffer))
		throw std::runtime_error("Could not create temporary file");
#endif
	return File(string(buffer));
}

#ifndef WIN32
int Shell::system(const SystemOptions& options, RunStatistics* stats) {
	std::string command = '"' + options.command + '"';

	for (std::string argument : options.arguments) {
		command += " \"" + argument + "\"";
	}

	if (!options.inFile.empty())
		command += "<\"" + options.inFile + '"';
	
	// Pipe stdout to /dev/null if no outFile is specified
	command += " > ";
	if(options.outFile.empty()) {
		command += "/dev/null";
	} else {
		// If there is an outFile specified, pipe stdout to it
		command += '"' + options.outFile + '"';
	}

	// Pipe stderr to /dev/null if no errFile is specified
	command += " 2> ";
	if(options.errFile.empty()) {
		command += "/dev/null";
	} else {
		// If there is an errFile specified, pipe stderr to it
		command += '"' + options.errFile + '"';
	}
	
	// If no statProgram is specified, use this default
	string statProgram = options.statProgram;
	if(options.statProgram.empty()) {
		statProgram = "time -p";
	}
	
	bool removeTmpFile = false;
	bool useStatFile = false;
	bool useBuiltinTime = (0==strncmp("time ",statProgram.c_str(),5)) || (0==strncmp("time",statProgram.c_str(),5));
	bool useBuiltinMemtime = (0==strncmp("memtime ",statProgram.c_str(),5)) || (0==strncmp("memtime",statProgram.c_str(),5));

	// If no statFile was specified, use a temporary
	File statFile = File(options.statFile);
	if(options.statFile.empty()) {
		statFile = getTempFile();
		removeTmpFile = true;
	}
	
	// If statistics are requested, set up the command
	if(stats) {
		*stats = RunStatistics();
		useStatFile = true;
	}
	if(useStatFile) {
		if(useBuiltinTime) {
			command = "(" + statProgram + " " + command + ") 2> " + statFile.getFileRealPath();
		} else {
			command = statProgram + " " + command;
			if(options.errFile.empty()) command += " 2> " + statFile.getFileRealPath();
		}
	}
	
	// Obtain the real path of the specified cwd and enter it
	string realCWD = FileSystem::getRealPath(options.cwd);
	PushD dir(realCWD);
	if(messageFormatter) messageFormatter->reportAction("Entering directory: `" + realCWD + "'", options.verbosity);

	// Execute the command
	int result = 0;
	if(messageFormatter) messageFormatter->reportAction("Executing: " + command, options.verbosity);
	
	//timespec time1, time2;
	//clock_gettime(CLOCK_MONOTONIC_RAW, &time1);
	System::Timer timer;
	result = ::system( command.c_str() );
	if(stats) stats->time_monraw = (float)timer.getElapsedSeconds();
	//clock_gettime(CLOCK_MONOTONIC_RAW, &time2);
	//stats->time_monraw = (float)(time2.tv_sec -time1.tv_sec )
	//                   + (float)(time2.tv_nsec-time1.tv_nsec)*0.000000001;
	
	if(messageFormatter) {
		std::stringstream str;
		str << "Process exited with result: ";
		str << result;
		messageFormatter->reportAction(str.str(), options.verbosity);
	}
	
	// Obtain statistics
	if(stats && useStatFile) {
		if(useBuiltinTime) {
			readTimeStatistics(statFile,*stats);
			if(messageFormatter) messageFormatter->reportAction("Read time statistics", options.verbosity);
		} else if(useBuiltinMemtime) {
			readMemtimeStatisticsFromLog(statFile,*stats);
			if(messageFormatter) messageFormatter->reportAction("Read memtime statistics", options.verbosity);
		}
	}
	
	// Leave the cwd
	dir.popd();
	if(messageFormatter) messageFormatter->reportAction("Exiting directory: `" + realCWD + "'", options.verbosity);
	
	if(removeTmpFile && FileSystem::exists(statFile))
		FileSystem::remove(statFile);
	
	// Check if the command was killed, e.g. by ctrl-c
	if (options.signalHandler && WIFSIGNALED(result)) {
		result = options.signalHandler(result);
	}
	
	// Return the result of the command
	return result;
}
#else // WIN32 version
/* Escape an argument so that CommandLineToArgv parses it correctly */
static void escapeArgument(std::stringstream& out, const std::string& argument) {
	int num_backslashes = 0;
	out << '"';
	for (char c : argument) {
		if (c == '\\') {
			num_backslashes++;
		} else if (c == '"') {
			for (int i = 0; i < num_backslashes; i++)
				out << "\\\\";
			num_backslashes = 0;
			out << "\\\"";
		} else {
			for (int i = 0; i < num_backslashes; i++)
				out << '\\';
			num_backslashes = 0;
			out << c;
		}
	}
	out << '"';
}

int Shell::system(const SystemOptions& options, RunStatistics* stats) {
	STARTUPINFOA startupInfo = {0};
	startupInfo.dwFlags = STARTF_USESTDHANDLES;

	// Child process gets no input
	const char *inputFile = "NUL";
	if (!options.inFile.empty())
		inputFile = options.inFile.c_str();
	startupInfo.hStdInput = CreateFileA(inputFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (startupInfo.hStdInput == INVALID_HANDLE_VALUE) {
		messageFormatter->reportError("Could not open NUL for input?", options.verbosity);
		return 1;
	}
	SetHandleInformation(startupInfo.hStdInput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	// Pipe stdout to NUL if no outFile is specified
	std::string stdOutFile = "NUL";
	if (!options.outFile.empty())
		stdOutFile = options.outFile;

	startupInfo.hStdOutput = CreateFileA(stdOutFile.c_str(), FILE_WRITE_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (startupInfo.hStdOutput == INVALID_HANDLE_VALUE) {
		CloseHandle(startupInfo.hStdInput);
		messageFormatter->reportError("Could not open: " + stdOutFile, options.verbosity);
		return 1;
	}
	SetHandleInformation(startupInfo.hStdOutput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	// Pipe stderr to NUL if no outFile is specified
	std::string stdErrFile = "NUL";
	if (!options.errFile.empty())
		stdErrFile = options.errFile;

	startupInfo.hStdError = CreateFileA(stdErrFile.c_str(), FILE_WRITE_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (startupInfo.hStdError == INVALID_HANDLE_VALUE) {
		CloseHandle(startupInfo.hStdInput);
		CloseHandle(startupInfo.hStdOutput);
		messageFormatter->reportError("Could not open: " + stdErrFile, options.verbosity);
		return 1;
	}
	SetHandleInformation(startupInfo.hStdError, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	std::stringstream argumentstream;
	escapeArgument(argumentstream, options.command);

	for (std::string argument : options.arguments) {
		argumentstream << ' ';
		escapeArgument(argumentstream, argument);
	}

	std::string arguments = argumentstream.str();
	std::unique_ptr<char[]> argument_cstr = std::make_unique<char[]>(arguments.length() + 1);
	strcpy(argument_cstr.get(), arguments.c_str());

	if (messageFormatter)
          messageFormatter->reportAction("Executing: " + arguments, options.verbosity);

	string realCWD = FileSystem::getRealPath(options.cwd);

	PROCESS_INFORMATION processInfo;
	BOOL ret = CreateProcessA(
		options.command.c_str(),
		argument_cstr.get(),
		NULL,
		NULL,
		TRUE,
		CREATE_NO_WINDOW,
		NULL,
		realCWD.c_str(),
		&startupInfo,
		&processInfo
	);

	CloseHandle(startupInfo.hStdInput);
	CloseHandle(startupInfo.hStdOutput);
	CloseHandle(startupInfo.hStdError);

	if (!ret) {
		messageFormatter->reportError("Error executing: " + arguments, options.verbosity);
        return 1;
	}

	WaitForSingleObject(processInfo.hProcess, INFINITE);

	CloseHandle(processInfo.hThread);
	DWORD exitCode = 0;
	if (!GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
		messageFormatter->reportError("Could not get exit code for: " + arguments, options.verbosity);
		exitCode = 1;
	}

	// If statistics are requested, set up the command
	if (stats) {
		*stats = RunStatistics();
		FILETIME start, exit, user, kernel;
		if (!GetProcessTimes(processInfo.hProcess, &start, &exit, &kernel, &user)) {
			messageFormatter->reportError("Could not get timing information for: " + arguments, options.verbosity);
		} else {
			uint64_t startTime = (((uint64_t)start.dwHighDateTime) << 32) + start.dwLowDateTime;
			uint64_t endTime = (((uint64_t)exit.dwHighDateTime) << 32) + exit.dwLowDateTime;
			uint64_t kernelTime = (((uint64_t)kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime;
			uint64_t userTime = (((uint64_t)user.dwHighDateTime) << 32) + user.dwLowDateTime;
			stats->time_elapsed = (endTime - startTime) / 10000000.0;
			stats->time_system = kernelTime / 10000000.0;
			stats->time_user = userTime / 10000000.0;
		}
	}

	if (messageFormatter) {
		std::stringstream str;
		str << "Process exited with result: " << exitCode;
		messageFormatter->reportAction(str.str(), options.verbosity);
	}

	// Return the result of the command
	return exitCode;
}
#endif // WIN32

bool Shell::readSvlStatisticsFromLog(File logFile, Shell::SvlStatistics& stats) {
    if(!FileSystem::exists(logFile)) {
        return true;
    }
    
    std::ifstream log(logFile.getFileRealPath());
    if(!log.is_open()) {
        return true;
    }
    
    static const int MAX_CHARS = 200;
    
    char buffer[MAX_CHARS];
    Shell::SvlStatistics statsTemp;
    while(true) {
        
        // Read the next line
        log.getline(buffer,MAX_CHARS);
        
        // Read the stats from the current line
        if(!Shell::readSvlStatistics(buffer,statsTemp)) {
            stats.maxValues(statsTemp);
        }
        
        // Abort if EOF found
        if(log.eof()) {
            break;
        }
        
        // Handle a fail
        if(log.fail()) {
            
            // If the fail is caused by a line that is too long
            if(log.gcount()==MAX_CHARS-1) {
                
                // Clear the error and continue
                log.clear();
                
                // If the fail was because of something else
            } else {
                
                // Abort
                break;
            }
        }
        
    }
    return false;
}

bool Shell::readSvlStatistics(File file, SvlStatistics& stats) {
    string* contents = FileSystem::load(file);
    
    if(contents) {
        bool result = readSvlStatistics(*contents,stats);
        delete contents;
        return result;
    } else {
        return true;
    }
}

bool Shell::readSvlStatistics(const string& contents, SvlStatistics& stats) {
    return readSvlStatistics(contents.c_str(),stats);
}

bool Shell::readSvlStatistics(const char* contents, SvlStatistics& stats) {
    // Format example:     (* 5 states, 9 transitions, 3.0 Kbytes *)
    int result = sscanf(contents,"    (* %d states, %d transitions, %f Kbytes *)",
                        &stats.max_states,
                        &stats.max_transitions,
                        &stats.max_memory
                        );
    return result != 3;
}


bool Shell::readMemtimeStatisticsFromLog(File logFile, Shell::RunStatistics& stats) {
	if(!FileSystem::exists(logFile)) {
		return true;
	}
	
	std::ifstream log(logFile.getFileRealPath());
	if(!log.is_open()) {
		return true;
	}
	
	static const int MAX_CHARS = 200;
	
	char buffer[MAX_CHARS];
	Shell::RunStatistics statsTemp;
	while(true) {
		
		// Read the next line
		log.getline(buffer,MAX_CHARS);
		
		// Read the stats from the current line
		if(!Shell::readMemtimeStatistics(buffer,statsTemp)) {
			stats.addTimeMaxMem(statsTemp);
		}
		
		// Abort if EOF found
		if(log.eof()) {
			break;
		}
		
		// Handle a fail
		if(log.fail()) {
			
			// If the fail is caused by a line that is too long
			if(log.gcount()==MAX_CHARS-1) {
				
				// Clear the error and continue
				log.clear();
			
			// If the fail was because of something else
			} else {
				
				// Abort
				break;
			}
		}
		
	}
	return false;
}

bool Shell::readMemtimeStatistics(File file, RunStatistics& stats) {
	string* contents = FileSystem::load(file);
	
	if(contents) {
		bool result = readMemtimeStatistics(*contents,stats);
		delete contents;
		return result;
	} else {
		return true;
	}
}
	
bool Shell::readMemtimeStatistics(const string& contents, RunStatistics& stats) {
	return readMemtimeStatistics(contents.c_str(),stats);
}

bool Shell::readMemtimeStatistics(const char* contents, RunStatistics& stats) {
	// Format example: 0.00 user, 0.00 system, 0.10 elapsed -- Max VSize = 4024KB, Max RSS = 76KB
	int result = sscanf(contents,"%f user, %f system, %f elapsed -- Max VSize = %fKB, Max RSS = %fKB",
		   &stats.time_user,
		   &stats.time_system,
		   &stats.time_elapsed,
		   &stats.mem_virtual,
		   &stats.mem_resident
	);
	return result != 5;
}
