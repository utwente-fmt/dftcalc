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
	for(size_t i=0; i<nodes.size(); ++i) {
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
		case DFT::Nodes::GatePhasedOrType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GateOrType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GateAndType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GateHSPType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GateWSPType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GateCSPType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GatePAndType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GateSeqType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GateOFType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GateFDEPType: {
			assert(0);
			break;
		}
		case DFT::Nodes::GateTransferType: {
			assert(0);
			break;
		}
		default: {
			out << "UnknownNode";
			break;
		}
		}
		out << std::endl;
	}
	return 0;
}

int DFT::DFTreePrinter::printBasicEvent(std::ostream& out, DFT::Nodes::BasicEvent* basicEvent) {
	assert(basicEvent);
	out << "BasicEvent[" << basicEvent->getName() << "]";
	return 0;
}
int DFT::DFTreePrinter::printGate(std::ostream& out, DFT::Nodes::Gate* gate) {
	assert(gate);
	out << "Gate[" << gate->getName() << "] ";
	out << gate->getType();
	return 0;
}
