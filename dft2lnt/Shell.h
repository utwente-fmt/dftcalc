/*
 * Shell.h
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#ifndef SHELL_H
#define SHELL_H

#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <string>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include "MessageFormatter.h"
#include "FileSystem.h"
#include "System.h"
#include "yaml-cpp/yaml.h"

class Shell {
public:
	
	static int handleSignal(int signal) {
		if(messageFormatter) {
			ConsoleWriter& cw = messageFormatter->getConsoleWriter();
			messageFormatter->getConsoleWriter().appendPostfix();
			stringstream ss;
			ss << "Caught signal `" << signal << "', ";
			cw << " " << ConsoleWriter::Color::CyanBright << ">" << ConsoleWriter::Color::WhiteBright << "  " << ss.str();
			while(true) {
				cw << "terminate " << ConsoleWriter::Color::MagentaBright << "j" << ConsoleWriter::Color::WhiteBright << "ob or " << ConsoleWriter::Color::MagentaBright << "p" << ConsoleWriter::Color::WhiteBright << "rogram?" << ConsoleWriter::Color::Reset;
				char answer[10];
				std::cin.getline(answer,9);
				if(!strncmp("j",answer,1)) {
					break;
				} else if(!strncmp("p",answer,1)) {
					exit(signal);
				} else {
					cw << " " << ConsoleWriter::Color::CyanBright << ">" << ConsoleWriter::Color::WhiteBright << "  ";
				}
			}
			cw.appendPostfix();
		}
		return signal;
	}
	
	typedef int (*SignalHandler)(int signal);
	
	class SystemOptions {
	public:
		std::string command;
		std::string cwd;
		std::string outFile;
		std::string errFile;
		std::string statFile;
		std::string statProgram;
		std::string reportFile;
		int verbosity;
		std::function<int(int)> signalHandler;
		
		SystemOptions():
			command(""),
			cwd("."),
			outFile(""),
			errFile(""),
			statFile(""),
			statProgram(""),
			reportFile(""),
			verbosity(0),
			signalHandler(&handleSignal) {
		}
	};
	
	class RunStatistics {
	public:
		float time_user;
		float time_system;
		float time_elapsed;
		float time_monraw;
		float mem_virtual;
		float mem_resident;
		
		RunStatistics():
			time_user(0.0f),
			time_system(0.0f),
			time_elapsed(0.0f),
			mem_virtual(0.0f),
			mem_resident(0.0f) {
		}
		
		
		void addTimeMaxMem(const RunStatistics& other) {
			time_user    += other.time_user;
			time_system  += other.time_system;
			time_elapsed += other.time_elapsed;
			mem_virtual   = mem_virtual  > other.mem_virtual  ? mem_virtual  : other.mem_virtual;
			mem_resident  = mem_resident > other.mem_resident ? mem_resident : other.mem_resident;
		}
		
		void maxMem(const RunStatistics& other) {
			mem_virtual   = mem_virtual  > other.mem_virtual  ? mem_virtual  : other.mem_virtual;
			mem_resident  = mem_resident > other.mem_resident ? mem_resident : other.mem_resident;
		}
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
	static int system(std::string command, std::string cwd=".", std::string outFile="", std::string errFile="", int verbosity=0, RunStatistics* stats = NULL) {
		SystemOptions sysOps;
		sysOps.command = command;
		sysOps.cwd = cwd;
		sysOps.outFile = outFile;
		sysOps.errFile = errFile;
		sysOps.verbosity = verbosity;
		return system(sysOps,stats);
	}
	
	static int system(std::string command, int verbosity=0, RunStatistics* stats = NULL) {
		return system(command,".","","",verbosity,stats);
	}
	
	static int system(const SystemOptions& options, RunStatistics* stats = NULL) {
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

		// If no statFile was specified, use a temporary
		File statFile = File(options.statFile);
		if(options.statFile.empty()) {
			char buffer[L_tmpnam];
			tmpnam(buffer);
			statFile = File(string(buffer));
			removeTmpFile = true;
		}
		
		// If statistics are requested, set up the command
		if(stats) *stats = RunStatistics();
		if(useStatFile) {
			command = "(" + statProgram + " " + command + ") 2> " + statFile.getFileRealPath();
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
		
		// Obtain statistics
		if(stats && useStatFile) {
			readTimeStatistics(statFile,*stats);
		}
		
		// Leave the cwd
		dir.popd();
		if(messageFormatter) messageFormatter->reportAction("Exiting directory: `" + realCWD + "'", options.verbosity);
		
		if(useStatFile && removeTmpFile) {
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
	
	static bool readMemtimeStatistics(File file, RunStatistics& stats) {
		string* contents = FileSystem::load(file);
		if(contents) {
			bool result = readMemtimeStatistics(*contents,stats);
			delete contents;
			return result;
		} else {
			return true;
		}
	}
		
	static bool readMemtimeStatistics(const string& contents, RunStatistics& stats) {
		return readMemtimeStatistics(contents.c_str(),stats);
	}

	static bool readMemtimeStatistics(const char* contents, RunStatistics& stats) {
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

	static bool readTimeStatistics(File file, RunStatistics& stats) {
		string* contents = FileSystem::load(file);
		
		if(contents) {
			// Format example: 0.00 user, 0.00 system, 0.10 elapsed -- Max VSize = 4024KB, Max RSS = 76KB
			int result = sscanf(contents->c_str(),"\nreal %f\nuser %f\nsys %f",
				   &stats.time_user,
				   &stats.time_system,
				   &stats.time_elapsed
			);
			delete contents;
			if(result == 3) {
				stats.mem_virtual = 0;
				stats.mem_resident = 0;
				return false;
			} else {
				return true;
			}
		} else {
			return true;
		}
	}
};

const YAML::Node& operator>>(const YAML::Node& node, Shell::RunStatistics& stats);
YAML::Emitter& operator<<(YAML::Emitter& out, const Shell::RunStatistics& stats);

#endif // SHELL_H