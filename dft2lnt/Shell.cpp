/*
 * Shell.cpp
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#include "Shell.h"

MessageFormatter* Shell::messageFormatter = NULL;

const YAML::Node& operator>>(const YAML::Node& node, Shell::RunStatistics& stats) {
	if(const YAML::Node* itemNode = node.FindValue("time_monraw")) {
		*itemNode >> stats.time_monraw;
	}
	if(const YAML::Node* itemNode = node.FindValue("time_user")) {
		*itemNode >> stats.time_user;
	}
	if(const YAML::Node* itemNode = node.FindValue("time_system")) {
		*itemNode >> stats.time_system;
	}
	if(const YAML::Node* itemNode = node.FindValue("time_elapsed")) {
		*itemNode >> stats.time_elapsed;
	}
	if(const YAML::Node* itemNode = node.FindValue("mem_virtual")) {
		*itemNode >> stats.mem_virtual;
	}
	if(const YAML::Node* itemNode = node.FindValue("mem_resident")) {
		*itemNode >> stats.mem_resident;
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

int Shell::system(const SystemOptions& options, RunStatistics* stats) {
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
		char buffer[L_tmpnam];
		tmpnam(buffer);
		statFile = File(string(buffer));
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
	
	if(useStatFile && removeTmpFile && FileSystem::exists(statFile)) {
		FileSystem::remove(statFile);
	}
	
	// Check if the command was killed, e.g. by ctrl-c
#ifdef WIN32
#else
	if (options.signalHandler && WIFSIGNALED(result)) {
		result = options.signalHandler(result);
	}
#endif
	
	// Return the result of the command
	return result;
}

int Shell::execute(const SystemOptions& options, RunStatistics* stats, unordered_map<string,string> environment) {
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
		char buffer[L_tmpnam];
		tmpnam(buffer);
		statFile = File(string(buffer));
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
	
	if(messageFormatter) messageFormatter->reportAction2("Forking..." + string(strerror(errno)), options.verbosity);
	int pid = fork(); // FIXME: vfork()?
	if(pid<0) {
		return -1;
		if(messageFormatter) messageFormatter->reportError("Could not fork: " + string(strerror(errno)), options.verbosity);
	} else if (pid==0) {
		System::sleep(1000);
		if(messageFormatter) messageFormatter->reportAction2("Forked." + string(strerror(errno)), options.verbosity);
		exit(11);
	} else {
		int status = 0;
		waitpid(pid,&status,0);
		result = WEXITSTATUS(status);
		if(messageFormatter) messageFormatter->reportAction2("Joined.", options.verbosity);
	}
	
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
	
	if(useStatFile && removeTmpFile && FileSystem::exists(statFile)) {
		FileSystem::remove(statFile);
	}
	
	// Check if the command was killed, e.g. by ctrl-c
#ifdef WIN32
#else
	if (options.signalHandler && WIFSIGNALED(result)) {
		result = options.signalHandler(result);
	}
#endif
	
	// Return the result of the command
	return result;
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
