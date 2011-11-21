
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

/**
 * Describes the various ASTNode types.
 */
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
    ASTGateTypeType,
    ASTVotingGateTypeType,

    AnyType,
    NUMBEROF
};

/**
 * Ancestor ASTNode. Contains code for location tracking.
 */
class ASTNode {
private:
	DFT::AST::NodeType type;
protected:
	const Location location;
public:

	/**
	 * Constructs a new ASTNode at the specifed source location.
	 * The type will be used for later identification (like RTTI)
	 */
	ASTNode(DFT::AST::NodeType type,Location location):
		type(type),
		location(location) {
	}
	
	/**
	 * Returns the type of this ASTNode.
	 * @return The type of this ASTNode.
	 */
	DFT::AST::NodeType getType() {
		return type;
	}
	
	/**
	 * Returns the location in the code of this ASTNode.
	 * @eturn The location in the code of this ASTNode.
	 */
	const Location& getLocation() const {
		return location;
	}
};

//template<NodeType nodeType>
//class ASTNode: public ASTNode {
//public:
//	ASTNode(): ASTNode(nodeType) {};
//};

/**
 * TopLevel ASTNode.
 * Specifies what Node is the Top node of the DFT
 */
class ASTTopLevel: public ASTNode {
private:
	ASTIdentifier* topNode;
	public:

	/**
	 * Constructs a new ASTTopLevel sourced from the specified location.
	 * @param topNode The node that is to be the Top node in the DFT.
	 */
	ASTTopLevel(Location location, ASTIdentifier* topNode):
		ASTNode(TopLevelType,location),
		topNode(topNode) {
	}
	
	/**
	 * Sets the Top node value.
	 * @param topNode The Top node value to be set.
	 */
	void setTopNode(ASTIdentifier* topNode) {
		this->topNode = topNode;
	}
	
	/**
	 * Returns the Top node value.
	 * @return The Top node value.
	 */
	ASTIdentifier* getTopNode() const {
		return topNode;
	}
};

//class ASTEntity: public ASTNode {
//public:
//	const std::string& getName() const = 0;
//};

/**
 * DFT Node attribute value ASTNode
 */
class ASTAttrib: public ASTNode {
public:
	ASTAttrib(NodeType type, Location location):
		ASTNode(type,location) {
	}
	virtual bool isFloat()  { return getType() == BEAttributeFloatType;  }
	virtual bool isString() { return getType() == BEAttributeStringType; }
	virtual bool isNumber() { return getType() == BEAttributeNumberType; }
};

/**
 * DFT Node float attribute.
 */
class ASTAttribFloat: public ASTAttrib {
private:
	float value;
public:
	ASTAttribFloat(Location location, float value):
		ASTAttrib(BEAttributeFloatType,location),
		value(value) {

	}
	/**
	 * Returns the value of this attribute.
	 * @return The value of this attribute.
	 */
	float getValue() {
		return value;
	}

	/**
	 * Sets the value of this attribute.
	 * @param value The value to be set.
	 */
	void setValue(float value) {
		this->value = value;
	}
	
	virtual bool isFloat()  { return true;  }
	virtual bool isString() { return false; }
	virtual bool isNumber() { return false; }
};

/**
 * DFT Node number  attribute.
 */
class ASTAttribNumber: public ASTAttrib {
private:
	int value;
public:
	ASTAttribNumber(Location location, int value):
		ASTAttrib(BEAttributeNumberType,location),
		value(value) {

	}

	/**
	 * Returns the value of this attribute.
	 * @return The value of this attribute.
	 */
	int getValue() {
		return value;
	}

	/**
	 * Sets the value of this attribute.
	 * @param value The value to be set.
	 */
	void setValue(int value) {
		this->value = value;
	}

	virtual bool isFloat()  { return false;  }
	virtual bool isNumber() { return true; }
	virtual bool isString() { return false;  }
};

/**
 * DFT Node string attribute.
 */
class ASTAttribString: public ASTAttrib {
private:
	ASTIdentifier* value;
public:
	ASTAttribString(Location location, ASTIdentifier* value):
		ASTAttrib(BEAttributeStringType,location),
		value(value) {

	}

	/**
	 * Returns the value of this attribute.
	 * @return The value of this attribute.
	 */
	ASTIdentifier* getValue() {
		return value;
	}
	
	/**
	 * Sets the value of this attribute.
	 * @param value The value to be set.
	 */
	void setValue(ASTIdentifier* value) {
		this->value = value;
	}

	virtual bool isFloat()  { return false;  }
	virtual bool isNumber() { return false;  }
	virtual bool isString() { return true;  }
};

/**
 * Identifier ASTNode
 */
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
	
	/**
	 * Sets the string value.
	 * @param str The string value to be set.
	 */
	void setString(const std::string& str) {
		this->str = str;
	}
	
	/**
	 * Returns the string value.
	 * @return The string value.
	 */
	const std::string& getString() const {
		return str;
	}
};

/**
 * DFT node attribute ASTNode
 */
class ASTAttribute: public ASTIdentifier {
private:
	DFT::Nodes::BE::AttributeLabelType label;
	ASTAttrib* value;
public:
	ASTAttribute(Location location, std::string str, DFT::Nodes::BE::AttributeLabelType label):
		ASTIdentifier(AttributeLabelType,location,str),
		label(label) {
	}
	
	/**
	 * Returns the attribute label of this attribute.
	 * @return The attribute label of this attribute.
	 */
	const DFT::Nodes::BE::AttributeLabelType& getLabel() const {
		return label;
	}
	
	/**
	 * Sets the attribute value of this attribute.
	 * @param value The attribute value to be set.
	 */
	void setValue(ASTAttrib* value) {
		this->value = value;
	}
	
	/**
	 * Returns the value of this attribute.
	 * @return The value of this attribute.
	 */
	ASTAttrib* getValue() {
		return value;
	}
};

/**
 * Gate type ASTNode
 */
class ASTGateType: public ASTIdentifier {
private:
	DFT::Nodes::NodeType nodeType;
protected:
	ASTGateType(NodeType type, Location location, std::string str, DFT::Nodes::NodeType nodeType):
		ASTIdentifier(type,location,str),
		nodeType(nodeType) {
	}
	
public:
	ASTGateType(Location location, std::string str, DFT::Nodes::NodeType nodeType):
		ASTIdentifier(ASTGateTypeType,location,str),
		nodeType(nodeType) {
	}
	
	/**
	 * Returns the type of the Gate.
	 * @return The type of the Gate.
	 */
	const DFT::Nodes::NodeType& getGateType() const {
		return nodeType;
	}
};

/**
 * The Voting GateType is the only rule that has node specific date in the
 * gate type identifier
 */
class ASTVotingGateType: public ASTGateType {
private:
	int threshold; // threshold
	int total; // total
	DFT::Nodes::NodeType nodeType;
public:
	ASTVotingGateType(Location location, int threshold, int total):
		ASTGateType(ASTVotingGateTypeType,location,"voting",DFT::Nodes::GateVotingType),
		threshold(threshold),
		total(total) {
	}
	
	int getThreshold() const {return threshold;}
	int getTotal() const {return total;}
};

/**
 * BasicEvent ASTNode
 */
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
	
	/**
	 * Sets the name of the BasicEvent.
	 * @param name The name of the BasicEvent to be set.
	 */
	void setName(ASTIdentifier* name) {
		this->name = name;
	}
	
	/**
	 * Returns the name of the BasicEvent.
	 * @return The name of the BasicEvent.
	 */
	ASTIdentifier* getName() const {
		return name;
	}
	
	/**
	 * Sets the list of attributes of this BasicEvent. The old list will be
	 * deleted first.
	 * @param attributes The new list of attributes.
	 */
	void setAttributes(std::vector<DFT::AST::ASTAttribute*>* attributes) {
		if(this->attributes) {
			for(int i=this->attributes->size(); i--;) {
				delete this->attributes->at(i);
			}
			delete this->attributes;
		}
		this->attributes = attributes;
	}
	
	std::vector<DFT::AST::ASTAttribute*>* getAttributes() {
		return attributes;
	}
	
	void setPhase(int phase) {

	}
};

class ASTGate: public ASTNode {
private:
	ASTIdentifier* name;
	ASTGateType* gateType;
	std::vector<ASTIdentifier*>* children;
public:
	ASTGate(Location location, ASTIdentifier* name, ASTGateType* gateType, std::vector<ASTIdentifier*>* children):
		ASTNode(GateType,location),
		name(name),
		gateType(gateType),
		children(children) {
	}
	virtual ~ASTGate() {
	}
	
	/**
	 * Sets the name of this ASTGate.
	 * @param name The name to be set.
	 */
	void setName(ASTIdentifier* name) {
		this->name = name;
	}
	
	/**
	 * Returns the name of this ASTGate.
	 * @return The name of this ASTGate.
	 */
	ASTIdentifier* getName() const {
		return name;
	}
	
	/**
	 * Return the type of the Gate described by this ASTGate.
	 * @return The type of the Gate described by this ASTGate.
	 */
	ASTGateType* getGateType() {
		return gateType;
	}
	
	/**
	 * Returns the outgoing references of this ASTGate.
	 * @return The outgoing references of this ASTGate.
	 */
	std::vector<ASTIdentifier*>* getChildren() {
		return children;
	}
};

/**
 * Page ASTNode
 * This is for the graphical display of Galileo.
 */
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
	
	/**
	 * Sets the DFT node that will be cast to a specific page.
	 * @param nodeName The name of the DFT node to be set.
	 */
	void setNodeName(ASTIdentifier* nodeName) {
		this->nodeName = nodeName;
	}
	
	/**
	 * Returns the name of the DFT Node.
	 * @return the name of the DFT Node.
	 */
	ASTIdentifier* getNodeName() const {
		return nodeName;
	}
	
	/**
	 * Returns the page to which a DFT Node will be cast.
	 * @return The page to which a DFT Node will be cast.
	 */
	int getPage() const {
		return page;
	}
};

} // Namespace: DFT
} // Namespace: AST

#endif // DFT_AST_H
