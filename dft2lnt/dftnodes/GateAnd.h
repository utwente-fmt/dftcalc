class GateAnd;

#ifndef GATEAND_H
#define GATEAND_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateAnd: public Gate {
private:
	
public:
	GateAnd(Location loc, std::string name):
		Gate(loc,name,GateAndType) {
	}
	virtual ~GateAnd() {
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEAND_H
