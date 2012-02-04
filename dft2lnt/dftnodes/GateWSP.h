/*
 * GateWSP.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

class GateVoting;

#ifndef GATEWSP_H
#define GATEWSP_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateWSP: public Gate {
private:
	
public:
	GateWSP(Location loc, std::string name):
		Gate(loc,name,GateWSPType) {
	}
	virtual ~GateWSP() {
	}
	virtual bool usesDynamicActivation() const { return true; }
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEWSP_H
