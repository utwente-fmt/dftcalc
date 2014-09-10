/*
 * GateSEQOr.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Florian Arnold
 */

class GateSEQOr;

#ifndef GATESEQOR_H
#define GATESEQOR_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateSEQOr: public Gate {
private:
	
public:
	GateSEQOr(Location loc, std::string name):
		Gate(loc,name,GateSEQOrType) {
	}
	virtual ~GateSEQOr() {
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GateSEQOr_H
