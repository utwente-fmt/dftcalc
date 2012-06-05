/*
 * DFTreeValidator.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

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

int DFT::DFTreeValidator::validateSingleParent() {
	int valid = true;
	std::vector<Nodes::Node*>& nodes = dft->getNodes();
	std::map<Nodes::Node*,std::vector<Nodes::Node*>*> parents;

	for(size_t i=0; i<nodes.size();++i) {
		assert(nodes.at(i));
		parents.insert(pair<Nodes::Node*,std::vector<Nodes::Node*>*>(nodes.at(i),new std::vector<Nodes::Node*>()));
	}
	for(size_t i=0; i<nodes.size();++i) {
		Nodes::Node& parent = *nodes.at(i);
		if(parent.isGate()) {
			Nodes::Gate& gate = static_cast<Nodes::Gate&>(parent);
			for(size_t c=0; c<gate.getChildren().size(); ++c) {
				Nodes::Node& child = *gate.getChildren().at(c);
				std::map<Nodes::Node*,std::vector<Nodes::Node*>*>::iterator it = parents.find(&child);
				std::vector<Nodes::Node*>* cParents = it->second;
				if(cParents) {
					cParents->push_back(&parent);
				} else {
					assert(0);
				}
			}
		}
	}
	
	for(size_t i=0; i<nodes.size();++i) {
		Nodes::Node& node = *nodes.at(i);
		std::map<Nodes::Node*,std::vector<Nodes::Node*>*>::iterator it = parents.find(&node);
		std::vector<Nodes::Node*>* cParents = it->second;
		if(cParents->size() == 0 && &node != dft->getTopNode()) {
			cc->reportErrorAt(node.getLocation(),"non-top node " + (node.getName()) + " has no parents");
			valid = 0;
		} else if(cParents->size() > 1) {
			cc->reportErrorAt(node.getLocation(),"node " + (node.getName()) + " has multiple parents:");
			valid = 0;
		}
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
			valid = validateBasicEvent(*be) ? valid : false;
		} else if(DFT::Nodes::Node::typeMatch(node->getType(),DFT::Nodes::GateType)) {
			DFT::Nodes::Gate* gate = static_cast<DFT::Nodes::Gate*>(node);
			assert(gate);
			valid = validateGate(*gate) ? valid : false;
		} else {
			assert(0);
		}
	}

	return valid;
}

int DFT::DFTreeValidator::validateBasicEvent(const DFT::Nodes::BasicEvent& be) {
	int valid = true;
	
	switch(be.getMode()) {
	case DFT::Nodes::BE::CalculationMode::EXPONENTIAL: {
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
		break;
	}
	case DFT::Nodes::BE::CalculationMode::APH: {
		// TODO check that file exists
		break;
	}
	case DFT::Nodes::BE::CalculationMode::WEIBULL: {
		valid = false;
		cc->reportErrorAt(be.getLocation(),"BasicEvent `" + (be.getName()) + "': Weibull distribution is not supported yet");
		break;
	}
	default: {
		valid = false;
		cc->reportErrorAt(be.getLocation(),"BasicEvent `" + (be.getName()) + "': has unknown calculation mode");
		break;
	}
	}
	
	return valid;
}

int DFT::DFTreeValidator::validateGate(const DFT::Nodes::Gate& gate) {
	return true;
}

int DFT::DFTreeValidator::validate() {
	int valid = true;
	valid = validateReferences() ? valid : false;
	//valid = validateSingleParent() ? valid : false;
	valid = validateNodes() ? valid : false;
	return valid;
}
