/*
 * Node.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 * @modified by Dennis Guck
 */

#include "Node.h"
#include "Gate.h"
#include <string>
#include <unordered_set>

namespace DFT {
namespace Nodes {
	
	const std::string Node::BasicEventStr("be");
	const std::string Node::GateAndStr("and");
    const std::string Node::GateSAndStr("sand");
	const std::string Node::GateOrStr("or");
	const std::string Node::GateWSPStr("wsp");
	const std::string Node::GatePAndStr("pand");
    const std::string Node::GatePorStr("por");
	const std::string Node::GateVotingStr("voting");
	const std::string Node::GateFDEPStr("fdep");
	const std::string Node::UnknownStr("XXX");
	const std::string Node::RepairUnitStr("ru");
	const std::string Node::RepairUnitFcfsStr("ru_f");
	const std::string Node::RepairUnitPrioStr("ru_p");
	const std::string Node::RepairUnitNdStr("ru_nd");
    const std::string Node::InspectionStr("inspection");
    const std::string Node::ReplacementStr("replacement");

	bool Node::hasInspectionModule(void) const {
		for (Gate *g : parents) {
			Node *n = (Node *)g;
			if (n->matchesType(Nodes::InspectionType))
				return true;
		}
		return false;
	}

	bool Node::hasRepairModule(void) const {
		for (Gate *g : parents) {
			Node *n = (Node *)g;
			if (n->matchesType(Nodes::RepairUnitAnyType))
				return true;
		}
		return false;
	}

	bool Node::isIndependentSubtree(void) const {
		if (parents.size() > 1)
			return false;
		if (parents.size() == 0)
			return true;
		std::unordered_set<const Node *> to_explore, subtree;
		to_explore.insert(this);
		while (to_explore.size() != 0) {
			const Node *current = *to_explore.begin();
			to_explore.erase(current);
			subtree.insert(current);
			if (current->isGate()) {
				const Gate *g = static_cast<const Gate *>(current);
				for (Node *c : g->getChildren()) {
					if (subtree.find(c) == subtree.end())
						to_explore.insert(c);
				}
			}
			if (current == this)
				continue;
			for (Node *par : current->parents) {
				if (subtree.find(par) == subtree.end())
					to_explore.insert(par);
			}
		}
		Node *parent = static_cast<Node *>(parents[0]);
		return subtree.find(parent) == subtree.end();
	}
}
}
