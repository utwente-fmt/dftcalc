/*
 * BasicEvent.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

class BasicEvent;

#ifndef BASICEVENT_H
#define BASICEVENT_H

#include "Node.h"

namespace DFT {
namespace Nodes {

namespace BE {

/**
 * The type of attributes a BasicEvent can have.
 */
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

enum class CalculationMode {
	UNDEFINED = 0,
	EXPONENTIAL,
	WEIBULL, // not supported
	NUMBER_OF
	
};

const std::string& getCalculationModeStr(CalculationMode mode);

/**
 * The label of an attribute
 */
class AttributeLabel {
private:
	AttributeLabelType labelType;
	std::string label;
public:

	/**
	 * Constructs a new AttributeLabel using the specified labelType and
	 * label name
	 */
	AttributeLabel(AttributeLabelType labelType, std::string label):
		labelType(labelType),
		label(label) {
	}
	
	/**
	 * Returns the label name of this label.
	 * @return The label name of this label.
	 */
	const std::string& getLabel() const {
		return label;
	}
	
	/**
	 * Returns the label type of this label.
	 * @return The label type of this label.
	 */
	const AttributeLabelType& getLabelType() const {
		return labelType;
	}
};

/**
 * Attribute superclass handling type and name. The value is handled by the
 * individual subclasses.
 */
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
	
	/**
	 * Returns the label type of this attribute.
	 * @return The label type of this attribute.
	 */
	const AttributeLabelType& getLabel() const {
		return label;
	}
	
	/**
	 * Returns the label name of this attribute.
	 * @return The label name of this attribute.
	 */
	std::string getName() {
		return name;
	}
	
	/**
	 * Sets the label name of this attribute.
	 * @param The label name to be set.
	 */
	void setName(std::string name) {
		this->name = name;
	}
//	TValue getValue() { return value; }
//	void setValue(TValue value) { this->value = value; }
};

class AttribFloat: public Attrib {
private:
	double value;
public:
	AttribFloat(std::string name, double value):
		Attrib(name),
		value(value) {

	}

	/**
	 * Returns the value of this attribute.
	 * @return The value of this attribute.
	 */
	double getValue() {
		return value;
	}

	/**
	 * Sets the value of this attribute.
	 * @return The value to be set.
	 */
	void setValue(double value) {
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

	/**
	 * Returns the value of this attribute.
	 * @return The value of this attribute.
	 */
	int getValue() {
		return value;
	}

	/**
	 * Sets the value of this attribute.
	 * @return The value to be set.
	 */
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

	/**
	 * Returns the value of this attribute.
	 * @return The value of this attribute.
	 */
	std::string getValue() {
		return value;
	}

	/**
	 * Sets the value of this attribute.
	 * @return The value to be set.
	 */
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

/**
 * The BasicEvent DFT Node
 * FIXME: different implementations (exponentional, weibull) should be
 * separate subclasses.
 */
class BasicEvent: public Node {
private:
	DFT::Nodes::BE::CalculationMode mode;
	double lambda;
	double mu;
//	double dorm;
	double rate;
	int shape;
	bool failed;
public:
	
	void setLambda(double lambda) {
		this->lambda = lambda;
	}
	void setMu(double mu) {
		this->mu = mu;
	}
	void setRate(double rate) {
		this->rate = rate;
	}
	void setShape(int shape) {
		this->shape = shape;
	}
	void setMode(const DFT::Nodes::BE::CalculationMode& mode) {
		this->mode = mode;
	}
	
	/**
	 * Returns the lambda failure probability of this Basic Event.
	 * @return The lambda failure probability of this Basic Event.
	 */
	double getLambda() const { return lambda; }
	
	/**
	 * Returns the mu failure probability of this Basic Event.
	 * @return The mu failure probability of this Basic Event.
	 */
	double getMu()     const { return mu; }
	
	/**
	 * Returns the dormancy factor (mu/lambda) of this Basic Event.
	 * @return The dormancy factor (mu/lambda) of this Basic Event.
	 */
	double getDorm()   const { return mu / lambda; }
	
	const DFT::Nodes::BE::CalculationMode& getMode() const {
		return mode;
	}
	
	/**
	 * Creates a new Basic Event instance, originating from the specified
	 * location and with the specified name.
	 * @param loc Source location of the Basic Event.
	 * @param name The name of the Basic Event.
	 */
	BasicEvent(Location loc, std::string name):
		Node(loc,name,BasicEventType),
		mode(DFT::Nodes::BE::CalculationMode::UNDEFINED),
		lambda(-1),
		mu(0),
		rate(-1),
		shape(-1),
		failed(false) {
	}
	virtual ~BasicEvent() {
	}
	
	/**
	 * Mark this Basic Event as failed or not. Being marked as failed means the
	 * Basic Event will fail at t=0, the moment the system starts. Otherwise
	 * the Basic Event will fail according to its mu and lambda.
	 * @param failed true/false: whether or not this BE is marked as failed.
	 */
	void setFailed(bool failed) {
		this->failed = failed;
	}
	
	/**
	 * Returns whether or not this Basic Event is marked as being failed.
	 * @return Whether or not this Basic Event is marked as being failed.
	 */
	bool getFailed() const {
		return failed;
	}
	
	virtual bool isBasicEvent() const { return true; }
	virtual bool isGate() const { return false; }
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // BASICEVENT_H
