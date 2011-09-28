namespace DFT {
class DFTreeValidator;
}

#ifndef DFTREEVALIDATOR_H
#define DFTREEVALIDATOR_H

#include "DFTree.h"
#include "dft_parser.h"

namespace DFT {

class DFTreeValidator {
private:
	DFT::DFTree* dft;
	CompilerContext* cc;
	
	int validateReferences();
public:
	DFTreeValidator(DFT::DFTree* dft, CompilerContext* cc);
	virtual ~DFTreeValidator() {
	}

	int validate();

};

} // Namespace: DFT

#endif // DFTREEVALIDATOR_H
