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

double Storm::default_result = -1;
static const char needle[] = "Result (for initial states): ";

int Storm::FileHandler::readOutputFile(const File& file) {
	if(!FileSystem::exists(file)) {
		return 1;
	}

	std::ifstream input(file.getFileRealPath());
	std::string line;
	std::getline(input, line);
	while (input.good()) {
		if (line.find(needle) != std::string::npos) {
			line.erase(0, strlen(needle));
			i_result = std::stod(line);
			i_isCalculated = true;
			break;
		}
		std::getline(input, line);
	}
	return 0;
}

double Storm::FileHandler::getResult() {
	return i_result;
}
