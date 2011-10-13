#ifndef GATE_H
#define GATE_H

#include <vector>
#include "Node.h"

using namespace std;

namespace DFT {
namespace Nodes {

class Gate: public Node {
private:
	std::vector<Node*> children;
protected:
	Gate(std::string name, DFT::Nodes::NodeType nodeType): Node(name,nodeType) {
	}
public:

	/**
	 * Returns the list of children of this Gate.
	 * @return The list of children of this Gate.
	 */
	std::vector<Node*>& getChildren() { return children; }
	
	/**
	 * Adds the specified Node to this Node's list of children.
	 * @param child The Node to add to this Node's list of children.
	 */
	void addChild(Node* child) { children.push_back(child); }
	
	virtual ~Gate() {
	}
	virtual void addReferencesTo(std::vector<Node*>& nodeList) {
		for(int i=0; i<children.size(); ++i) {
			nodeList.push_back(children.at(i));
		}
	}
};

} // Namespace: Node
} // Namespace: DFT

#endif // GATE_H
