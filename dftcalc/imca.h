/*
 * imca.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Dennis Guck
 */

#ifndef IMCA_H
#define IMCA_H

#include "FileSystem.h"
#include "decnumber.h"
#include "query.h"
#include <vector>

class IMCAParser {
private:
	std::vector<std::pair<std::string,decnumber<>>> results;
	bool i_isCalculated;
	decnumber<> result;
public:
	/**
	 * Read the result from the specified IMCA output file.
	 * Obtain the result with getResult().
	 * @param file The IMCA output file in which the result can be found.
	 */
	IMCAParser(const File file);

	bool hasResults() {return i_isCalculated;}
	
	/**
	 * Return the result previously read with readOutputFile().
	 * @return The result of the calculation.
	 */
	decnumber<> getResult();

	std::vector<std::pair<std::string,decnumber<>>> getResults();
};

#endif
