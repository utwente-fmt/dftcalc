class GateFDEP;

#ifndef GATEFDEP_H
#define GATEFDEP_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateFDEP: public Gate {
private:
	vector<DFT::Nodes::Node*> dependers;
public:
	GateFDEP(Location loc, std::string name):
		Gate(loc,name,GateFDEPType) {
	}
	virtual ~GateFDEP() {
	}
	
	void setDependers(vector<DFT::Nodes::Node*> dependers) {
		this->dependers = dependers;
	}
	vector<DFT::Nodes::Node*>& getDependers() {
		return dependers;
	}
	const vector<DFT::Nodes::Node*>& getDependers() const {
		return dependers;
	}
	virtual bool outputIsDumb() const { return true; }

	DFT::Nodes::Node* getEventSource() const {
		if(getChildren().size()!=1) return NULL;
		else return getChildren()[0];
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEPAND_H
