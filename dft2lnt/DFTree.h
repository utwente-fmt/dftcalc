namespace DFT {
class DFTree;
}

#ifndef DFT_H
#define DFT_H

#include <algorithm>
#include <vector>
#include <map>
#include <iostream>
#include <assert.h>
#include "dftnodes/nodes.h"

class Parser;

namespace DFT {

/**
 * Dynamic Fault Tree. Contains all the data about a DFT.
 */
class DFTree {
private:

	// The nodes of the DFT
	std::vector<Nodes::Node*> nodes;
	
	// The mapping from name (e.g. "A") to a Node
	std::map<std::string,DFT::Nodes::Node*> nodeTable;
	
	// The Top (root) Node of the DFT
	Nodes::Node* topNode;

	/**
	 * Sets the Top node without any checks
	 */
	int setTopNode_(Nodes::Node* node) {
		this->topNode = node;
		return 0;
	}

public:
	DFTree(): nodes(0), topNode(NULL) {

	}
	virtual ~DFTree() {
		for(int i=nodes.size(); i--;) {
			assert(nodes.at(i));
			delete nodes.at(i);
		}
	}

	// Claims ownership
	/**
	 * Adds the specified node to this DFT.
	 * NOTE: it claims ownership of the Node; do not delete
	 * @param node The Node to add to the DFT.
	 */
	void addNode(Nodes::Node* node) {
		//std::cout << "Added node: " << node->getName() << ", type: " << node->getType() << std::endl;
		nodes.push_back(node);
	}

	/**
	 * Returns the Node associated with the specified name.
	 * Returns NULL if no such Node exists.
	 * @return The Node associated with the specified name.
	 */
	Nodes::Node* getNode(const std::string& name) {
		for(int i=0; i<nodes.size(); ++i) {
			if(nodes.at(i)->getName()==name) {
				return nodes.at(i);
			}
		}
		return NULL;
	}

	/**
	 * Returns the list of nodes.
	 * @return The list of nodes.
	 */
	std::vector<Nodes::Node*>& getNodes() {
		return nodes;
	}

	/**
	 * Sets the Top Node to the specified Node.
	 * The Node has to be already added to this DFT.
	 * @param The Top Node to set.
	 * @return false: success; true: error
	 */
	bool setTopNode(Nodes::Node* node) {
		// Check if the specified node is in this DFT
		for(int i=0; i<nodes.size(); ++i) {
			if(nodes.at(i)==node) {
				return setTopNode_(node);
			}
		}
		return true; // FIXME: ERROR HANDLING
	}
	
	/**
	 * Returns the Top Node.
	 * @return The Top Node.
	 */
	Nodes::Node* getTopNode() {
		return topNode;
	}
};
} // Namespace: DFT

#endif // DFT_H
