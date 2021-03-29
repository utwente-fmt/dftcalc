/*
 * mrmc.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#ifndef MRMC_H
#define MRMC_H

#include "FileSystem.h"
#include "decnumber.h"
#include "checker.h"
#include "query.h"
#include "executor.h"
#include <vector>

class MRMCRunner : public Checker {
private:
	const std::string goalLabel;
	const bool isCtmdp;
	const File modelFile, labFile;
	const File mrmcExec;
public:
	MRMCRunner(MessageFormatter *mf, DFT::CommandExecutor *exec,
	           bool imrmc, File executable, File model, File lab)
	        :Checker(mf, exec),
		 goalLabel(imrmc ? "marked" : "reach"), isCtmdp(!imrmc),
		 modelFile(model), labFile(lab), mrmcExec(executable)
	{}

	virtual std::vector<DFT::DFTCalculationResultItem> analyze(vector<Query> queries);
};

#endif
