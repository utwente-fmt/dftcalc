/*
 * DFTreePrinter.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include "DFTreePrinter.h"
#include <iostream>

DFT::DFTreePrinter::DFTreePrinter(DFT::DFTree* dft, CompilerContext* cc):
	dft(dft),
	cc(cc) {

}

int DFT::DFTreePrinter::print(std::ostream& out) {
	assert(dft->getTopNode());
	out << "toplevel \"" << dft->getTopNode()->getName() << "\";" << std::endl;

	std::vector<Nodes::Node*>& nodes = dft->getNodes();
	for(DFT::Nodes::Node* node: nodes) {
		assert(node);
		if(node->isBasicEvent()) {
			DFT::Nodes::BasicEvent* be = static_cast<DFT::Nodes::BasicEvent*>(node);
			DFT::DFTreePrinter::printBasicEvent(out,be);
		} else if(node->isGate()) {
			DFT::Nodes::Gate* gate = static_cast<DFT::Nodes::Gate*>(node);
			DFT::DFTreePrinter::printGate(out,gate);
		} else {
			out << "UnknownNode";
		}
		out << std::endl;
	}
	return 0;
}

int DFT::DFTreePrinter::printBasicEvent(std::ostream& out, const DFT::Nodes::BasicEvent* basicEvent) {
	assert(basicEvent);
	out << "\"" << basicEvent->getName() << "\"";
	streamsize ss_old = out.precision();
	out.precision(10);
	out << " lambda=" << fixed << basicEvent->getLambda().str();
	out << " prob=" << fixed << basicEvent->getProb().str();
	out << " dorm=" << basicEvent->getDorm();
    if(basicEvent->getPhases()>0){
        out << " phases=" << basicEvent->getPhases();
    }
    if(basicEvent->getInterval()>0){
        out << " interval=" << basicEvent->getInterval();
    }
    if(basicEvent->getMaintain()>0){
        out << " maintain=" << basicEvent->getMaintain();
    }
	if(basicEvent->isRepairable() && basicEvent->getRepair() >= 0)
		out << " repair=" << basicEvent->getRepair();
	out.precision(ss_old);
	out << ";";
	if(basicEvent->getFailed()) {
		out << " /*FAILED*/";
	}
	return 0;
}

int DFT::DFTreePrinter::printGate(std::ostream& out, const DFT::Nodes::Gate* gate) {
	assert(gate);
	out << "\"" << gate->getName() << "\"";
	out << " " << gate->getTypeStr();
	if(DFT::Nodes::Node::typeMatch(gate->getType(),DFT::Nodes::GateFDEPType)) {
		const DFT::Nodes::GateFDEP* fdep = static_cast<const DFT::Nodes::GateFDEP*>(gate);
		printGateFDEP(out,fdep);
	} else {
		for(DFT::Nodes::Node* node: gate->getChildren()) {
			out << " \"" << node->getName() << "\"";
		}
		out << ";";
	}
	return 0;
}

int DFT::DFTreePrinter::printGateFDEP(std::ostream& out, const DFT::Nodes::GateFDEP* gate) {
	assert(gate);
	out << " \"" << gate->getEventSource()->getName() << "\"";
	for(DFT::Nodes::Node* node: gate->getDependers()) {
		out << " \"" << node->getName() << "\"";
	}
	out << ";";
	return 0;
}
