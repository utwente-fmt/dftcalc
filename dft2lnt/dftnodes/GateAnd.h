/*
 * GateAnd.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

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
