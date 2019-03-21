#include "executor.h"
#include "Shell.h"
#include <string>

using std::string;
const int VERBOSITY_EXECUTIONS = 2;

void DFT::CommandExecutor::printOutput(const File& file, int status) {
	string* outContents = FileSystem::load(file);
	if(outContents) {
		mf->notifyHighlighted("** OUTPUT of " + file.getFileName() + " **");
		if (status)
			mf->message(*outContents, MessageFormatter::MessageType::Error);
		else
			mf->message(*outContents);
		mf->notifyHighlighted("** END output of " + file.getFileName() + " **");
		delete outContents;
	}
}

string DFT::CommandExecutor::genInputFile(std::string extension) {
	return workingDir + "/" + baseFile + "."
	              + std::to_string(commandNum) + "." + extension;
}

string DFT::CommandExecutor::runCommand(std::string command,
                                        std::string cmdName,
                                        std::vector<File> outputFiles)
{
	Shell::SystemOptions sysOps;
	sysOps.verbosity = VERBOSITY_EXECUTIONS;
	sysOps.cwd = workingDir;
	sysOps.command = command;
	string base = workingDir + "/" + baseFile + "."
	              + std::to_string(commandNum++) + "." + cmdName;
	sysOps.reportFile = base + ".report";
	sysOps.errFile    = base + ".err";
	sysOps.outFile    = base + ".out";
	int result = Shell::system(sysOps);

	if(result) {
		printOutput(File(sysOps.outFile), result);
		printOutput(File(sysOps.errFile), result);
		return "";
	}
	for (File expected : outputFiles) {
		if (!FileSystem::exists(expected)) {
			printOutput(File(sysOps.outFile), result);
			printOutput(File(sysOps.errFile), result);
			return "";
		}
	}
	if (mf->getVerbosity() >= 5) {
		printOutput(File(sysOps.outFile), result);
		printOutput(File(sysOps.errFile), result);
	}
	return sysOps.outFile;
}
