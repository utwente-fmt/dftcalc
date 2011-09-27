class GateAnd;

#ifndef GATEAND_H
#define GATEAND_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateAnd: public Gate {
private:
	
public:
	GateAnd(std::string name): Gate(name) {
	}
	virtual ~GateAnd() {
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEAND_H
