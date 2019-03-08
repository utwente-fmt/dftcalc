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
#include <vector>

class Storm {
	bool i_isCalculated;
	decnumber<> i_result;
public:
	/**
	 * Read the result from the specified STORM output file.
	 * Obtain the result with getResult().
	 * @param file The STORM output file in which the result can be found.
	 */
	Storm(const File &file);

	bool hasResult() {return i_isCalculated;}
	
	/**
	 * Return the result previously read with readOutputFile().
	 * @return The result of the calculation.
	 */
	decnumber<> getResult();
};

#endif
