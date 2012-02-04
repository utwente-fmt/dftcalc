/*
 * DFTreeSVLAndLNTBuilder.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

namespace DFT {
class DFTreeSVLAndLNTBuilder;
}

#ifndef DFTREESVLANDLNTBUILDER_H
#define DFTREESVLANDLNTBUILDER_H

#include <set>
#include "DFTree.h"
#include "dft_parser.h"
#include "FileWriter.h"
#include "files.h"

namespace DFT {

class DFTreeSVLAndLNTBuilder {
private:
	std::string root;
	std::string tmp;
	std::string name;
	DFT::DFTree* dft;
	CompilerContext* cc;
	
	FileWriter svl_options;
	FileWriter svl_dependencies;
	FileWriter svl_body;

	std::vector<DFT::Nodes::BasicEvent*> basicEvents;
	std::vector<DFT::Nodes::Gate*> gates;
	std::set<DFT::Nodes::NodeType> neededFiles;

	int validateReferences();

public:

	/**
	 * Constructs a new DFTreeSVLAndLNTBuilder using the specified DFT and
	 * CompilerContext.
	 * @param tmp The root dft2lnt folder
	 * @param tmp The folder in which temporary files will be put in.
	 * @param The name of the SVL script to generate.
	 * @param dft The DFT to be validated.
	 * @param cc The CompilerConstruct used for eg error reports.
	 */
	DFTreeSVLAndLNTBuilder(std::string root, std::string tmp, std::string name, DFT::DFTree* dft, CompilerContext* cc);
	virtual ~DFTreeSVLAndLNTBuilder() {
	}

	/**
	 * Returns the Lotos NT File needed for the specified NodeType.
	 * @return he Lotos NT File needed for the specified NodeType.
	 */
	const std::string& getFileForNodeType(DFT::Nodes::NodeType nodeType);
	
	/**
	 * Start building SVL and Lotos NT specification from the DFT specified
	 * in the constructor.
	 * @return UNDECIDED
	 */
	int build();

	/**
	 * Builds the SVL Options at the top of the SVL script specification.
	 * Affects FileWriters: svl_options
	 * @return UNDECIDED
	 */
	int buildSVLOptions();

	/**
	 * Builds the list of Lotos NT files on which the SVL script depends.
	 * Affects FileWriters: svl_dependencies
	 * @return UNDECIDED
	 */
	int buildSVLHeader();

	/**
	 * Builds the actual composition script from the DFT specification
	 * Affects FileWriters: svl_body
	 * @return UNDECIDED
	 */
	int buildSVLBody();
	
	/**
	 * Builds the SVL command to import a BasicEvent.
	 * Affects FileWriters: svl_body
	 * @return UNDECIDED
	 */
	int buildBasicEvent(int& current, int& total, DFT::Nodes::BasicEvent* basicEvent);

	/**
	 * Builds the SVL command to import a Gate.
	 * Affects FileWriters: svl_body
	 * @param current Reference to the current number of DFT Nodes parsed
	 * @return UNDECIDED
	 */
	int buildGate(int& current, int& total, DFT::Nodes::Gate* gate);

	bool shouldCompileBCG_1(std::string fileName);
	bool shouldGenerateLNT_1(std::string fileName);
	int compileGate(DFT::Nodes::Gate* gate);
	void generateLNT(FileWriter& out, DFT::Nodes::Gate* gate);
	void generateLNTAnd(FileWriter& out, DFT::Nodes::GateAnd* gate);
	int compileLNT(std::string lntFile, std::string bcgFile);
};

} // Namespace: DFT

#endif // DFTREESVLANDLNTBUILDER_H