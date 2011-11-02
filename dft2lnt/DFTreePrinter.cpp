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
			DFT::Nodes::GateOr* gate = static_cast<DFT::Nodes::GateOr*>(nodes.at(i));
			DFT::DFTreePrinter::printGateOr(out,gate);
			break;
		}
		case DFT::Nodes::GateAndType: {
			DFT::Nodes::GateAnd* gate = static_cast<DFT::Nodes::GateAnd*>(nodes.at(i));
			DFT::DFTreePrinter::printGateAnd(out,gate);
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
		case DFT::Nodes::GateVotingType: {
			DFT::Nodes::GateVoting* gate = static_cast<DFT::Nodes::GateVoting*>(nodes.at(i));
			DFT::DFTreePrinter::printGateVoting(out,gate);
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

int DFT::DFTreePrinter::printBasicEvent(std::ostream& out, const DFT::Nodes::BasicEvent* basicEvent) {
	assert(basicEvent);
	out << "BasicEvent[" << basicEvent->getName() << "]";
	return 0;
}

int DFT::DFTreePrinter::printGate(std::ostream& out, const DFT::Nodes::Gate* gate) {
	assert(gate);
	out << "Gate[" << gate->getName() << "] ";
	out << gate->getType();
	return 0;
}

int DFT::DFTreePrinter::printGateOr(std::ostream& out, const DFT::Nodes::GateOr* gate) {
	assert(gate);
	out << "GateOr[" << gate->getName() << "] ";
	return 0;
}

int DFT::DFTreePrinter::printGateAnd(std::ostream& out, const DFT::Nodes::GateAnd* gate) {
	assert(gate);
	out << "GateAnd[" << gate->getName() << "] ";
	return 0;
}

int DFT::DFTreePrinter::printGateVoting(std::ostream& out, const DFT::Nodes::GateVoting* gate) {
	assert(gate);
	out << "GateVoting[" << gate->getName() << "] ";
	out << "( " << gate->getThreshold() << " / " << gate->getTotal() << " )";
	return 0;
}
