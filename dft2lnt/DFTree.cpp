/*
 * DFTree.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include "DFTree.h"
#include "dft_parser.h"

namespace DFT {
	/* This should be replaced by a proper check to allow replacing of
	 * entire subtrees rather than just basic events, but at least we
	 * know for certain that basic events are independent.
	 */
	static bool isIndependentSubtree(Nodes::Node *node) {
		if (node->getParents().size() > 1)
			return false;
		return node->isBasicEvent();
	}

	static void setAllDormsToZero(Nodes::Node *node) {
		if (node->matchesType(Nodes::GateType)) {
			Nodes::Gate *gate = static_cast<DFT::Nodes::Gate *>(node);
			for (Nodes::Node *child : gate->getChildren())
				setAllDormsToZero(child);
		} else if (node->isBasicEvent()) {
			Nodes::BasicEvent* be = static_cast<Nodes::BasicEvent*>(node);
			be->setMu(0);
		}
	}

	void DFTree::replaceSEQs() {
		for (Nodes::Node *node : nodes) {
			if (!node->matchesType(Nodes::GateSeqType))
				continue;
			Nodes::GateSeq *seq = static_cast<DFT::Nodes::GateSeq *>(node);
			bool canReplace = true;
			for (Nodes::Node *child : seq->getChildren()) {
				if (!isIndependentSubtree(child)) {
					canReplace = false;
					break;
				}
			}
			if (canReplace) {
				setAllDormsToZero(seq);
				// Create a new SAND node
				Nodes::GateSAnd *sand = new Nodes::GateSAnd(seq->getLocation(),
				                                            seq->getName());
				// Add it to the DFT
				addNode(sand);
				// Add all children to the SAND.
				for (Nodes::Node *child : seq->getChildren()) {
					vector<Nodes::Gate *> &pars = child->getParents();
					for (int i = 0; i < pars.size(); i++) {
						if (pars[i] == seq)
							pars[i] = sand;
					}
					sand->addChild(child);
				}
				
				// Point all the SEQ's parents to the SAND
				for(Nodes::Gate* parent: seq->getParents()) {
					// Add the parent to the OR-node's parents
					seq->getParents().push_back(parent);
					vector<Nodes::Node *> &pcs = parent->getChildren();
					for (int i = 0; i < pcs.size(); i++) {
						if (pcs[i] == seq)
							pcs[i] = sand;
					}
				}
				// Remove the original SEQ node
				removeNode(seq);

				// Replace the top node if necessary.
				if (getTopNode() == seq)
					setTopNode(sand);
			}
		}
		return;
	}

	void DFTree::addSimulRepairs() {
		for (Nodes::Node *node : nodes) {
			if (!node->matchesType(Nodes::BasicEventType))
				continue;
			if (node->hasRepairModule())
				continue;
			if (!node->isRepairable())
				continue;
			Nodes::RepairUnit *ru = new Nodes::RepairUnit(node->getLocation(),
				                                          "r_"+node->getName(),
			                                        Nodes::RepairUnitSimulType);

			ru->addChild(node);
			node->getParents().push_back(ru);
			addNode(ru);
		}
		return;
	}
}
