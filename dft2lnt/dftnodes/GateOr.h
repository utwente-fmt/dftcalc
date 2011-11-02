class GateOr;

#ifndef GATEOR_H
#define GATEOR_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateOr: public Gate {
private:
	
public:
	GateOr(std::string name): Gate(name,GateOrType) {
	}
	virtual ~GateOr() {
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEOR_H
