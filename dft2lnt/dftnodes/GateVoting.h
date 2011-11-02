class GateVoting;

#ifndef GATEVOTING_H
#define GATEVOTING_H

#include "Gate.h"

namespace DFT {
namespace Nodes {

class GateVoting: public Gate {
private:
	int threshold;
	int total;
public:
	GateVoting(std::string name, int threshold, int total):
		Gate(name,GateVotingType),
		threshold(threshold), 
		total(total) {
	}
	virtual ~GateVoting() {
	}
	
	int getThreshold() const {return threshold;}
	int getTotal() const {return total;}
};

} // Namespace: Nodes
} // Namespace: DFT

#endif // GATEVOTING_H
