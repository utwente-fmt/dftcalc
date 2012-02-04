/*
 * Gate.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

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
	Gate(Location loc, std::string name, DFT::Nodes::NodeType nodeType): Node(loc,name,nodeType) {
	}
public:

	/**
	 * Returns the list of children of this Gate.
	 * @return The list of children of this Gate.
	 */
	std::vector<Node*>& getChildren() { return children; }
	const std::vector<Node*>& getChildren() const { return const_cast<const std::vector<Node*>&>(children); }
	
	/**
	 * Adds the specified Node to this Node's list of children.
	 * @param child The Node to add to this Node's list of children.
	 */
	void addChild(Node* child) { children.push_back(child); }
	
	virtual ~Gate() {
	}
	virtual void addReferencesTo(std::vector<Node*>& nodeList) {
		for(size_t i=0; i<children.size(); ++i) {
			nodeList.push_back(children.at(i));
		}
	}
	virtual bool isBasicEvent() const { return false; }
	virtual bool isGate() const { return true; }
};

} // Namespace: Node
} // Namespace: DFT

#endif // GATE_H
