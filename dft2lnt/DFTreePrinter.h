namespace DFT {
class DFTreePrinter;
}

#ifndef DFTREEPRINTER_H
#define DFTREEPRINTER_H

#include "DFTree.h"
#include "dft_parser.h"

namespace DFT {

class DFTreePrinter {
private:
	DFT::DFTree* dft;
	CompilerContext* cc;
	
	int validateReferences();
public:
	DFTreePrinter(DFT::DFTree* dft, CompilerContext* cc);
	virtual ~DFTreePrinter() {
	}

	int print           (std::ostream& out);
	int printBasicEvent (std::ostream& out, const DFT::Nodes::BasicEvent* basicEvent);
	int printGate       (std::ostream& out, const DFT::Nodes::Gate* gate);
	int printGateVoting (std::ostream& out, const DFT::Nodes::GateVoting* gate);
	int printGateAnd    (std::ostream& out, const DFT::Nodes::GateAnd* gate);
	int printGateOr     (std::ostream& out, const DFT::Nodes::GateOr* gate);

};

} // Namespace: DFT

#endif // DFTREEPRINTER_H
