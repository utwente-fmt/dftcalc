#include "dftreevalidator.h"
#include "dft_parser.h"

DFT::DFTreeValidator::DFTreeValidator(DFT::DFTree* dft, Parser* parser):
	dft(dft),
	parser(parser) {
	
}

int DFT::DFTreeValidator::validateReferences() {
	int valid = true;
	std::vector<Nodes::Node*>& nodes = dft->getNodes();
	std::vector<Nodes::Node*> notDefinedButReferencedNodes;

	for(int i=0; i<nodes.size();++i) {
		assert(nodes.at(i));
		nodes.at(i)->addReferencesTo(notDefinedButReferencedNodes);
	}
	for(int i=0; i<nodes.size();++i) {
		//std::remove(notDefinedButReferencedNodes.begin(),notDefinedButReferencedNodes.end(),nodes.at(i));
		std::vector<Nodes::Node*>::iterator it = std::find(notDefinedButReferencedNodes.begin(),notDefinedButReferencedNodes.end(),nodes.at(i));
		if(it != notDefinedButReferencedNodes.end()) notDefinedButReferencedNodes.erase(it);
	}
	
	for(std::vector<Nodes::Node*>::iterator i = notDefinedButReferencedNodes.begin(); i != notDefinedButReferencedNodes.end(); i++) {
		parser->getCC()->reportErrorAt((*i)->getLocation(),"node " + ((*i)->getName()) + " references inexistent node");
	}

	return valid;
}

int DFT::DFTreeValidator::validate() {
	int valid = true;
	valid = validateReferences() ? valid : false;
	return valid;
}
