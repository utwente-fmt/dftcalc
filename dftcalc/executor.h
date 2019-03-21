#ifndef DFTCALC_EXECUTOR_H
#define DFTCALC_EXECUTOR_H
#include "MessageFormatter.h"
#include "FileSystem.h"

namespace DFT{
class CommandExecutor {
private:
	MessageFormatter *mf;
	int commandNum;

public:
	const std::string baseFile;
	const std::string workingDir;
	CommandExecutor(MessageFormatter *mf, std::string workingDir,
	                std::string baseFile)
		: mf(mf), workingDir(workingDir), baseFile(baseFile),
	          commandNum(0)
	{}

	void printOutput(const File& file, int status);

	std::string genInputFile(std::string extension);

	std::string runCommand(std::string command,
                               std::string cmdName,
                               std::vector<File> outputFiles);

	std::string runCommand(std::string command,
	                       std::string cmdName,
			       File outputFile)
	{
		std::vector<File> outputs;
		outputs.push_back(outputFile);
		return runCommand(command, cmdName, outputs);
	}

	std::string runCommand(std::string command,
	                       std::string cmdName)
	{
		return runCommand(command, cmdName, std::vector<File>());
	}
};
};

#endif
