namespace DFT {
class DFTreePrinter;
}

#ifndef DFTREEBCGNODEBUILDER_H
#define DFTREEBCGNODEBUILDER_H

#include <sys/stat.h>
#include <sys/types.h>

#include "DFTree.h"
#include "dft_parser.h"

namespace DFT {

class DFTreeBCGNodeBuilder {
public:
	static const std::string GATE_FAIL;
	static const std::string GATE_ACTIVATE;
	static const std::string GATE_REPAIR;
	static const std::string GATE_RATE_FAIL;
	static const std::string GATE_RATE_REPAIR;
private:
	static const unsigned int VERSION = 2;

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
	int generateBE(FileWriter& out, const DFT::Nodes::BasicEvent& gate);

	int generateSVLBuilder(FileWriter& out, std::string fileName);
	int executeSVL(std::string root, std::string fileName);
public:

	static const std::string LNTROOT;
	static const std::string BCGROOT;

	DFTreeBCGNodeBuilder(std::string root, DFT::DFTree* dft, CompilerContext* cc):
		root(root),
		lntRoot(root+LNTROOT+"/"),
		bcgRoot(root+BCGROOT+"/"),
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
	static int bcgIsValid(std::string bcgFilePath);

	int generate(const DFT::Nodes::Node& node);
	int generate();

	int fancyFileWrite(const std::string& filePath, FileWriter& fw);

};

} // Namespace: DFT

#endif // DFTREEBCGNODEBUILDER_H
