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
class DFTree {
private:
	std::vector<Nodes::Node*> nodes;
	std::map<std::string,DFT::Nodes::Node*> nodeTable;
	Nodes::Node* topNode;

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
	void addNode(Nodes::Node* node) {
		//std::cout << "Added node: " << node->getName() << ", type: " << node->getType() << std::endl;
		nodes.push_back(node);
	}

	Nodes::Node* getNode(const std::string& name) {
		for(int i=0; i<nodes.size(); ++i) {
			if(nodes.at(i)->getName()==name) {
				return nodes.at(i);
			}
		}
		return NULL;
	}

	std::vector<Nodes::Node*>& getNodes() {
		return nodes;
	}

	bool setTopNode(Nodes::Node* node) {
		// Check if the specified node is in this DFT
		for(int i=0; i<nodes.size(); ++i) {
			if(nodes.at(i)==node) {
				return setTopNode_(node);
			}
		}
		return true; // FIXME: ERROR HANDLING
	}
	Nodes::Node* getTopNode() {
		return topNode;
	}
};
} // Namespace: DFT

#endif // DFT_H
