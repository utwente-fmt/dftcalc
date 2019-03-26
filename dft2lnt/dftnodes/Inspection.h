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
            int phases;
            decnumber<> lambda;
            mutable std::string cachedName;
        public:
            Inspection(Location loc, std::string name, int phases, decnumber<> lambda):
                Gate(loc,name,InspectionType),
                phases(phases),
                lambda(lambda) {
            }
            virtual ~Inspection() {
            }
            
            virtual bool outputIsDumb() const { return true; }

            virtual bool repairsChildren() const { return true; }
            
            int getPhases() const {return phases;}
            decnumber<> getLambda() const {return lambda;}

            virtual const std::string& getTypeStr() const {
                if(cachedName.empty()) {
                    std::stringstream ss;
		    if (phases > 1)
			    ss << phases;
                    ss << "insp";
                    cachedName = ss.str();
                }
                return cachedName;
            }

            virtual void setChildRepairs()
            {
	            for (size_t i = 0; i < getChildren().size(); i++) {
					DFT::Nodes::Node *child = getChildren().at(i);
					if (child->isBasicEvent())
						child->setRepairable(true);
				}
            }
        };
    } // Namespace: Nodes
} // Namespace: DFT

#endif // INSPECTION_H
