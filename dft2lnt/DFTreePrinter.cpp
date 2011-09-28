#include "DFTreePrinter.h"
#include <iostream>

DFT::DFTreePrinter::DFTreePrinter(DFT::DFTree* dft, CompilerContext* cc):
	dft(dft),
	cc(cc) {

}

int DFT::DFTreePrinter::print(std::ostream& out) {
	assert(dft->getTopNode());
	out << "Top Node: " << dft->getTopNode()->getName() << std::endl;

	std::vector<Nodes::Node*>& nodes = dft->getNodes();
	for(int i=0; i<nodes.size(); ++i) {
		assert(nodes.at(i));
		switch(nodes.at(i)->getType()) {
		case DFT::Nodes::BasicEventType: {
			DFT::Nodes::BasicEvent* be = static_cast<DFT::Nodes::BasicEvent*>(nodes.at(i));
			DFT::DFTreePrinter::printBasicEvent(out,be);
			break;

		}
		case DFT::Nodes::GateType: {
			DFT::Nodes::Gate* gate = static_cast<DFT::Nodes::Gate*>(nodes.at(i));
			DFT::DFTreePrinter::printGate(out,gate);
			break;
		}
		}
	}
}

int DFT::DFTreePrinter::printBasicEvent(std::ostream& out, DFT::Nodes::BasicEvent* basicEvent) {
	assert(basicEvent);
	out << "BasicEvent[" << basicEvent->getName() << "]" << std::endl;
}
int DFT::DFTreePrinter::printGate(std::ostream& out, DFT::Nodes::Gate* gate) {
	assert(gate);
	out << "Gate[" << gate->getName() << "] ";
	out << gate->getType() << std::endl;
}
