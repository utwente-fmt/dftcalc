/*
 * GateSeq.h sequence enforcer
 *
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 *
 * @author Enno Ruijters
 */

class GateSeq;

#include "Gate.h"

namespace DFT {
    namespace Nodes {
        
        class GateSeq: public Gate {
        private:
            
        public:
            GateSeq(Location loc, std::string name):
            Gate(loc,name,GateSeqType) {
            }
            virtual ~GateSeq() {
            }
        };
        
    } // Namespace: Nodes
} // Namespace: DFT
