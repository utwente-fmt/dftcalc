/*
 * modest.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Enno Ruijters, based on imca.h by Dennis Guck
 */

#ifndef MODEST_H
#define MODEST_H

#include "FileSystem.h"
#include "decnumber.h"
#include "checker.h"
#include <vector>

class ModestRunner : public Checker{
private:
	const File janiFile;
	const std::string modestCmd;
	std::vector<std::string> getCommandOptions(Query q);
public:
	ModestRunner(MessageFormatter *mf, DFT::CommandExecutor *exec,
	             std::string modestCmd, File model)
		:Checker(mf, exec), modestCmd(modestCmd), janiFile(model)
	{}

	virtual std::vector<DFT::DFTCalculationResultItem> analyze(vector<Query> queries);
};

#endif
