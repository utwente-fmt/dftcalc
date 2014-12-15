/*
 * Inspection.h
 *
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 *
 * @author Dennis Guck
 */

class Inspection;

#ifndef INSPECTION_H
#define INSPECTION_H

#include "Gate.h"

namespace DFT {
    namespace Nodes {
        
        class Inspection: public Gate {
        private:
            vector<DFT::Nodes::Node*> dependers;
        public:
            Inspection(Location loc, std::string name):
            Gate(loc,name,InspectionType) {
            }
            Inspection(Location loc, std::string name, NodeType type):
            Gate(loc,name,type) {
            }
            virtual ~Inspection() {
            }
            
            void setDependers(vector<DFT::Nodes::Node*> dependers) {
                this->dependers = dependers;
            }
            vector<DFT::Nodes::Node*>& getDependers() {
                return dependers;
            }
            const vector<DFT::Nodes::Node*>& getDependers() const {
                return dependers;
            }
            virtual bool outputIsDumb() const { return true; }
            
            DFT::Nodes::Node* getEventSource() const {
                if(getChildren().size()!=1) return NULL;
                else return getChildren()[0];
            }
        };
        
    } // Namespace: Nodes
} // Namespace: DFT

#endif // INSPECTION_H
