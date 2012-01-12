#ifndef MRMC_H
#define MRMC_H

#include "FileSystem.h"
#include <vector>

class MRMC {
public:

	typedef float T_Chance;
	static float T_Chance_Default;

	class FileHandler {
	private:
		std::vector<T_Chance> results;
		bool m_isCalculated;
		T_Chance result;
	public:
		virtual int generateInputFile(const File& file);
		virtual int readOutputFile(const File& file);
		virtual T_Chance getResult();
	};
public:
	int generateInputFile(File file, const MRMC::FileHandler& settings);
	int readOutputFile(File file, MRMC::FileHandler& output);
};

#endif