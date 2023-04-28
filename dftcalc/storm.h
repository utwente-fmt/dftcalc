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
	void getCommandOptions(Query q, std::vector<std::string> &opts);
	bool runExact;
	bool mayHaveNondeterminism;
public:
	StormRunner(MessageFormatter *mf, DFT::CommandExecutor *exec,
	            File stormExec, File model, bool exact, bool maybeNonDet)
		:Checker(mf, exec),
		 janiFile(model), stormExec(stormExec), runExact(exact),
		 mayHaveNondeterminism(maybeNonDet)
	{}

	virtual std::vector<DFT::DFTCalculationResultItem> analyze(vector<Query> queries);
};

#endif
