/*
 * imca.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Dennis Guck, Enno Ruijters
 */

#ifndef IMCA_H
#define IMCA_H

#include "FileSystem.h"
#include "decnumber.h"
#include "query.h"
#include "checker.h"
#include "executor.h"
#include <vector>

class IMCARunner : public Checker {
	const File imcaExec;
	const File modelFile;

public:
	IMCARunner(MessageFormatter *mf, DFT::CommandExecutor *exec,
		   File imcaExec, File maFile)
		:Checker(mf, exec), imcaExec(imcaExec), modelFile(maFile)
	{}

	virtual std::vector<DFT::DFTCalculationResultItem> analyze(vector<Query> queries);
};

#endif
