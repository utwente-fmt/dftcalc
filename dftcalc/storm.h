/*
 * storm.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Enno Ruijters, based on imca.h by Dennis Guck
 */

#ifndef STORM_H
#define STORM_H

#include "FileSystem.h"
#include "decnumber.h"
#include "checker.h"
#include <vector>

class StormRunner : public Checker{
private:
	const File janiFile;
	const File stormExec;
	std::string getCommandOptions(Query q);
public:
	bool runExact;
	StormRunner(MessageFormatter *mf, DFT::CommandExecutor *exec,
	            File stormExec, File model)
		:Checker(mf, exec), stormExec(stormExec), janiFile(model), runExact(0)
	{}

	virtual std::vector<DFT::DFTCalculationResultItem> analyze(vector<Query> queries);
};

#endif
