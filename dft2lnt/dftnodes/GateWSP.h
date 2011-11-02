class GateVoting;

#ifndef GATEWSP_H
#define GATEWSP_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateWSP: public Gate {
private:
	
public:
	GateWSP(std::string name):
		Gate(name,GateWSPType) {
	}
	virtual ~GateWSP() {
	}
	virtual bool usesDynamicActivation() const { return true; }
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEWSP_H
