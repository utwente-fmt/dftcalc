/*
 * GateOr.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

class GateOr;

#ifndef GATEOR_H
#define GATEOR_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateOr: public Gate {
private:
	
public:
	GateOr(Location loc, std::string name):
		Gate(loc,name,GateOrType) {
	}
	virtual ~GateOr() {
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEOR_H
