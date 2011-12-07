#include <string.h>
#include <time.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include <fcntl.h>

#include "DFTreeBCGNodeBuilder.h"
#include "files.h"
#include "FileWriter.h"
#include "ConsoleWriter.h"

//#ifdef WIN32
//#include "utimes.h"
//#endif

const std::string DFT::DFTreeBCGNodeBuilder::LNTROOT          ("/lntnodes");
const std::string DFT::DFTreeBCGNodeBuilder::BCGROOT          ("/bcgnodes");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_FAIL        ("FAIL");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE    ("ACTIVATE");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_REPAIR      ("REPAIR");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL   ("RATE_FAIL");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_RATE_REPAIR ("RATE_REPAIR");

const unsigned int DFT::DFTreeBCGNodeBuilder::VERSION   = 2;

const int DFT::DFTreeBCGNodeBuilder::VERBOSE_LNTISVALID = 2;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_BCGISVALID = 2;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_SVLEXECUTE = 2;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_FILE_LNT   = 3;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_FILE_SVL   = 3;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_GENERATION = 1;

std::string DFT::DFTreeBCGNodeBuilder::getFileForNode(const DFT::Nodes::Node& node) {
	std::stringstream ss;
	ss << node.getTypeStr();
	ss << "_p" << (node.getParents().size()>0?node.getParents().size():1);
	if(node.isBasicEvent()) {
		const DFT::Nodes::BasicEvent& be = *static_cast<const DFT::Nodes::BasicEvent*>(&node);
		if(be.getMu()==0) {
			ss << "_cold";
		}
	} else if(node.isGate()) {
		const DFT::Nodes::Gate& gate = *static_cast<const DFT::Nodes::Gate*>(&node);
		ss << "_c" << gate.getChildren().size();
		if(node.getType()==DFT::Nodes::GateVotingType) {
			const DFT::Nodes::GateVoting& gateVoting = *static_cast<const DFT::Nodes::GateVoting*>(&node);
			ss << "_t" << gateVoting.getThreshold();
		}
	} else {
		assert(0 && "getFileForNode(): Unknown node type");
	}
	return ss.str();
}

int DFT::DFTreeBCGNodeBuilder::bcgIsValid(std::string bcgFilePath) {
#ifdef WIN32
	int res = system( ("bcg_info \"" + bcgFilePath +"\" > NUL 2> NUL").c_str() );
#else
	int res = system( ("bcg_info \"" + bcgFilePath +"\"").c_str() );
#endif
	return res==0;
}

int DFT::DFTreeBCGNodeBuilder::generateAnd(FileWriter& out, const DFT::Nodes::GateAnd& gate) {
	//int nr_parents = gate.getParents().size();
	//int total = gate.getChildren().size();
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateOr(FileWriter& out, const DFT::Nodes::GateOr& gate) {
	//int nr_parents = gate.getParents().size();
	//int total = gate.getChildren().size();
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateVoting(FileWriter& out, const DFT::Nodes::GateVoting& gate) {
	int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	int threshold = gate.getThreshold();
	out << out.applyprefix << " * Generating Voting(parents=" << nr_parents << ", setting= " << threshold << "/" << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	out << out.applyprefix << "module " << getFileForNode(gate) << "(VOTING) is" << out.applypostfix;
	out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;

		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "VOTING [" << GATE_FAIL << "," << GATE_ACTIVATE << "] (" << threshold << " of NAT, " << total << " of NAT, (BOOL_ARRAY(FALSE)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generatePAnd(FileWriter& out, const DFT::Nodes::GatePAnd& gate) {
	//int nr_parents = gate.getParents().size();
	//int total = gate.getChildren().size();
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateSpare(FileWriter& out, const DFT::Nodes::GateWSP& gate) {
	//int nr_parents = gate.getParents().size();
	//int total = gate.getChildren().size();
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateBE(FileWriter& out, const DFT::Nodes::BasicEvent& be) {
	int nr_parents = be.getParents().size();
	bool cold = be.getMu()>0;

	out << out.applyprefix << " * Generating BE(parents=" << nr_parents << ")" << out.applypostfix;
	generateHeaderClose(out);

	out << out.applyprefix << "module " << getFileForNode(be) << "(BE";
	if(cold) out << "_COLD";
	out << ") is" << out.applypostfix;
	out.appendLine("");
	out.indent();
		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL, " << GATE_RATE_FAIL << " : NAT_NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "BEproc [" << GATE_FAIL << "," << GATE_ACTIVATE << "," << GATE_RATE_FAIL << "](" << nr_parents << " of NAT)" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out.appendLine("");
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generate(const DFT::Nodes::Node& node) {
	ConsoleWriter out(std::cout);
	FileWriter lntOut;
	FileWriter svlOut;
	FileWriter bcgOut;
	std::fstream lntFile;
	std::fstream svlFile;
	std::fstream bcgFile;
	
	//std::cerr << "Generating: " << node.getName() << std::endl;
	
	bool lntGenerationNeeded = false;
	bool bcgGenerationNeeded = false;
	
	std::string fileName = getFileForNode(node);
	std::string lntFileName = fileName + "." + DFT::FileExtensions::LOTOSNT;
	std::string svlFileName = fileName + "." + DFT::FileExtensions::SVL;
	std::string bcgFileName = fileName + "." + DFT::FileExtensions::BCG;
	std::string lntFilePath = lntRoot + lntFileName;
	std::string svlFilePath = lntRoot + svlFileName;
	std::string bcgFilePath = bcgRoot + bcgFileName;
	
	{
		//std::ofstream lntFile;
		lntFile.open(lntFilePath);
		if(!lntFile.is_open()) {
			{
				FILE* f = fopen(lntFilePath.c_str(),"wb");
				if(f) {
					fflush(f);
					fclose(f);
				}
			}
			lntFile.clear();
			lntFile.open(lntFilePath);
		}
		//std::ofstream svlFile (svlFilePath);
		svlFile.open(svlFilePath);
		if(!svlFile.is_open()) {
			{
				FILE* f = fopen(svlFilePath.c_str(),"wb");
				if(f) {
					fflush(f);
					fclose(f);
				}
			}
			svlFile.clear();
			svlFile.open(svlFilePath);
		}
		//std::ofstream bcgFile (bcgFilePath);
		bcgFile.open(bcgFilePath);
		if(!bcgFile.is_open()) {
			{
				FILE* f = fopen(bcgFilePath.c_str(),"wb");
				if(f) {
					fflush(f);
					fclose(f);
				}
			}
			bcgFile.clear();
			bcgFile.open(bcgFilePath);
		}


		if(!lntFile.is_open()) {
			// report error: could not open
			cc->reportError("could not open LNT file: " + lntFilePath);
			//lntGenerationNeeded = true;
			return 1;
		}
		if(!svlFile.is_open()) {
			// report error: could not open
			cc->reportError("could not open SVL file: " + svlFilePath);
			return 1;
		}
		if(!bcgFile.is_open()) {
			// report error: could not open
			cc->reportError("could not open BCG file: " + bcgFilePath);
			//bcgGenerationNeeded = true;
			return 1;
		}
	}

	// Check if the LNT or BCG files need regeneration based on
	// the modification times of the files
	// Check the size of the BCG file as well
	switch(0) default: {
		struct stat lntFileStat;
		struct stat bcgFileStat;
		struct stat bcgValidFileStat;
		
		// Get the info if the BCG file, enable (re)generation on error
		if(stat((lntFilePath).c_str(),&lntFileStat)) {
			lntGenerationNeeded = true;
			cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' not found");
			break;
		}

		// Get the info if the BCG file, enable (re)generation on error
		if(stat((bcgFilePath).c_str(),&bcgFileStat)) {
			bcgGenerationNeeded = true;
			cc->reportAction("BCG file `" + getFileForNode(node) + ".bcg' not found");
			break;
		}
		
		// Get the info if the BCG file, enable (re)generation on error
		if(stat((bcgFilePath+".valid").c_str(),&bcgValidFileStat)) {
			bcgGenerationNeeded = true;
			cc->reportAction("BCG file `" + getFileForNode(node) + ".bcg' is invalid");
			break;
		}

		//cerr << "lntFileStat.st_mtime: " << lntFileStat.st_mtime << endl;
		//cerr << "bcgFileStat.st_mtime: " << bcgFileStat.st_mtime << endl;
		
		// If the LNT file is newer than the BCG file, regeneration is needed
		if(lntFileStat.st_mtime > bcgFileStat.st_mtime) {
			bcgGenerationNeeded = true;
			cc->reportAction("BCG file `" + getFileForNode(node) + ".bcg' is out of date");
			break;
		}
		
		// If the BCG file is empty, regeneration is needed
//		if(bcgFileStat.st_size<1) {
//			bcgGenerationNeeded = true;
//			cc->reportAction("BCG file `" + getFileForNode(node) + ".bcg' is empty");
//			break;
//		}
		
		// If the BCG file is newer than the BCG Valid file, validation is needed
		if(bcgFileStat.st_mtime > bcgValidFileStat.st_mtime) {
			// If the BCG file is valid
			if(bcgIsValid(bcgFilePath)) {
				
				// Update the timestamp of the .valid file to the current time
				//struct utimbuf newTimes = {bcgFileStat.st_atime,bcgFileStat.st_mtime};
				utime( (bcgFilePath+".valid").c_str(), NULL );
				
			
			} else {
				bcgGenerationNeeded = true;
				cc->reportAction("BCG file `" + getFileForNode(node) + ".bcg' is invalid");
				break;
			}
		}
	}
	
	// Check if the LNT file needs regeneration based on the version info in
	// the LNT header
	{
		//cerr << " at " << lntFile.tellg() << endl;
		//cerr << "errflags: " << lntFile.rdstate() << endl;
		
		//if(lntFile.good()) cerr << "IT IS GOOD" << endl;
		//if(lntFile.bad()) cerr << "IT IS BAD" << endl;
		//cerr << "errflags: " << lntFile.rdstate() << endl;
		if(lntFile.is_open()) {
			char header_c[12];
			lntFile.read(header_c,11);
			header_c[11] = '\0';
			//cerr << " at " << lntFile.tellg() << endl;
			//cerr << "errflags: " << lntFile.rdstate() << endl;
			
			// If failed to read 11 characters from the LNT file
			if(lntFile.rdstate()&ifstream::failbit) {
				lntFile.clear();
				lntGenerationNeeded = true;
				cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' is invalid");
				
			// If successfully read 11 characters from the LNT file
			} else {
				std::string header(header_c);
				//cc->message("LNT: `" + string(header_c) + "'");
				
				// If the header does not match
				if(strncmp("(** V",header_c,5)) {
					lntGenerationNeeded = true;
					cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' has invalid header");
				
				// If the header matches, compare the versions
				} else {
					unsigned int version = atoi(&header_c[5]);
					//std::cout << "File: " << version << ", mine: " << VERSION << endl;
					if(version < VERSION) {
						lntGenerationNeeded = true;
						cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' out of date");
					}
				}
			}
		}
	}
	
	// If the LNT file need (re)generation
	if(lntGenerationNeeded) {
		bool generationOK = true;
		
		// Generate header (header comment is not closed!)
		generateHeader(lntOut);
		switch(node.getType()) {
			case DFT::Nodes::BasicEventType: {
				const DFT::Nodes::BasicEvent& be = static_cast<const DFT::Nodes::BasicEvent&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(be) << " (parents=" << be.getParents().size() << ")";
				cc->reportAction(report.toString());
				generateBE(lntOut,be);
				break;
			}
			case DFT::Nodes::GatePhasedOrType: {
				break;
			}
			case DFT::Nodes::GateOrType: {
				const DFT::Nodes::GateOr& gate = static_cast<const DFT::Nodes::GateOr&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (parents=" << gate.getParents().size() << ", children=" << gate.getChildren().size() << ")";
				cc->reportAction(report.toString());
				generateOr(lntOut,gate);
				break;
			}
			case DFT::Nodes::GateAndType: {
				const DFT::Nodes::GateAnd& gate = static_cast<const DFT::Nodes::GateAnd&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (parents=" << gate.getParents().size() << ", children=" << gate.getChildren().size() << ")";
				cc->reportAction(report.toString());
				generateAnd(lntOut,gate);
				break;
			}
			case DFT::Nodes::GateHSPType: {
				break;
			}
			case DFT::Nodes::GateWSPType: {
				const DFT::Nodes::GateWSP& gate = static_cast<const DFT::Nodes::GateWSP&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (parents=" << gate.getParents().size() << ", children=" << gate.getChildren().size() << ")";
				cc->reportAction(report.toString());
				generateSpare(lntOut,gate);
				break;
			}
			case DFT::Nodes::GateCSPType: {
				break;
			}
			case DFT::Nodes::GatePAndType: {
				const DFT::Nodes::GatePAnd& gate = static_cast<const DFT::Nodes::GatePAnd&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (parents=" << gate.getParents().size() << ", children=" << gate.getChildren().size() << ")";
				cc->reportAction(report.toString());
				generatePAnd(lntOut,gate);
				break;
			}
			case DFT::Nodes::GateSeqType: {
				break;
			}
			case DFT::Nodes::GateVotingType: {
				const DFT::Nodes::GateVoting& gate = static_cast<const DFT::Nodes::GateVoting&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (parents=" << gate.getParents().size() << ", children=" << gate.getChildren().size() << ", threshold=" << gate.getThreshold() << ")";
				cc->reportAction(report.toString());
				generateVoting(lntOut,gate);
				break;
			}
			case DFT::Nodes::GateFDEPType: {
				break;
			}
			case DFT::Nodes::GateTransferType: {
				break;
			}
			default:
				generationOK = false;
		}
		if(generationOK) {
			cc->message("generated: " + lntFilePath);
			//cc->reportFile(lntOut.toString());
			fancyFileWrite(lntFilePath,lntOut);
		} else {
			cc->reportErrorAt(node.getLocation(),"Could not generate LNT file for node type `" + node.getTypeStr() + "'");
			cc->reportErrorAt(node.getLocation(),"... for this node: `" + node.getName() + "'");
		}
		//out << lntOut;
	}
	
	// If the LNT file needed (re)generation
	// or the BCG file needs (re)generation
	if(lntGenerationNeeded || bcgGenerationNeeded) {
		bool generationOK = true;
		cc->reportAction("Generating: " + bcgFileName);
		// Generate SVL
		generateSVLBuilder(svlOut,getFileForNode(node));

		if(generationOK) {
			// call SVL
			//cc->reportFile(svlOut.toString());
			fancyFileWrite(svlFilePath,svlOut);
			executeSVL(lntRoot,svlFileName);
			
			// Check if the generation resulted in a valid BCG file
			if(bcgIsValid(bcgFilePath)) {
				
				// Open and close .valid file, making sure it exists
				{
					std::ofstream bcgValidFile(bcgFilePath+".valid");
				}
				
				// Update the timestamp of the .valid file to the current time
				utime( (bcgFilePath+".valid").c_str(), NULL );
				
			
			} else {
				cc->reportErrorAt(node.getLocation(),"Could not generate BCG file for node type `" + node.getTypeStr() + "'");
				cc->reportErrorAt(node.getLocation(),"... for this node: `" + node.getName() + "'");
			}
		} else {
			cc->reportErrorAt(node.getLocation(),"Could not generate LNT file for node type `" + node.getTypeStr() + "'");
			cc->reportErrorAt(node.getLocation(),"... for this node: `" + node.getName() + "'");
		}
	}
	
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generate() {
	std::vector<DFT::Nodes::Node*>::iterator it = dft->getNodes().begin();
	for(;it!=dft->getNodes().end(); it++) {
		generate(**it);
	}
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateHeader(FileWriter& out) {
	struct tm *current;
	time_t now;
	
	time(&now);
	current = localtime(&now);
	
	out << out.applyprefix << "(** V";
	out.outlineRightNext(6,'0'); out << VERSION;
	out << out.applypostfix;
	out << out.applyprefix << " * File generated by dft2lnt on ";
	
	out.outlineRightNext(4,'0'); out << (1900+current->tm_year); // current->tm_year contains 'years since 1900'
	out << "-";
	out.outlineRightNext(2,'0'); out << (1+current->tm_mon); // current->tm_mon contains 'months since January'
	out << "-";
	out.outlineRightNext(2,'0'); out << current->tm_mday;
	out << " ";
	out.outlineRightNext(2,'0'); out << current->tm_hour;
	out << ":";
	out.outlineRightNext(2,'0'); out << current->tm_min;
	out << ":";
	out.outlineRightNext(2,'0'); out << current->tm_sec;
	
	out << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateHeaderClose(FileWriter& out) {
	out << out.applyprefix << " *)" << out.applypostfix;
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateSVLBuilder(FileWriter& out, std::string fileName) {
	generateHeader(out);
	generateHeaderClose(out);
	out << out.applyprefix << "\".." << BCGROOT << "/" << fileName << "." << DFT::FileExtensions::BCG << "\" = strong reduction of \"" << fileName << "." << DFT::FileExtensions::LOTOSNT << "\"" << out.applypostfix;
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::fancyFileWrite(const std::string& filePath, FileWriter& fw) {
	int err = 0;
	std::fstream stream (filePath);
	stream.seekp(0);
	stream << fw.toString();
	stream.flush();
	int newSize = stream.tellp();
	stream.close();
	int fileC = open(filePath.c_str(),O_RDWR);
	if(fileC) {
		struct stat fileStat;
		if(stat((filePath).c_str(),&fileStat)) {
			
		} else {
			cc->flush();
			if(fileStat.st_size>newSize && ftruncate(fileC,newSize)) {
				cc->reportWarning("could not truncate: " + filePath);
				err = errno;
			}
		}
		close(fileC);
	} else {
		cc->reportWarning("could not open for truncate: " + filePath);
		err = errno;
	}
	return err;
}

int DFT::DFTreeBCGNodeBuilder::executeSVL(std::string root, std::string fileName) {
#ifdef WIN32
	int res = system( ("cd " + root + " && svl " + fileName + " && cd - > NUL 2> NUL").c_str() );
#else
	int res = system( ("cd " + root + " && svl " + fileName + " && cd - > /dev/null 2> /dev/null").c_str() );
#endif
	return res;
}
