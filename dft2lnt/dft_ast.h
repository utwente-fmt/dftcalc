
namespace DFT {
namespace AST {
class ASTIdentifier;
class ASTAttributeLabel;
class ASTAttrib;
class ASTAttribute;
}
}

#ifndef DFT_AST_H
#define DFT_AST_H

#include "dftnodes/nodes.h"

namespace DFT {
namespace AST {

enum NodeType {
    TopLevelType = 0,
    BasicEventType,
    GateType,
    PageType,
    IdentifierType,
    BEAttributeFloatType,
    BEAttributeNumberType,
    BEAttributeStringType,
    AttributeLabelType,

    AnyType,
    NUMBEROF
};

class ASTNode {
private:
	DFT::AST::NodeType type;
protected:
	const Location location;
public:
	ASTNode(DFT::AST::NodeType type,Location location):
		type(type),
		location(location) {
	}
	DFT::AST::NodeType getType() {
		return type;
	}
	const Location& getLocation() const {
		return location;
	}
};

//template<NodeType nodeType>
//class ASTNode: public ASTNode {
//public:
//	ASTNode(): ASTNode(nodeType) {};
//};

class ASTTopLevel: public ASTNode {
private:
	ASTIdentifier* topNode;
public:
	ASTTopLevel(Location location, ASTIdentifier* topNode):
		ASTNode(TopLevelType,location),
		topNode(topNode) {
	}
	void setTopNode(ASTIdentifier* topNode) {
		this->topNode = topNode;
	}
	ASTIdentifier* getTopNode() const {
		return topNode;
	}
};

//class ASTEntity: public ASTNode {
//public:
//	const std::string& getName() const = 0;
//};

class ASTAttrib: public ASTNode {
public:
	ASTAttrib(NodeType type, Location location):
		ASTNode(type,location) {
	}
};

class ASTAttribFloat: public ASTAttrib {
private:
	float value;
public:
	ASTAttribFloat(Location location, float value):
		ASTAttrib(BEAttributeFloatType,location),
		value(value) {

	}
	float getValue() {
		return value;
	}
	void setValue(float value) {
		this->value = value;
	}
};

class ASTAttribNumber: public ASTAttrib {
private:
	int value;
public:
	ASTAttribNumber(Location location, int value):
		ASTAttrib(BEAttributeNumberType,location),
		value(value) {

	}
	int getValue() {
		return value;
	}
	void setValue(int value) {
		this->value = value;
	}
};

class ASTAttribString: public ASTAttrib {
private:
	ASTIdentifier* value;
public:
	ASTAttribString(Location location, ASTIdentifier* value):
		ASTAttrib(BEAttributeStringType,location),
		value(value) {

	}
	ASTIdentifier* getValue() {
		return value;
	}
	void setValue(ASTIdentifier* value) {
		this->value = value;
	}
};

class ASTIdentifier: public ASTNode {
private:
	std::string str;
public:
	ASTIdentifier(NodeType type, Location location, std::string str):
		ASTNode(type,location),
		str(str) {
	}
	ASTIdentifier(Location location, std::string str):
		ASTNode(IdentifierType,location),
		str(str) {
	}
	void setString(const std::string& str) {
		this->str = str;
	}
	const std::string& getString() const {
		return str;
	}
};

class ASTAttribute: public ASTIdentifier {
private:
	DFT::Nodes::BE::AttributeLabelType label;
	ASTAttrib* value;
public:
	ASTAttribute(Location location, std::string str, DFT::Nodes::BE::AttributeLabelType label):
		ASTIdentifier(AttributeLabelType,location,str),
		label(label) {
	}
	const DFT::Nodes::BE::AttributeLabelType& getLabel() const {
		return label;
	}
	void setValue(ASTAttrib* value) {
		this->value = value;
	}
	ASTAttrib* getValue() {
		return value;
	}
};

class ASTBasicEvent: public ASTNode {
private:
	ASTIdentifier* name;
	std::vector<DFT::AST::ASTAttribute*>* attributes;
public:
	ASTBasicEvent(Location location, ASTIdentifier* name):
		ASTNode(BasicEventType,location),
		name(name),
		attributes(NULL) {
	}
	ASTBasicEvent(Location location, ASTIdentifier* name, std::vector<DFT::AST::ASTAttribute*>* attributes):
		ASTNode(BasicEventType,location),
		name(name),
		attributes(attributes) {
	}
	void setName(ASTIdentifier* name) {
		this->name = name;
	}
	ASTIdentifier* getName() const {
		return name;
	}
	void setAttributes(std::vector<DFT::AST::ASTAttribute*>* attributes) {
		if(attributes) {
			for(int i=attributes->size(); i--;) {
				delete attributes->at(i);
			}
			delete attributes;
		}
		this->attributes = attributes;
	}
	void setPhase(int phase) {

	}
};

class ASTGate: public ASTNode {
private:
	ASTIdentifier* name;
	ASTIdentifier* gateType;
	std::vector<ASTIdentifier*>* children;
public:
	ASTGate(Location location, ASTIdentifier* name, ASTIdentifier* gateType, std::vector<ASTIdentifier*>* children):
		ASTNode(GateType,location),
		name(name),
		gateType(gateType),
		children(children) {
	}
	virtual ~ASTGate() {
	}
	void setName(ASTIdentifier* name) {
		this->name = name;
	}
	ASTIdentifier* getName() const {
		return name;
	}
	std::vector<ASTIdentifier*>* getChildren() {
		return children;
	}
};

class ASTPage: public ASTNode {
private:
	int page;
	ASTIdentifier* nodeName;
public:
	ASTPage(Location location, int page, ASTIdentifier* nodeName):
		ASTNode(PageType,location),
		page(page),
		nodeName(nodeName) {
	}
	virtual ~ASTPage() {
	}
	void setNodeName(ASTIdentifier* nodeName) {
		this->nodeName = nodeName;
	}
	ASTIdentifier* getNodeName() const {
		return nodeName;
	}
};

} // Namespace: DFT
} // Namespace: AST

#endif // DFT_AST_H
