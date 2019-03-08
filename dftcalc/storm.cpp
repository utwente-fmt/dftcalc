/*
 * storm.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Enno Ruijters
 */

#include "storm.h"

#include "FileSystem.h"
#include "FileWriter.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>

static const char needle[] = "Result (for initial states): ";

Storm::Storm(const File &file) :
		i_isCalculated(false),
		i_result(-1)
{
	if(!FileSystem::exists(file))
		return;

	std::ifstream input(file.getFileRealPath());
	std::string line;
	std::getline(input, line);
	while (input.good()) {
		if (line.find(needle) != std::string::npos) {
			line.erase(0, strlen(needle));
			i_result = decnumber<>(line);
			i_isCalculated = true;
			break;
		}
		std::getline(input, line);
	}
}

decnumber<> Storm::getResult() {
	return i_result;
}
