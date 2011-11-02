class GatePAnd;

#ifndef GATEPAND_H
#define GATEPAND_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GatePAnd: public Gate {
private:
	
public:
	GatePAnd(std::string name):
		Gate(name,GatePAndType) {
	}
	virtual ~GatePAnd() {
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEPAND_H
