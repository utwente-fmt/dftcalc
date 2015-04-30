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

namespace DFT {
namespace Nodes {

class Gate: public Node {
private:
	std::vector<Node*> children;
	int repairableChildren;
	bool active;
	bool initialized = false;
// protected:
// 	Gate(Location loc, std::string name, DFT::Nodes::NodeType nodeType): Node(loc,name,nodeType) {
// 	}
public:
	/**
	 * Returns the list of children of this Gate.
	 * @return The list of children of this Gate.
	 */
	std::vector<Node*>& getChildren() { return children; }
	const std::vector<Node*>& getChildren() const { return const_cast<const std::vector<Node*>&>(children); }

	/**
	 * Returns the list of repairable children of this Gate.
	 * @return The list of repairable children of this Gate.
	 */
	const int getRepairableChildren() const{ return repairableChildren; }

	/**
	 * Adds the specified Node to this Node's list of children.
	 * @param child The Node to add to this Node's list of children.
	 */
	void addChild(Node* child) { children.push_back(child); }

    /**
     * Adds the specified Node to this Node's list of children.
     * @param child The Node to add to this Node's list of children.
     */
    void delChild(int n) { children.erase(children.begin()+n); }

	/**
	 * Adds the specified Node to this Node's list of repairable children.
	 * @param child The Node to add to this Node's list of repairable children.
	 */
	void setRepairableChildren(int nr) { repairableChildren=nr; }

	Gate(Location loc, std::string name, DFT::Nodes::NodeType nodeType): Node(loc,name,nodeType) {
	}
	virtual ~Gate() {
	}
	virtual void addReferencesTo(std::vector<Node*>& nodeList) {
		for(size_t i=0; i<children.size(); ++i) {
			nodeList.push_back(children.at(i));
		}
	}
	virtual bool isBasicEvent() const { return false; }
	virtual bool isGate() const { return true; }
	/**
	* Determines whether or not smart semantics are applicabel for the Gate
	*/
	void setActive(){
		if(!initialized){
			active= !this->isRepairable();
			for(size_t n = 0; n<this->getParents().size() &&active; ++n) {
				DFT::Nodes::Gate* parent = static_cast<DFT::Nodes::Gate*>(this->getParents().at(n));
				parent->isActive();
				active=parent->isActive();
			}
			for(size_t n=0; n < this->getChildren().size() && active; ++n){
				DFT::Nodes::Node* child = (this->getChildren().at(n));
				active = !child->isRepairable();
			}
			initialized=true;
		}
	}
	/**
   * Returns whether or not this Gate is marked as being active.
   * @return whether or not this Gate is marked as being active.
   */
	bool isActive() const{
		return active;
	}

	/**
   * Returns whether or not this Gate is a the spare type.
   * @return whether or not this Gate is a spare type.
   */
	bool isSpare() const{
		return typeMatch(this->getType(), DFT::Nodes::GateWSPType) ||typeMatch(this->getType(), DFT::Nodes::GateCSPType) || typeMatch(this->getType(), DFT::Nodes::GateHSPType)  ;
	}

	/**
   * Marks this Basic Event as not active. This means smart semantics cannot be
   * applied to this Basic Event
   */
	void setNotActive(){
		active=false;
		initialized = true;
	}
};

} // Namespace: Node
} // Namespace: DFT

#endif // GATE_H
