/*
 * DFTreeValidator.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

namespace DFT {
class DFTreeValidator;
}

#ifndef DFTREEVALIDATOR_H
#define DFTREEVALIDATOR_H

#include "DFTree.h"
#include "dft_parser.h"

namespace DFT {

/**
 * Validates the DFT.
 * Note that a DFT generated from a valid AST should be guaranteed valid.
 */
class DFTreeValidator {
private:
	DFT::DFTree* dft;
	CompilerContext* cc;
	
	int validateReferences();
	int validateSingleParent();
	int validateNodes();
	int validateBasicEvent(const DFT::Nodes::BasicEvent& be);
	int validateGate(const DFT::Nodes::Gate& gate);
public:

	/**
	 * Constructs a new DFTreeValidator using the specified DFT and
	 * CompilerContext.
	 * @param dft The DFT to be validated.
	 * @param cc The CompilerConstruct used for eg error reports.
	 */
	DFTreeValidator(DFT::DFTree* dft, CompilerContext* cc);

	virtual ~DFTreeValidator() {
	}

	/**
	 * Start the validation process using the DFT specified in teh constructor.
	 * Returns whether the DFT is valid or not.
	 * @return true: valid DFT, false: invalid DFT
	 */
	int validate();

};

} // Namespace: DFT

#endif // DFTREEVALIDATOR_H
