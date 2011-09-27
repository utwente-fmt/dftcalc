class BasicEvent;

#ifndef BASICEVENT_H
#define BASICEVENT_H

#include "Node.h"

namespace DFT {
namespace Nodes {

namespace BE {

enum AttributeLabelType {
    AttrLabelNone = 0,

    AttrLabelOther = 1,

    AttrLabelProb = 10,
    AttrLabelLambda,
    AttrLabelRate,
    AttrLabelShape,
    AttrLabelMean,
    AttrLabelStddev,
    AttrLabelCov,
    AttrLabelRes,
    AttrLabelRepl,
    AttrLabelDorm,
    AttrLabelAph
};

class AttributeLabel {
private:
	AttributeLabelType labelType;
	std::string label;
public:
	AttributeLabel(AttributeLabelType labelType, std::string label):
		labelType(labelType),
		label(label) {
	}
	const std::string& getLabel() const {
		return label;
	}
	const AttributeLabelType& getLabelType() const {
		return labelType;
	}
};

class Attrib {
public:
//	static std::string Lambda;
private:
	AttributeLabelType label;
	std::string name;
	//TValue value;
public:
	Attrib(std::string name):
		name(name) {

	}
	const AttributeLabelType& getLabel() const {
		return label;
	}
	std::string getName() {
		return name;
	}
	void setName(std::string name) {
		this->name = name;
	}
//	TValue getValue() { return value; }
//	void setValue(TValue value) { this->value = value; }
};

class AttribFloat: public Attrib {
private:
	float value;
public:
	AttribFloat(std::string name, float value):
		Attrib(name),
		value(value) {

	}
	float getValue() {
		return value;
	}
	void setValue(float value) {
		this->value = value;
	}
};

class AttribNumber: public Attrib {
private:
	int value;
public:
	AttribNumber(std::string name, int value):
		Attrib(name),
		value(value) {

	}
	int getValue() {
		return value;
	}
	void setValue(int value) {
		this->value = value;
	}
};

class AttribString: public Attrib {
private:
	std::string value;
public:
	AttribString(std::string name, std::string value):
		Attrib(name),
		value(value) {

	}
	std::string getValue() {
		return value;
	}
	void setValue(std::string value) {
		this->value = value;
	}
};

//class AttribFloat: public Attrib<float> {
//	AttribFloat(std::string name, float value): Attrib<float>(name,value) {
//	}
//};
//
//class AttribNumber: public Attrib<int> {
//	AttribNumber(std::string name, int value): Attrib<int>(name,value) {
//	}
//};
//
//class AttribString: public Attrib<std::string> {
//	AttribString(std::string name, std::string value): Attrib<std::string>(name,value) {
//	}
//};

} // Namespace: BE

class BasicEvent: public Node {
private:
	float lambda;
	float mu;
public:
	BasicEvent(std::string name):
		Node(name,BasicEventType) {
	}
	virtual ~BasicEvent() {
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // BASICEVENT_H
