#include "DFTreeValidator.h"
#include "dft2lnt.h"

DFT::DFTreeValidator::DFTreeValidator(DFT::DFTree* dft, CompilerContext* cc):
	dft(dft),
	cc(cc) {
	
}

int DFT::DFTreeValidator::validateReferences() {
	int valid = true;
	std::vector<Nodes::Node*>& nodes = dft->getNodes();
	std::vector<Nodes::Node*> notDefinedButReferencedNodes;

	for(size_t i=0; i<nodes.size();++i) {
		assert(nodes.at(i));
		nodes.at(i)->addReferencesTo(notDefinedButReferencedNodes);
	}
	for(size_t i=0; i<nodes.size();++i) {
		//std::remove(notDefinedButReferencedNodes.begin(),notDefinedButReferencedNodes.end(),nodes.at(i));
		std::vector<Nodes::Node*>::iterator it = std::find(notDefinedButReferencedNodes.begin(),notDefinedButReferencedNodes.end(),nodes.at(i));
		if(it != notDefinedButReferencedNodes.end()) notDefinedButReferencedNodes.erase(it);
	}
	
	for(std::vector<Nodes::Node*>::iterator i = notDefinedButReferencedNodes.begin(); i != notDefinedButReferencedNodes.end(); i++) {
		cc->reportErrorAt((*i)->getLocation(),"node " + ((*i)->getName()) + " references inexistent node");
	}

	return valid;
}

int DFT::DFTreeValidator::validate() {
	int valid = true;
	valid = validateReferences() ? valid : false;
	return valid;
}
