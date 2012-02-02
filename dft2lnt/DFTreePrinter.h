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
	int printGateFDEP   (std::ostream& out, const DFT::Nodes::GateFDEP* gate);

};

} // Namespace: DFT

#endif // DFTREEPRINTER_H
