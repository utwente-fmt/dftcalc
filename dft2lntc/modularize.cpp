#include "modularize.h"
#include "dftnodes/nodes.h"
#include <ostream>
#include <fstream>
#include <vector>

static void writeModules(std::ostream &out, DFT::DFTree *dft, DFT::Nodes::Node *root)
{
	if (root->matchesType(DFT::Nodes::NodeType::BasicEventType)) {
		DFT::Nodes::BasicEvent *be;
		be = static_cast<DFT::Nodes::BasicEvent *>(root);
		if (be->getLambda() == -1) {
			out << "=" << be->getProb().str() << "\n";
			return;
		}
	}
	if (!(root->matchesType(DFT::Nodes::NodeType::GateAndType)
	      || root->matchesType(DFT::Nodes::NodeType::GateOrType)
	      || root->matchesType(DFT::Nodes::NodeType::GateVotingType)))
	{
		out << "M" << root->getName() << "\n";
		return;
	}
	DFT::Nodes::Gate *g = static_cast<DFT::Nodes::Gate *>(root);
	std::vector<DFT::Nodes::Node *> children = g->getChildren();
	for (DFT::Nodes::Node *child : children) {
		if (!child->isIndependentSubtree()) {
			out << "M" << root->getName() << "\n";
			return;
		}
	}
	if (root->matchesType(DFT::Nodes::NodeType::GateAndType))
		out << "*" << children.size() << "\n";
	else if (root->matchesType(DFT::Nodes::NodeType::GateOrType))
		out << "+" << children.size() << "\n";
	else {
		DFT::Nodes::GateVoting *v;
		v = static_cast<DFT::Nodes::GateVoting *>(root);
		out << "/" << v->getThreshold() <<" "<< children.size() << "\n";
	}
	for (DFT::Nodes::Node *child : children)
		writeModules(out, dft, child);
}

void writeModules(std::string filename, DFT::DFTree *dft)
{
	DFT::Nodes::Node *root = dft->getTopNode();
	if (filename.empty())
		writeModules(std::cout, dft, root);
	else {
		ofstream out(filename, std::ofstream::out);
		writeModules(out, dft, root);
		out.close();
	}
}
