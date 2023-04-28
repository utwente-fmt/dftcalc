#include "DFTreeAUTNodeBuilder.h"
#include <fstream>
#include <stdexcept>
#include <memory>
#include <iostream>

#ifndef WIN32
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
static int try_sync(const std::string filename, CompilerContext *cc) {
	int fd = open(filename.c_str(), O_WRONLY);
	if (fd == -1) {
		char *errstr = strerror(errno);
		cc->reportError("Error opening " + filename + " to sync: " + errstr);
		return -1;
	}
	int ret = fsync(fd);
	if (ret) {
		char *errstr = strerror(errno);
		cc->reportError("Error syncing " + filename + ": " + errstr);
		close(fd);
		return ret;
	}
	ret = close(fd);
	if (ret) {
		char *errstr = strerror(errno);
		cc->reportError("Error closing " + filename + " to sync: " + errstr);
		return ret;
	}
	std::string dir;
	size_t dir_end = filename.rfind('/');
	if (dir_end == std::string::npos)
		dir = ".";
	else
		dir = filename.substr(0, dir_end);
#ifdef O_DIRECTORY
	fd = open(dir.c_str(), O_RDONLY | O_DIRECTORY);
#else
	fd = open(dir.c_str(), O_RDONLY);
#endif
	if (!fd) {
		char *errstr = strerror(errno);
		cc->reportError("Error opening dir " + dir + " to sync: " + errstr);
		return -1;
	}
	ret = fsync(fd);
	if (ret) {
		char *errstr = strerror(errno);
		cc->reportError("Error syncing dir " + dir + ": " + errstr);
		close(fd);
		return ret;
	}
	return close(fd);
}
#else /* unistd.h */
static int try_sync(const std::string &filename, CompilerContext *cc) {
	return 0;
}
#endif

namespace DFT {
const unsigned int DFTreeAUTNodeBuilder::VERSION = 2;

std::string DFTreeAUTNodeBuilder::getFileForNode(const DFT::Nodes::Node& node) {
	return getNodeName(node) + ".aut";
}

std::string DFT::DFTreeAUTNodeBuilder::getFileForTopLevel() {
	return "toplevel.aut";
}

static bool already_valid(std::string filename)
{
	std::string fullFileName = filename + ".version";
	try {
		std::ifstream in(fullFileName);
		unsigned int currentVersion = -1;
		in >> currentVersion;
		if (!in.good())
			return 0;
		return currentVersion >= DFTreeAUTNodeBuilder::VERSION;
	} catch (std::exception &e) {
		return 0;
	}
}

static void make_valid(std::string filename)
{
	std::string fullFileName = filename + ".version";
	std::ofstream out(fullFileName);
	out << DFTreeAUTNodeBuilder::VERSION;
}

static int generateTopLevel(std::string root, CompilerContext *cc) {
	std::string filename(root + "toplevel.aut");
	if (already_valid(filename))
		return 0;
	std::ofstream out(filename);
	out << "des (0, 1, 2)\n";
	out << "(0, \"" << automata::signals::ACTIVATE(0, true) << "\", 1)\n";
	out.close();
	if (out.fail()) {
		cc->reportError("Error writing " + filename);
		return -1;
	}
	if (try_sync(filename, cc))
		return -1;
	make_valid(filename);
	return 0;
}

int DFTreeAUTNodeBuilder::generate(const Nodes::Node &node) {
	std::string nodename = getNodeName(node);
	std::string filename = autRoot + getFileForNode(node);
	if (already_valid(filename))
		return 0;
	std::unique_ptr<automaton> to_output;
	switch (node.getType()) {
	case Nodes::BasicEventType: {
		const Nodes::BasicEvent& be = static_cast<const Nodes::BasicEvent&>(node);
		to_output = std::unique_ptr<automaton>(new automata::be(be));
		}
		break;
	case DFT::Nodes::GateOrType: {
		const Nodes::GateOr& gate = static_cast<const Nodes::GateOr&>(node);
		to_output = std::unique_ptr<automaton>(new automata::voting(gate));
		}
		break;
	case DFT::Nodes::GateAndType: {
		const Nodes::GateAnd& gate = static_cast<const Nodes::GateAnd&>(node);
		to_output = std::unique_ptr<automaton>(new automata::voting(gate));
		}
		break;
	case DFT::Nodes::GateVotingType: {
		const Nodes::GateVoting& gate = static_cast<const Nodes::GateVoting&>(node);
		to_output = std::unique_ptr<automaton>(new automata::voting(gate));
		}
		break;
	case DFT::Nodes::GatePAndType: {
		const Nodes::GatePAnd& gate = static_cast<const Nodes::GatePAnd&>(node);
		to_output = std::unique_ptr<automaton>(new automata::pand(gate));
		}
		break;
	case DFT::Nodes::GateWSPType: {
		const Nodes::GateWSP& gate = static_cast<const Nodes::GateWSP&>(node);
		to_output = std::unique_ptr<automaton>(new automata::spare(gate));
		}
                break;
	case DFT::Nodes::GateFDEPType: {
		const Nodes::GateFDEP& gate = static_cast<const Nodes::GateFDEP&>(node);
		to_output = std::unique_ptr<automaton>(new automata::fdep(gate));
		}
                break;
	case DFT::Nodes::InspectionType: {
                const Nodes::Inspection& gate = static_cast<const Nodes::Inspection&>(node);
		to_output = std::unique_ptr<automaton>(new automata::insp(gate));
		}
		break;
	default:
		return 1;
	}
	if (to_output) {
		std::ofstream out(filename);
		to_output->write(out);
		out.close();
		if (out.fail()) {
			cc->reportError("Error writing " + filename);
			return -1;
		}
		if (try_sync(filename, cc))
			return -1;
		make_valid(filename);
	}
	return 0;
}

int DFTreeAUTNodeBuilder::generate() {
	if (generateTopLevel(autRoot, cc)) {
		cc->reportError("Error generating Top Level AUT file");
		return 1;
	}
	std::vector<const Nodes::Node *> nodes = dft->getNodes();
	for (const Nodes::Node *node : nodes) {
		int bad = generate(*node);
		if(bad < 0) {
			cc->reportErrorAt(node->getLocation(),"Unable to create AUT file for this node: `" + node->getName() + "'");
		} else if (bad) {
			cc->reportWarningAt(node->getLocation(),"Unable to create AUT file for this node: `" + node->getName() + "'");
			return bad;
		}
	}
	return 0;
}

} /* Namespace DFT */
