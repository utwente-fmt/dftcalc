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

	int print(std::ostream& out);
	int printBasicEvent(std::ostream& out, DFT::Nodes::BasicEvent* basicEvent);
	int printGate(std::ostream& out, DFT::Nodes::Gate* gate);

};

} // Namespace: DFT

#endif // DFTREEPRINTER_H
