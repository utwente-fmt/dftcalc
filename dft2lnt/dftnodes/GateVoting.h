/*
 * GateVoting.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

class GateVoting;

#ifndef GATEVOTING_H
#define GATEVOTING_H

#include <string>
#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateVoting: public Gate {
private:
	int threshold;
	int total;
	mutable std::string cachedName;
public:
	GateVoting(Location loc, std::string name, int threshold, int total):
		Gate(loc,name,GateVotingType),
		threshold(threshold), 
		total(total) {
	}
	virtual ~GateVoting() {
	}
	
	int getThreshold() const {return threshold;}
	int getTotal() const {return total;}

	virtual const std::string& getTypeStr() const {
		if(cachedName.empty()) {
			std::stringstream ss;
			ss << threshold;
			ss << "of";
			ss << total;
			cachedName = ss.str();
		}
		return cachedName;
	}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEVOTING_H
