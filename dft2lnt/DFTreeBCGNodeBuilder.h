/*
 * DFTreeBCGNodeBuilder.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 * @modified by Dennis Guck
 */

namespace DFT {
class DFTreePrinter;
}

#ifndef DFTREEBCGNODEBUILDER_H
#define DFTREEBCGNODEBUILDER_H

#include <sys/stat.h>
#include <sys/types.h>

#include "DFTree.h"
#include "dft_parser.h"
#include "dft2lnt.h"

namespace DFT {

class DFTreeBCGNodeBuilder {
public:
	static const std::string GATE_FAIL;
	static const std::string GATE_ACTIVATE;
	static const std::string GATE_REPAIR;
	static const std::string GATE_REPAIRED;
	static const std::string GATE_ONLINE;
	static const std::string GATE_RATE_FAIL;
	static const std::string GATE_RATE_REPAIR;
private:
	static const unsigned int VERSION;
	static const int VERBOSE_LNTISVALID;
	static const int VERBOSE_BCGISVALID;
	static const int VERBOSE_SVLEXECUTE;
	static const int VERBOSE_FILE_LNT;
	static const int VERBOSE_FILE_SVL;
	static const int VERBOSE_GENERATION;
	
	std::string root;
	std::string lntRoot;
	std::string bcgRoot;
	DFT::DFTree* dft;
	CompilerContext* cc;

	int generateHeader(FileWriter& out);
	int generateHeaderClose(FileWriter& out);

	int generateAnd(FileWriter& out, const DFT::Nodes::GateAnd& gate);
	int generateOr(FileWriter& out, const DFT::Nodes::GateOr& gate);
	int generateVoting(FileWriter& out, const DFT::Nodes::GateVoting& gate);
	int generatePAnd(FileWriter& out, const DFT::Nodes::GatePAnd& gate);
	int generateSpare(FileWriter& out, const DFT::Nodes::GateWSP& gate);
	int generateFDEP(FileWriter& out, const DFT::Nodes::GateFDEP& gate);
	int generateBE(FileWriter& out, const DFT::Nodes::BasicEvent& gate);
	int generateRU(FileWriter& out, const DFT::Nodes::RepairUnit& gate);
	int generateRU_FCFS(FileWriter& out, const DFT::Nodes::RepairUnit& gate);

	int generateSVLBuilder(FileWriter& out, std::string fileName);
	int executeSVL(std::string root, std::string fileName);
public:
	
	DFTreeBCGNodeBuilder(std::string root, DFT::DFTree* dft, CompilerContext* cc):
		root(root),
		lntRoot(root+DFT2LNT::LNTSUBROOT+"/"),
		bcgRoot(root+DFT2LNT::BCGSUBROOT+"/"),
		dft(dft),
		cc(cc) {
	}
	virtual ~DFTreeBCGNodeBuilder() {
	}
	
	/**
	 * Returns the Lotos NT File needed for the specified node.
	 * @return he Lotos NT File needed for the specified node.
	 */
	static std::string getFileForNode(const DFT::Nodes::Node& node);
	
	int generate(const DFT::Nodes::Node& node, set<string>& triedToGenerate);
	int generate();
	
	/**
	 * Checks is the BCG file at the specified file is a valid BCG file.
	 * Uses the return value of a call to bcg_info.
	 * @param bcgFilePath The BCG file to check.
	 */
	int bcgIsValid(std::string bcgFilePath);

	/**
	 * Checks is the BCG file at the specified file is a valid BCG file.
	 * Uses the return value of a call to bcg_info.
	 * @param bcgFilePath The BCG file to check.
	 */
	int lntIsValid(std::string lntFilePath);
	
	/**
	 * Writes the contents of the specified FileWriter to the file at the
	 * specified path. It will report warnings to the CompilerContext known to
	 * this instance.
	 * @param filePath Path to the file to write to
	 * @param fw FileWriter of which the contents will be written to the file.
	 * @return 0 on success or error code of errno on failure.
	 */
	int fancyFileWrite(const std::string& filePath, FileWriter& fw);
	
};

} // Namespace: DFT

#endif // DFTREEBCGNODEBUILDER_H
