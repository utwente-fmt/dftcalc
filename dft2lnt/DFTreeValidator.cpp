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
		for(;;) {
			std::vector<Nodes::Node*>::iterator it = std::find(notDefinedButReferencedNodes.begin(),notDefinedButReferencedNodes.end(),nodes.at(i));
			if(it == notDefinedButReferencedNodes.end()) break;
			notDefinedButReferencedNodes.erase(it);
		}
	}
	
	for(std::vector<Nodes::Node*>::iterator i = notDefinedButReferencedNodes.begin(); i != notDefinedButReferencedNodes.end(); i++) {
		cc->reportErrorAt((*i)->getLocation(),"node " + ((*i)->getName()) + " references inexistent node");
	}

	return valid;
}

int DFT::DFTreeValidator::validateNodes() {
	int valid = true;
	std::vector<Nodes::Node*>& nodes = dft->getNodes();

	for(size_t i=0; i<nodes.size();++i) {
		DFT::Nodes::Node* node = nodes.at(i);
		if(DFT::Nodes::Node::typeMatch(node->getType(),DFT::Nodes::BasicEventType)) {
			DFT::Nodes::BasicEvent* be = static_cast<DFT::Nodes::BasicEvent*>(node);
			assert(be);
			validateBasicEvent(*be);
		} else if(DFT::Nodes::Node::typeMatch(node->getType(),DFT::Nodes::GateType)) {
			DFT::Nodes::Gate* gate = static_cast<DFT::Nodes::Gate*>(node);
			assert(gate);
			validateGate(*gate);
		} else {
			assert(0);
		}
	}

	return valid;
}

int DFT::DFTreeValidator::validateBasicEvent(const DFT::Nodes::BasicEvent& be) {
	int valid = true;

	float l = be.getLambda();
	if(l<0) {
		valid = false;
		cc->reportErrorAt(be.getLocation(),"BasicEvent `" + (be.getName()) + "': has negative lambda");
	}
	
	float m = be.getMu();
	if(m<0) {
		valid = false;
		cc->reportErrorAt(be.getLocation(),"BasicEvent `" + (be.getName()) + "': has negative mu");
	}

	return valid;
}

int DFT::DFTreeValidator::validateGate(const DFT::Nodes::Gate& gate) {
	return true;
}

int DFT::DFTreeValidator::validate() {
	int valid = true;
	valid = validateReferences() ? valid : false;
	valid = validateNodes() ? valid : false;
	return valid;
}
