#ifndef FILES_H
#define FILES_H

#include <string>

namespace DFT {

/**
 * The Lotos NT specification files of DFT nodes
 */
namespace Files {
	extern const std::string Unknown;
	extern const std::string BasicEvent;
	extern const std::string GateAnd;
	extern const std::string GateOr;
	extern const std::string GateVoting;
}

/**
 * The file extensions used throughout dft2lnt
 */
namespace FileExtensions {
	extern const std::string DFT;
	extern const std::string LOTOS;
	extern const std::string LOTOSNT;
	extern const std::string BCG;
	extern const std::string SVL;
	extern const std::string EXP;
}

} // Namespace: DFT

#endif // FILES_H