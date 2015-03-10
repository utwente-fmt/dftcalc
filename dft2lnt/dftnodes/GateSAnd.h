/*
 * GateSAnd.h sequence AND without sharing and whatsoever
 *
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 *
 * @author Dennis Guck
 */

class GateAnd;

#ifndef GATESAND_H
#define GATESAND_H

#include "Gate.h"

namespace DFT {
    namespace Nodes {
        
        class GateSAnd: public Gate {
        private:
            
        public:
            GateSAnd(Location loc, std::string name):
            Gate(loc,name,GateSAndType) {
            }
            virtual ~GateSAnd() {
            }
        };
        
    } // Namespace: Nodes
} // Namespace: DFT

#endif // GATESAND_H
