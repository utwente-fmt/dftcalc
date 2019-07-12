#ifndef DFTREENODEBUILDER_H
#define DFTREENODEBUILDER_H

#include "DFTree.h"
#include "compiler.h"

namespace DFT {
class DFTreeNodeBuilder {
protected:
	const DFT::DFTree* dft;
	CompilerContext* cc;

	static std::string getNodeName(const DFT::Nodes::Node& node);
public:
	DFTreeNodeBuilder(DFT::DFTree* dft, CompilerContext* cc)
		:dft(dft), cc(cc)
	{ }

	/**
	 * Returns the Lotos NT File needed for the specified node.
	 * @return he Lotos NT File needed for the specified node.
	 */
	virtual std::string getFileForNode(const Nodes::Node& node) = 0;
	virtual std::string getFileForTopLevel() = 0;
	virtual std::string getRoot() = 0;
	virtual int generate() = 0;
};
} /* Namespace DFT */

#endif
