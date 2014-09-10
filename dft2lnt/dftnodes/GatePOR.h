/*
 * GatePOR.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Florian Arnold and extended by Dennis Guck
 */

class GatePOR;

#ifndef GATEPOR_H
#define GATEPOR_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GatePOR: public Gate {
private:
	
public:
	GatePOR(Location loc, std::string name):
		Gate(loc,name,GatePORType) {
	}
	virtual ~GatePOR() {
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GatePOR_H
