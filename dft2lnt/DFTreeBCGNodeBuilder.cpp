/*
 * DFTreeBCGNodeBuilder.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg and extended by Dennis Guck
 */

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
#include "FileSystem.h"
#include "FileWriter.h"
#include "ConsoleWriter.h"

//#ifdef WIN32
//#include "utimes.h"
//#endif

const std::string DFT::DFTreeBCGNodeBuilder::GATE_FAIL        ("FAIL");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_ACTIVATE    ("ACTIVATE");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_DEACTIVATE  ("DEACTIVATE");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_REPAIR      ("REPAIR");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_ONLINE      ("ONLINE");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_REPAIRED    ("REPAIRED");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_RATE_FAIL   ("RATE_FAIL");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_RATE_MAINTAIN   ("RATE_MAINTAIN");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_RATE_REPAIR ("RATE_REPAIR");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_RATE_INSPECTION ("RATE_INSPECTION");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_REPAIRING   ("REPAIRING");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_INSPECT   ("INSPECT");
const std::string DFT::DFTreeBCGNodeBuilder::GATE_INSPECTED   ("INSPECTED");

const unsigned int DFT::DFTreeBCGNodeBuilder::VERSION   = 6;

const int DFT::DFTreeBCGNodeBuilder::VERBOSE_LNTISVALID = 2;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_BCGISVALID = 2;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_SVLEXECUTE = 2;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_FILE_LNT   = 3;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_FILE_SVL   = 3;
const int DFT::DFTreeBCGNodeBuilder::VERBOSE_GENERATION = 1;

std::string DFT::DFTreeBCGNodeBuilder::getFileForNode(const DFT::Nodes::Node& node) {
	std::stringstream ss;
	
	if(node.getType()==DFT::Nodes::GateVotingType) {
		ss << "v";
	}
	
	ss << node.getTypeStr();
	ss << "_p" << (node.getParents().size()>0?node.getParents().size():1);
	if(node.isBasicEvent()) {
		const DFT::Nodes::BasicEvent& be = *static_cast<const DFT::Nodes::BasicEvent*>(&node);
		if(be.getMu()==0) {
			ss << "_cold";
		}
        if(be.getMaintain()>0) {
            ss << "_maintain";
        }
		// extension for a repairable BE
		if(be.getRepair()>0) {
            ss << "_repair";
        }
        if(be.getLambda()==0) {
            ss << "_dummy";
        }
		if(be.getFailed()) {
			ss << "_failed";
		}
	} else if(node.isGate()) {
		const DFT::Nodes::Gate& gate = *static_cast<const DFT::Nodes::Gate*>(&node);
		ss << "_c" << gate.getChildren().size();
		if(node.getType()==DFT::Nodes::GateVotingType) {
			const DFT::Nodes::GateVoting& gateVoting = *static_cast<const DFT::Nodes::GateVoting*>(&node);
			ss << "_t" << gateVoting.getThreshold();
		} if(node.getType()==DFT::Nodes::GateFDEPType) {
			const DFT::Nodes::GateFDEP& gateFDEP = *static_cast<const DFT::Nodes::GateFDEP*>(&node);
			ss << "_d" << gateFDEP.getDependers().size();
		}
		if(node.isRepairable()) {
			// FIXME: add this directly as gate information
			int repairable=0;
			for(size_t n = 0; n<gate.getChildren().size(); ++n) {

				// Get the current child and associated childID
				const DFT::Nodes::Node& child = *gate.getChildren().at(n);
				if(child.isRepairable())
					repairable++;
			}
			ss << "_r" << repairable;//gate.getRepairableChildren();
		}
	} else {
		assert(0 && "getFileForNode(): Unknown node type");
	}

	return ss.str();
}

int DFT::DFTreeBCGNodeBuilder::bcgIsValid(std::string bcgFilePath) {
	std::string command("bcg_info \"" + bcgFilePath +"\"");
	if(cc->getVerbosity()<VERBOSE_BCGISVALID) {
#ifdef WIN32
		command += " > NUL 2> NUL";
#else
		command += " > /dev/null 2> /dev/null";
#endif
	}
	PushD dir(bcgRoot);
	int res = system( command.c_str() );
	dir.popd();
	return res==0;
}

int DFT::DFTreeBCGNodeBuilder::lntIsValid(std::string lntFilePath) {
	std::string command("lnt.open \"" + lntFilePath +"\" -");
	if(cc->getVerbosity()<VERBOSE_LNTISVALID) {
#ifdef WIN32
		command += " > NUL 2> NUL";
#else
		command += " > /dev/null 2> /dev/null";
#endif
	}
	PushD dir(lntRoot);
	int res = system( command.c_str() );
	dir.popd();
	return res==0;
}


int DFT::DFTreeBCGNodeBuilder::generateAnd(FileWriter& out, const DFT::Nodes::GateAnd& gate) {
	int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();

	out << out.applyprefix << " * Generating And(parents=" << nr_parents << ", children= " << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	if(gate.isRepairable()){
		// FIXME: add this directly as gate information
		int repairable = 0;

		for(size_t n = 0; n<gate.getChildren().size(); ++n) {

			// Get the current child and associated childID
			const DFT::Nodes::Node& child = *gate.getChildren().at(n);
			if(child.isRepairable())
				repairable++;
		}

		out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_VOTING_REPAIR) is" << out.applypostfix;
		out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL, " << GATE_ONLINE << " : NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
		out << out.applyprefix << "VOTING_K [" << GATE_FAIL << "," << GATE_ACTIVATE << "," << GATE_ONLINE << "] (" << total << " of NAT, " << total << " of NAT, (BOOL_ARRAY(FALSE)), (BOOL_ARRAY(FALSE)), " << repairable << " of NAT)" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	} else {
		out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_VOTING) is" << out.applypostfix;
		out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL] is" << out.applypostfix;
		out.indent();
		out << out.applyprefix << "VOTING [" << GATE_FAIL << "," << GATE_ACTIVATE << "] (" << total << " of NAT, " << total << " of NAT, (BOOL_ARRAY(FALSE)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	}
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateOr(FileWriter& out, const DFT::Nodes::GateOr& gate) {
	int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	out << out.applyprefix << " * Generating Or(parents=" << nr_parents << ", children= " << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	if(gate.isRepairable()){
		// FIXME: add this directly as gate information
		int repairable = 0;

		for(size_t n = 0; n<gate.getChildren().size(); ++n) {

			// Get the current child and associated childID
			const DFT::Nodes::Node& child = *gate.getChildren().at(n);
			if(child.isRepairable())
				repairable++;
		}

		out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_VOTING_REPAIR) is" << out.applypostfix;
		out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL, " << GATE_ONLINE << " : NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
		out << out.applyprefix << "VOTING_K [" << GATE_FAIL << "," << GATE_ACTIVATE << "," << GATE_ONLINE << "] (" << 1 << " of NAT, " << total << " of NAT, (BOOL_ARRAY(FALSE)), (BOOL_ARRAY(FALSE)), " << repairable << " of NAT)" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	} else {

		out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_VOTING) is" << out.applypostfix;
		out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;

		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "VOTING [" << GATE_FAIL << "," << GATE_ACTIVATE << "] (" << "1 of NAT, " << total << " of NAT, (BOOL_ARRAY(FALSE)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	}

	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateVoting(FileWriter& out, const DFT::Nodes::GateVoting& gate) {
	int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	int threshold = gate.getThreshold();
	out << out.applyprefix << " * Generating Voting(parents=" << nr_parents << ", setting= " << threshold << "/" << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	if(gate.isRepairable()){
		// FIXME: add this directly as gate information
		int repairable = 0;

		for(size_t n = 0; n<gate.getChildren().size(); ++n) {

			// Get the current child and associated childID
			const DFT::Nodes::Node& child = *gate.getChildren().at(n);
			if(child.isRepairable())
				repairable++;
		}

		out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_VOTING_REPAIR) is" << out.applypostfix;
		out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL, " << GATE_ONLINE << " : NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
		out << out.applyprefix << "VOTING_K [" << GATE_FAIL << "," << GATE_ACTIVATE << "," << GATE_ONLINE << "] (" << threshold << " of NAT, " << total << " of NAT, (BOOL_ARRAY(FALSE)), (BOOL_ARRAY(FALSE)), " << repairable << " of NAT)" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	} else {

		out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_VOTING) is" << out.applypostfix;
		out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;

		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "VOTING [" << GATE_FAIL << "," << GATE_ACTIVATE << "] (" << threshold << " of NAT, " << total << " of NAT, (BOOL_ARRAY(FALSE)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;

	}
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generatePAnd(FileWriter& out, const DFT::Nodes::GatePAnd& gate) {
	int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	out << out.applyprefix << " * Generating PAnd(parents=" << nr_parents << ", children= " << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_PAND) is" << out.applypostfix;
	out.indent();
		out << out.applyprefix << "type NAT_ARRAY is array[1.." << total << "] of NAT end type" << out.applypostfix;
		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "PAND [" << GATE_FAIL << "," << GATE_ACTIVATE << "] (" << total << " of NAT, (NAT_ARRAY(0 of NAT)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generatePor(FileWriter& out, const DFT::Nodes::GatePor& gate) {
	int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	out << out.applyprefix << " * Generating POR(parents=" << nr_parents << ", children= " << total << ")" << out.applypostfix;
	generateHeaderClose(out);
    
	out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_POR) is" << out.applypostfix;
	out.indent();
    out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;
    out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL] is" << out.applypostfix;
    out.indent();
    out << out.applyprefix << "POR [" << GATE_FAIL << "," << GATE_ACTIVATE << "] (" << total << " of NAT, (BOOL_ARRAY(FALSE)))" << out.applypostfix;
    out.outdent();
    out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateSpare(FileWriter& out, const DFT::Nodes::GateWSP& gate) {
	int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	out << out.applyprefix << " * Generating Spare(parents=" << nr_parents << ", setting= " << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_SPARE) is" << out.applypostfix;
	out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;

		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "SPARE [" << GATE_FAIL << "," << GATE_ACTIVATE << "] (" << total << " of NAT, (BOOL_ARRAY(TRUE)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateFDEP(FileWriter& out, const DFT::Nodes::GateFDEP& gate) {
	//int nr_parents = gate.getParents().size();
	int dependers = gate.getDependers().size();
	out << out.applyprefix << " * Generating FDEP(dependers= " << dependers << ")" << out.applypostfix;
	generateHeaderClose(out);

	out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_FDEP) is" << out.applypostfix;
	out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << dependers << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "FDEP [" << GATE_FAIL << "," << GATE_ACTIVATE << "] (" << dependers << " of NAT, (BOOL_ARRAY(FALSE)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateBE(FileWriter& out, const DFT::Nodes::BasicEvent& be) {
	int nr_parents = be.getParents().size();
	bool cold = be.getMu()==0;
    bool dummy = be.getLambda()==0;

	// new boolean variable for repairable BE
	bool repair = be.getRepair() > 0;
    
    // check if it is maintainabel
    bool maintain = be.getMaintain() > 0;

	std::string initialState;
	if(be.getFailed()) initialState = "FAILING";
	else if(repair) initialState = "UP";
	else initialState = "DORMANT";
    if(!dummy){
	out << out.applyprefix << " * Generating BE(parents=" << nr_parents << ")" << out.applypostfix;
	generateHeaderClose(out);
	out << out.applyprefix << "module " << getFileForNode(be) << "(TEMPLATE_BE";
	// use repair template if  repairable
	out << (repair?"_REPAIR) is":maintain?"_MAINTAIN) is":") is") << out.applypostfix;
	out.appendLine("");
	out.indent();
		if(repair)
			out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL, " << GATE_RATE_FAIL << " : NAT_NAT_CHANNEL, "
			<< GATE_REPAIR << " : NAT_CHANNEL, " << GATE_REPAIRED << " : NAT_BOOL_CHANNEL, " << GATE_ONLINE << " : NAT_CHANNEL] is" << out.applypostfix;
		else if(maintain)
            out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL, " << "MAINTAIN : NAT_CHANNEL, " << GATE_RATE_FAIL << " : NAT_NAT_CHANNEL, " << GATE_RATE_MAINTAIN << " : NAT_NAT_CHANNEL] is" << out.applypostfix;
        else
			out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL, " << GATE_RATE_FAIL << " : NAT_NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
			if(repair)
				out << out.applyprefix << "BEproc [" << GATE_FAIL << "," << GATE_ACTIVATE << "," << GATE_RATE_FAIL << "," << GATE_REPAIR << "," << GATE_REPAIRED << "," << GATE_ONLINE <<
				"](" << nr_parents << " of NAT";
			else if(maintain)
                out << out.applyprefix << "BEproc [" << GATE_FAIL << "," << GATE_ACTIVATE << ", MAINTAIN, " << GATE_RATE_FAIL << "," << GATE_RATE_MAINTAIN << "](" << nr_parents << " of NAT";
            else
				out << out.applyprefix << "BEproc [" << GATE_FAIL << "," << GATE_ACTIVATE << "," << GATE_RATE_FAIL << "](" << nr_parents << " of NAT";
			out << ", " << (cold?"TRUE":"FALSE");
			out << ", " << initialState;
			out << ")" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out.appendLine("");
	out << out.applyprefix << "end module" << out.applypostfix;
    }else{
        out << out.applyprefix << " * Generating BE(parents=" << nr_parents << ")" << out.applypostfix;
        generateHeaderClose(out);
        out << out.applyprefix << "module " << getFileForNode(be) << "(TEMPLATE_BE_DUMMY) is" << out.applypostfix;
        out.appendLine("");
        out.indent();
			out << out.applyprefix << "process MAIN [" << GATE_FAIL << " : NAT_CHANNEL, " << GATE_ACTIVATE << " : NAT_BOOL_CHANNEL] is" << out.applypostfix;
		out.indent();
            out << out.applyprefix << "BEproc [" << GATE_FAIL << "," << GATE_ACTIVATE << "](" << nr_parents << " of NAT";
        //out << ", " << (cold?"TRUE":"FALSE");
        out << ", " << initialState;
        out << ")" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
        out.outdent();
        out.appendLine("");
        out << out.applyprefix << "end module" << out.applypostfix;
    }
	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateRU(FileWriter& out, const DFT::Nodes::RepairUnit& gate) {
	//int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	out << out.applyprefix << " * Generating RepairUnit(dependers=" << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_REPAIRUNIT_ARB) is" << out.applypostfix;
	out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "type NAT_ARRAY is array[1.." << total << "] of NAT end type" << out.applypostfix;

		out << out.applyprefix << "process MAIN [" << GATE_REPAIR << " : NAT_CHANNEL, " << GATE_REPAIRED << " : NAT_BOOL_CHANNEL, " << GATE_RATE_REPAIR  << " : NAT_NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "REPAIRUNIT [" << GATE_REPAIR << "," << GATE_REPAIRED << "," << GATE_RATE_REPAIR << "] (" << total << " of NAT, (BOOL_ARRAY(FALSE)), (BOOL_ARRAY(FALSE)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateRU_FCFS(FileWriter& out, const DFT::Nodes::RepairUnit& gate) {
	//int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	out << out.applyprefix << " * Generating RepairUnit(dependers=" << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_REPAIRUNIT) is" << out.applypostfix;
	out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "type NAT_ARRAY is array[1.." << total << "] of NAT end type" << out.applypostfix;

		out << out.applyprefix << "process MAIN [" << GATE_REPAIR << " : NAT_CHANNEL, " << GATE_REPAIRED << " : NAT_BOOL_CHANNEL, " << GATE_RATE_REPAIR  << " : NAT_NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "REPAIRUNIT [" << GATE_REPAIR << "," << GATE_REPAIRED << "," << GATE_RATE_REPAIR << "] (" << total << " of NAT, (BOOL_ARRAY(FALSE)),(NAT_ARRAY(0)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateRU_Prio(FileWriter& out, const DFT::Nodes::RepairUnit& gate) {
	//int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	out << out.applyprefix << " * Generating RepairUnit(dependers=" << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_REPAIRUNIT_PRIO) is" << out.applypostfix;
	out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "type NAT_ARRAY is array[1.." << total << "] of NAT end type" << out.applypostfix;

		out << out.applyprefix << "process MAIN [" << GATE_REPAIR << " : NAT_CHANNEL, " << GATE_REPAIRED << " : NAT_BOOL_CHANNEL, " << GATE_RATE_REPAIR  << " : NAT_NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "REPAIRUNIT [" << GATE_REPAIR << "," << GATE_REPAIRED << "," << GATE_RATE_REPAIR << "] (" << total << " of NAT, (BOOL_ARRAY(FALSE)), (BOOL_ARRAY(FALSE)), (NAT_ARRAY(";
			// obtain priorities of BEs
			for(size_t n = 0; n<gate.getChildren().size()-1; ++n) {
				// Get the current child and associated childID
				const DFT::Nodes::BasicEvent& child = static_cast<const DFT::Nodes::BasicEvent&>(*gate.getChildren().at(n));
				out << child.getPriority() << ",";
			}
			const DFT::Nodes::BasicEvent& child = static_cast<const DFT::Nodes::BasicEvent&>(*gate.getChildren().at(gate.getChildren().size()-1));
			out << child.getPriority();
			out << ")))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateRU_Nd(FileWriter& out, const DFT::Nodes::RepairUnit& gate) {
	//int nr_parents = gate.getParents().size();
	int total = gate.getChildren().size();
	out << out.applyprefix << " * Generating RepairUnit(dependers=" << total << ")" << out.applypostfix;
	generateHeaderClose(out);

	out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_REPAIRUNIT_ND) is" << out.applypostfix;
	out.indent();

		out << out.applyprefix << "type BOOL_ARRAY is array[1.." << total << "] of BOOL end type" << out.applypostfix;
		out << out.applyprefix << "type NAT_ARRAY is array[1.." << total << "] of NAT end type" << out.applypostfix;

		out << out.applyprefix << "process MAIN [" << GATE_REPAIR << " : NAT_CHANNEL, " << GATE_REPAIRED << " : NAT_BOOL_CHANNEL, " << GATE_RATE_REPAIR  << " : NAT_NAT_CHANNEL, " <<
				GATE_REPAIRING << " : NAT_CHANNEL] is" << out.applypostfix;
		out.indent();
			out << out.applyprefix << "REPAIRUNIT [" << GATE_REPAIR << "," << GATE_REPAIRED << "," << GATE_RATE_REPAIR << "," << GATE_REPAIRING << "] (" << total << " of NAT, (BOOL_ARRAY(FALSE)), (BOOL_ARRAY(FALSE)))" << out.applypostfix;
		out.outdent();
		out << out.applyprefix << "end process" << out.applypostfix;
	out.outdent();
	out << out.applyprefix << "end module" << out.applypostfix;

	return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateInspection(FileWriter& out, const DFT::Nodes::Inspection& gate) {
    int total = gate.getChildren().size();
    // TODO: define how to set and get phases for Inspection and Replacement
    int phases = 1;
    out << out.applyprefix << " * Generating Inspection(dependers=" << total << ")" << out.applypostfix;
    generateHeaderClose(out);
    
    out << out.applyprefix << "module " << getFileForNode(gate) << "(TEMPLATE_INSPECTION) is" << out.applypostfix;
    out.indent();

        out << out.applyprefix << "process MAIN [" << GATE_INSPECT << " : NAT_CHANNEL, " << GATE_REPAIR << " : NAT_CHANNEL, " << GATE_REPAIRED  << " : NAT_BOOL_CHANNEL, " <<
            GATE_RATE_INSPECTION << " : NAT_CHANNEL, " << GATE_INSPECTED << " : NAT_CHANNEL ] is" << out.applypostfix;
        out.indent();
    
            out << out.applyprefix << "INSPECTION [" << GATE_INSPECT << "," << GATE_REPAIR << "," << GATE_REPAIRED << "," << GATE_RATE_INSPECTION << "," << GATE_INSPECTED << "] (" << total << " of NAT," << phases << " of NAT)" << out.applypostfix;
    
        out.outdent();
        out << out.applyprefix << "end process" << out.applypostfix;
    
    out.outdent();
    out << out.applyprefix << "end module" << out.applypostfix;

    return 0;
}

int DFT::DFTreeBCGNodeBuilder::generateReplacement(FileWriter& out, const DFT::Nodes::Replacement& gate) {
    
    return 0;
}

int DFT::DFTreeBCGNodeBuilder::generate(const DFT::Nodes::Node& node, set<string>& triedToGenerate) {

	// If the LNT file for the node was not generated before in this iteration
	if(triedToGenerate.find(getFileForNode(node)) != triedToGenerate.end()) {
		return 0;
	}
	triedToGenerate.insert(getFileForNode(node));

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
		struct stat lntValidFileStat;
		struct stat bcgValidFileStat;
		
		// Get the info if the BCG file, enable (re)generation on error
		if(stat((lntFilePath).c_str(),&lntFileStat)) {
			lntGenerationNeeded = true;
			cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' not found",VERBOSE_GENERATION);
			break;
		}

		// Get the info if the LNT Valid file, enable (re)generation on error
		if(stat((lntFilePath+".valid").c_str(),&lntValidFileStat)) {
			lntGenerationNeeded = true;
			cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' is invalid",VERBOSE_GENERATION);
			break;
		}

		// If the LNT file is newer than the LNT Valid file, validation is needed
		if(lntFileStat.st_mtime > lntValidFileStat.st_mtime) {
			// If the LNT file is valid
			if(lntIsValid(lntFilePath)) {
				
				// Update the timestamp of the .valid file to the current time
				utime( (lntFilePath+".valid").c_str(), NULL );
				
			} else {
				lntGenerationNeeded = true;
				cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' is invalid",VERBOSE_GENERATION);
				break;
			}
		}

		// Get the info if the BCG file, enable (re)generation on error
		if(stat((bcgFilePath).c_str(),&bcgFileStat)) {
			bcgGenerationNeeded = true;
			cc->reportAction("BCG file `" + getFileForNode(node) + ".bcg' not found",VERBOSE_GENERATION);
			break;
		}
		
		// Get the info if the BCG Valid file, enable (re)generation on error
		if(stat((bcgFilePath+".valid").c_str(),&bcgValidFileStat)) {
			bcgGenerationNeeded = true;
			cc->reportAction("BCG file `" + getFileForNode(node) + ".bcg' is invalid",VERBOSE_GENERATION);
			break;
		}

		//cerr << "lntFileStat.st_mtime: " << lntFileStat.st_mtime << endl;
		//cerr << "bcgFileStat.st_mtime: " << bcgFileStat.st_mtime << endl;
		
		// If the LNT file is newer than the BCG file, regeneration is needed
		if(lntFileStat.st_mtime > bcgFileStat.st_mtime) {
			bcgGenerationNeeded = true;
			cc->reportAction("BCG file `" + getFileForNode(node) + ".bcg' is out of date",VERBOSE_GENERATION);
			break;
		}
		
		// If the BCG file is newer than the BCG Valid file, validation is needed
		if(bcgFileStat.st_mtime > bcgValidFileStat.st_mtime) {
			// If the BCG file is valid
			if(bcgIsValid(bcgFilePath)) {
				
				// Update the timestamp of the .valid file to the current time
				utime( (bcgFilePath+".valid").c_str(), NULL );
				
			
			} else {
				bcgGenerationNeeded = true;
				cc->reportAction("BCG file `" + getFileForNode(node) + ".bcg' is invalid",VERBOSE_GENERATION);
				break;
			}
		}
	}

	// Check if the LNT file needs regeneration based on the version info in
	// the LNT header
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
			cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' is invalid",VERBOSE_GENERATION);
			
		// If successfully read 11 characters from the LNT file
		} else {
			std::string header(header_c);
			//cc->message("LNT: `" + string(header_c) + "'");
			
			// If the header does not match
			if(strncmp("(** V",header_c,5)) {
				lntGenerationNeeded = true;
				cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' has invalid header",VERBOSE_GENERATION);
			
			// If the header matches, compare the versions
			} else {
				unsigned int version = atoi(&header_c[5]);
				//std::cout << "File: " << version << ", mine: " << VERSION << endl;
				if(version < VERSION) {
					lntGenerationNeeded = true;
					cc->reportAction("LNT file `" + getFileForNode(node) + ".lnt' out of date",VERBOSE_GENERATION);
				}
			}
			char buffer[200];
			vector<string> dependencies;
			while(lntFile.getline(buffer,200)) {
				//cerr << "Found line: " << string(buffer) << endl;
				char* deps = buffer;
				if(!strncasecmp("module",deps,6)) {
					
					// Skip 'module.*('
					deps+=7;
					while(*deps != '\0' && *deps != '(') ++deps;
					deps++;
					
					// Read the list of dependencies until ')' is reached
					while(*deps != '\0' && *deps != ')') {
						while(isspace(*deps)) ++deps;
						if(*deps==')' || *deps == '\0') break;
						char* enddep = deps;
						while(!isspace(*enddep) && *enddep != ')' && *enddep != '\0') ++enddep;
						dependencies.push_back(string(deps,enddep));
						deps = enddep;
					}
					
					// We have found the module list
					break;
				}
			}
//			cerr << "Found dependencies:";
//			for(string s: dependencies) {
//				cerr << " " << s;
//			}
//			cerr << endl;
		}
	}
	
	// If the LNT file needs (re)generation
	bool lntGenerationOK = true;
	if(lntGenerationNeeded) {
		// Generate header (header comment is not closed!)
		generateHeader(lntOut);
		switch(node.getType()) {
			case DFT::Nodes::BasicEventType: {
				const DFT::Nodes::BasicEvent& be = static_cast<const DFT::Nodes::BasicEvent&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(be) << " (parents=" << be.getParents().size() << ")";
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
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
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
				generateOr(lntOut,gate);
				break;
			}
			case DFT::Nodes::GateAndType: {
				const DFT::Nodes::GateAnd& gate = static_cast<const DFT::Nodes::GateAnd&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (parents=" << gate.getParents().size() << ", children=" << gate.getChildren().size() << ")";
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
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
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
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
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
				generatePAnd(lntOut,gate);
				break;
			}
            case DFT::Nodes::GatePorType: {
				const DFT::Nodes::GatePor& gate = static_cast<const DFT::Nodes::GatePor&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (parents=" << gate.getParents().size() << ", children=" << gate.getChildren().size() << ")";
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
				generatePor(lntOut,gate);
				break;
			}
			case DFT::Nodes::GateSeqType: {
				break;
			}
			case DFT::Nodes::GateVotingType: {
				const DFT::Nodes::GateVoting& gate = static_cast<const DFT::Nodes::GateVoting&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (parents=" << gate.getParents().size() << ", children=" << gate.getChildren().size() << ", threshold=" << gate.getThreshold() << ")";
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
				generateVoting(lntOut,gate);
				break;
			}
			case DFT::Nodes::GateFDEPType: {
				const DFT::Nodes::GateFDEP& gate = static_cast<const DFT::Nodes::GateFDEP&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (children=" << gate.getChildren().size() << ", dependers=" << gate.getDependers().size() << ")";
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
				generateFDEP(lntOut,gate);
				break;
			}
			case DFT::Nodes::RepairUnitType: {
				const DFT::Nodes::RepairUnit& gate = static_cast<const DFT::Nodes::RepairUnit&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (children=" << gate.getChildren().size() << ", dependers=" << gate.getDependers().size() << ")";
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
				generateRU(lntOut,gate);
				break;
			}
			case DFT::Nodes::RepairUnitFcfsType: {
				const DFT::Nodes::RepairUnit& gate = static_cast<const DFT::Nodes::RepairUnit&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (children=" << gate.getChildren().size() << ", dependers=" << gate.getDependers().size() << ")";
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
				generateRU_FCFS(lntOut,gate);
				break;
			}
			case DFT::Nodes::RepairUnitPrioType: {
				const DFT::Nodes::RepairUnit& gate = static_cast<const DFT::Nodes::RepairUnit&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (children=" << gate.getChildren().size() << ", dependers=" << gate.getDependers().size() << ")";
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
				generateRU_Prio(lntOut,gate);
				break;
			}
			case DFT::Nodes::RepairUnitNdType: {
				const DFT::Nodes::RepairUnit& gate = static_cast<const DFT::Nodes::RepairUnit&>(node);
				FileWriter report;
				report << "Generating " << getFileForNode(node) << " (children=" << gate.getChildren().size() << ", dependers=" << gate.getDependers().size() << ")";
				cc->reportAction(report.toString(),VERBOSE_GENERATION);
				generateRU_Nd(lntOut,gate);
				break;
			}
            case DFT::Nodes::InspectionType: {
                const DFT::Nodes::Inspection& gate = static_cast<const DFT::Nodes::Inspection&>(node);
                FileWriter report;
                report << "Generating " << getFileForNode(node) << " (children=" << gate.getChildren().size() << ", dependers=" << gate.getDependers().size() << ")";
                cc->reportAction(report.toString(),VERBOSE_GENERATION);
                generateInspection(lntOut,gate);
                break;
            }
            case DFT::Nodes::ReplacementType: {
                const DFT::Nodes::Replacement& gate = static_cast<const DFT::Nodes::Replacement&>(node);
                FileWriter report;
                report << "Generating " << getFileForNode(node) << " (children=" << gate.getChildren().size() << ", dependers=" << gate.getDependers().size() << ")";
                cc->reportAction(report.toString(),VERBOSE_GENERATION);
                generateReplacement(lntOut,gate);
                break;
            }
			case DFT::Nodes::GateTransferType: {
				break;
			}
			default:
				lntGenerationOK = false;
		}
		// If LNT file generation went OK so far
		if(lntGenerationOK) {
			// Write it to file
			fancyFileWrite(lntFilePath,lntOut);
			// Test if it is valid
			lntGenerationOK = lntIsValid(lntFilePath);
		}

		// If LNT file generation went OK
		if(lntGenerationOK) {
			// Open and close .valid file, making sure it exists
			{
				std::ofstream lntValidFile(lntFilePath+".valid");
			}
			
			// Update the timestamp of the .valid file to the current time
			utime( (lntFilePath+".valid").c_str(), NULL );
			
			// Report
			cc->reportSuccess("Generated: " + lntFilePath,VERBOSE_GENERATION);
			cc->reportFile(lntFileName,lntOut.toString(),VERBOSE_FILE_LNT);
		
		// If generation failed for some reason, report
		} else {
			cc->reportError("Could not generate LNT file `" + lntFileName +  "' for node type `" + node.getTypeStr() + "'");
		}
		//out << lntOut;
	
	// If regeneration of LNT file is not needed
	} else {
		cc->reportAction("LNT file up to date: " + lntFileName,VERBOSE_GENERATION);
	}
	
	// If the LNT file needed (re)generation
	// or the BCG file needs (re)generation
	bool bcgGenerationOK = true;
	if(lntGenerationOK) {
		if(lntGenerationNeeded || bcgGenerationNeeded) {
			cc->reportAction("Generating: " + bcgFileName,VERBOSE_GENERATION);
			// Generate SVL
			generateSVLBuilder(svlOut,getFileForNode(node));
			if(bcgGenerationOK) {
				// call SVL
				//cc->reportFile(svlOut.toString());
				fancyFileWrite(svlFilePath,svlOut);
				executeSVL(lntRoot,svlFileName);
				
				// Check if the generation resulted in a valid BCGs file
				if(bcgIsValid(bcgFilePath)) {
					
					// Open and close .valid file, making sure it exists
					{
						std::ofstream bcgValidFile(bcgFilePath+".valid");
					}
					
					// Update the timestamp of the .valid file to the current time
					utime( (bcgFilePath+".valid").c_str(), NULL );
					
					// Report
					cc->reportSuccess("Generated: " + bcgFilePath,VERBOSE_GENERATION);
					
				} else {
					cc->reportErrorAt(node.getLocation(),"Could not generate BCG file `" + bcgFileName +  "' for node type `" + node.getTypeStr() + "'");
				}
			} else {
				cc->reportErrorAt(node.getLocation(),"Could not generate BCG file `" + bcgFileName +  "' for node type `" + node.getTypeStr() + "'");
			}
		// If regeneration of BCG file is not needed
		} else {
			cc->reportAction("BCG file up to date: " + bcgFileName,VERBOSE_GENERATION);
		}
	}
	
	return !(lntGenerationOK && bcgGenerationOK);
}

int DFT::DFTreeBCGNodeBuilder::generate() {
	set<std::string> triedToGenerate;
	std::vector<DFT::Nodes::Node*>::iterator it = dft->getNodes().begin();
	for(;it!=dft->getNodes().end(); it++) {
		int bad = generate(**it,triedToGenerate);
		if(bad) {
			std::vector<DFT::Nodes::Node*>::iterator it2 = dft->getNodes().begin();
			for(;it2!=dft->getNodes().end(); it2++) {
				if(getFileForNode(**it)==getFileForNode(**it2)) {
					cc->reportErrorAt((*it2)->getLocation(),"... for this node: `" + (*it2)->getName() + "'");
				}
			}
		}
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
	out << out.applyprefix << "\"" << bcgRoot << fileName << "." << DFT::FileExtensions::BCG << "\" = strong reduction of \"" << fileName << "." << DFT::FileExtensions::LOTOSNT << "\"" << out.applypostfix;
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
	std::string command("svl \"" + fileName + "\"");
	if(cc->getVerbosity()<VERBOSE_BCGISVALID) {
#ifdef WIN32
		command += " > NUL 2> NUL";
#else
		command += " > /dev/null 2> /dev/null";
#endif
	}
	PushD dir(root);
	int res = system( command.c_str() );
	dir.popd();
	return res==0;
}
