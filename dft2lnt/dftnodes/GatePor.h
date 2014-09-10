/*
 * GatePOR.h
 *
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 *
 * @author Florian Arnold and extended by Dennis Guck
 */

class GatePor;

#ifndef GATEPOR_H
#define GATEPOR_H

#include "Gate.h"

namespace DFT {
namespace Nodes {
        
class GatePor: public Gate {
    private:
            
    public:
        GatePor(Location loc, std::string name):
        Gate(loc,name,GatePorType) {
        }
    virtual ~GatePor() {
    }
};
        
} // Namespace: Nodes
} // Namespace: DFT

#endif // GatePOR_H
