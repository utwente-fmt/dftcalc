class Node;

#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>

#include "dft_parser_location.h"

using namespace std;

namespace DFT {
namespace Nodes {
	
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
	GateOFType,
	GateFDEPType,
	GateTransferType,
	
	
	AnyType,
	NUMBEROF
};
	
class Node {
public:
	static const std::string BasicEventStr;
	static const std::string GateAndStr;
	static const std::string UnknownStr;
	static const std::string& getTypeName(NodeType type) {
		switch(type) {
		case BasicEventType:
			return BasicEventStr;
		case GateAndType:
			return GateAndStr;
		default:
			return UnknownStr;
		}
	}
	static bool typeMatch(NodeType type, NodeType matchType) {
		if(type == matchType) {
			return true;
		} else if(matchType==GateType) {
			return type==GateAndType;
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
	Node(std::string name, NodeType type):
		name(name),
		type(type) {
	}
	Node() {
	}
	virtual ~Node() {
	}
	const string& getName() {
		return name;
	}
	void setName(const string& name) {
		this->name = name;
	}
	virtual void addReferencesTo(std::vector<Node*>& nodeList) {
	}
	const Location& getLocation() { return location; }
	const NodeType& getType() const {return type;}
	
	virtual bool isBasicEvent() {
		return typeMatch(type,BasicEventType);
	}
	virtual bool isGate() {
		return typeMatch(type,GateType);
	}
};

class NodePlaceHolder: public Node {
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // NODE_H
