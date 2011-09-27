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
	
	AnyType,
	NUMBEROF
};
	
class Node {
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
};

class NodePlaceHolder: public Node {
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // NODE_H
