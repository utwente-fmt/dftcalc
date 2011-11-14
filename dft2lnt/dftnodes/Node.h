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
	BasicEventType,

	GateType,
	GatePhasedOrType,
	GateOrType,
	GateAndType,
	GateHSPType,
	GateWSPType,
	GateCSPType,
	GatePAndType,
	GateSeqType,
	GateVotingType,
	GateFDEPType,
	GateTransferType,

	GATES_FIRST = GatePhasedOrType,
	GATES_LAST  = GateTransferType,
	
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
	static const std::string GateOrStr;
	static const std::string GateWSPStr;
	static const std::string GatePAndStr;
	static const std::string UnknownStr;
	
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
		case GateOrType:
			return GateOrStr;
		case GateWSPType:
			return GateWSPStr;
		case GatePAndType:
			return GatePAndStr;
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
		} else {
			return false;
		}
	}
private:
	Location location;
	string name;
	NodeType type;
public:
	Node(Location location, NodeType type):
		location(location),
		type(type) {
	}
	Node(Location location, std::string name, NodeType type):
		location(location),
		name(name),
		type(type) {
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
	const NodeType& getType() const {return type;}
	
	/**
	 * Returns the type of this Node.
	 * @return The type of this Node.
	 */
	const std::string& getTypeStr() const { return getTypeName(getType()); }
	
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
	
	virtual bool usesDynamicActivation() const { return false; }
};

class NodePlaceHolder: public Node {
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // NODE_H
