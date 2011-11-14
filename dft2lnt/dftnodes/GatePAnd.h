class GatePAnd;

#ifndef GATEPAND_H
#define GATEPAND_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GatePAnd: public Gate {
private:
	
public:
	GatePAnd(Location loc, std::string name):
		Gate(loc,name,GatePAndType) {
	}
	virtual ~GatePAnd() {
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEPAND_H
