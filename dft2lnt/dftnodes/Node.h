/*
 * Node.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 * @modified by Dennis Guck
 */

class Node;

#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>

#include "dft_parser_location.h"

using namespace std;

namespace DFT {
namespace Nodes {

/**
 * The supported DFT Node types
 */
enum NodeType {
	BasicEventType=1,

	GatePhasedOrType,
	GateOrType,
	GateAndType,
    GateSAndType,
	GateHSPType,
	GateWSPType,
	GateCSPType,
	GateSpareType,
	GatePAndType,
	GateSeqType,
    GatePorType,
	GateVotingType,
	GateFDEPType,
	GateTransferType,

	RepairUnitType,
	RepairUnitFcfsType,
	RepairUnitPrioType,
	RepairUnitNdType,

    InspectionType,
    ReplacementType,
    
	GateType,
	GATES_FIRST = GatePhasedOrType,
	GATES_LAST  = ReplacementType,
	
	AnyType,
	NUMBEROF
};

/**
 * DFT Node superclass
 */
class Node {
public:
	static const std::string BasicEventStr;
	static const std::string GateAndStr;
    static const std::string GateSAndStr;
	static const std::string GateOrStr;
	static const std::string GateWSPStr;
	static const std::string GatePAndStr;
    static const std::string GatePorStr;
	static const std::string GateVotingStr;
	static const std::string GateFDEPStr;
	static const std::string UnknownStr;
	static const std::string RepairUnitStr;
	static const std::string RepairUnitFcfsStr;
	static const std::string RepairUnitPrioStr;
	static const std::string RepairUnitNdStr;
    static const std::string InspectionStr;
    static const std::string ReplacementStr;
	
	/**
	 * Returns the textual representation of the specified NodeType.
	 * @return The textual representation of the specified NodeType.
	 */
	static const std::string& getTypeName(NodeType type) {
		switch(type) {
		case BasicEventType:
			return BasicEventStr;
		case GateAndType:
			return GateAndStr;
        case GateSAndType:
            return GateSAndStr;
		case GateOrType:
			return GateOrStr;
		case GateWSPType:
			return GateWSPStr;
		case GatePAndType:
			return GatePAndStr;
        case GatePorType:
            return GatePorStr;
		case GateVotingType:
			return GateVotingStr;
		case GateFDEPType:
			return GateFDEPStr;
		case RepairUnitType:
			return RepairUnitStr;
		case RepairUnitFcfsType:
			return RepairUnitFcfsStr;
		case RepairUnitPrioType:
			return RepairUnitPrioStr;
		case RepairUnitNdType:
			return RepairUnitNdStr;
        case InspectionType:
            return InspectionStr;
        case ReplacementType:
            return ReplacementStr;
		default:
			return UnknownStr;
		}
	}
	
	/**
	 * Returns whether type matches matchType. The order matters, e.g.:
	 *   - typeMatch(GateAndType,GateType) == true
	 *   - typeMatch(GateType,GateAndType) == false
	 */
	static bool typeMatch(NodeType type, NodeType matchType) {
		if(type == matchType) {
			return true;
		} else if(matchType==GateType) {
		//	return type==GateAndType;
			return GATES_FIRST <= type && type <= GATES_LAST;
		} else if (matchType == GateSpareType) {
			return type == GateWSPType || type == GateCSPType || type == GateHSPType;
		} else {
			return false;
		}
	}
private:
	Location location;
	string name;
	NodeType type;
	bool repairable;
	bool alwaysActive;
	
	/// List of parents, instances are freed by DFTree instance.
	std::vector<Nodes::Node*> parents;
public:
	Node(Location location, NodeType type):
		location(location),
		type(type),
		repairable(false){
	}
	Node(Location location, std::string name, NodeType type):
		location(location),
		name(name),
		type(type),
		repairable(false){
	}
	Node(Location location, std::string name, NodeType type, bool repairable):
		location(location),
		name(name),
		type(type),
		repairable(repairable){
	}
	Node() {
	}
	virtual ~Node() {
	}
	
	/**
	 * Returns the name of this Node.
	 * @return The name of this Node.
	 */
	const string& getName() const {
		return name;
	}
	
	/**
	 * Sets the name of this Node
	 * @param The name of this Node to be set.
	 */
	void setName(const string& name) {
		this->name = name;
	}
	

	/**
	 * Adds all the Nodes that this Node references to the specified list.
	 * @param nodeList The list to which the references are added.
	 */
	virtual void addReferencesTo(std::vector<Node*>& nodeList) {
	}
	
	/**
	 * Returns the location in the source where this node was defined.
	 * @return The location in the source where this node was defined.
	 */
	const Location& getLocation() const { return location; }
	
	/**
	 * Returns the type of this Node.
	 * @return The type of this Node.
	 */
	virtual const NodeType& getType() const {return type;}
	
	void setParents(std::vector<Nodes::Node*> parents) { this->parents = parents;}
	std::vector<Node*>& getParents() {return parents;}
	const std::vector<Node*>& getParents() const {return parents;}
	
	/**
	 * Returns the type of this Node.
	 * @return The type of this Node.
	 */
	virtual const std::string& getTypeStr() const { return getTypeName(getType()); }
	
	void setRepairable(bool repair) { repairable=repair; }
	const void setRepairable(bool repair) const { setRepairable(repairable); }

	void setAlwaysActive(bool isAlwaysActive) { alwaysActive = isAlwaysActive; }
	const void setAlwaysActive(bool AA) const { setAlwaysActive(AA); }

	/**
	 * returns if gate is repairable
	 * @return True if repairable, otherwise false
	 */
	virtual bool isRepairable() const {
		return repairable;
	}

	/**
	 * returns if gate repairs (any of) its children.
	 */
	virtual bool repairsChildren() const {
		return false;
	}

	/**
	 * Set children's information that they are repaired.
	 */
	virtual void setChildRepairs() const {
	}

	/**
	 * returns if gate is always active.
	 */
	virtual bool isAlwaysActive() const {
		return alwaysActive;
	}

	/**
	 * Returns whether this Node is a BasicEvent, i.e. typeMatch(type,BasicEventType).
	 * @return true: this node is a BasicEvent, false otherwise
	 */
	virtual bool isBasicEvent() const {
		return typeMatch(type,BasicEventType);
	}
	
	/**
	 * Returns whether this Node is a Gate, i.e. typeMatch(type,GateType).
	 * @return true: this node is a Gate, false otherwise
	 */
	virtual bool isGate() const {
		return typeMatch(type,GateType);
	}
	
	/**
	 * Returns whether or not this node uses dynamic activation.
	 * Dynamic activation means the node listens to when one of its children
	 * is activated by a different node.
	 * @return true/false: whether this node uses dynamic activation.
	 */
	virtual bool usesDynamicActivation() const { return false; }
	
	
	/**
	 * Returns whether or not this node has a dummy output.
	 * When a node has a dummy output, it means the node never triggers the fail
	 * event to its parent.
	 * @return true/false: whether this node has a dummy output.
	 */
	virtual bool outputIsDumb() const { return false; }

	bool matchesType(NodeType otherType) const {return typeMatch(type, otherType);}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // NODE_H
