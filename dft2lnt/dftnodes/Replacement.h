/*
 * Replacement.h
 *
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 *
 * @author Dennis Guck
 */

class Replacement;

#ifndef REPLACEMENT_H
#define REPLACEMENT_H

#include "Gate.h"

namespace DFT {
    namespace Nodes {
        
        class Replacement: public Gate {
        private:
            vector<DFT::Nodes::Node*> dependers;
            int phases;
            double lambda;
            mutable std::string cachedName;
        public:
            Replacement(Location loc, std::string name, int phases, double lambda):
            Gate(loc,name,ReplacementType),
            phases(phases),
            lambda(lambda) {
            }
            virtual ~Replacement() {
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
            
            int getPhases() const {return phases;}
            double getLambda() const {return lambda;}
            
            virtual const std::string& getTypeStr() const {
                if(cachedName.empty()) {
                    std::stringstream ss;
                    ss << phases;
                    ss << "rep";
                    //ss << lambda;
                    cachedName = ss.str();
                }
                return cachedName;
            }
            
        };
        
    } // Namespace: Nodes
} // Namespace: DFT

#endif // REPLACEMENT_H
