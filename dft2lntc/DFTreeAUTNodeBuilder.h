#include "automata/automata.h"
#include "dft2lnt.h"
#include "DFTreeNodeBuilder.h"
#include <set>

namespace DFT{
class DFTreeAUTNodeBuilder : public DFTreeNodeBuilder {
public:
	static const unsigned int VERSION;

	DFTreeAUTNodeBuilder(std::string cacheDir, DFT::DFTree* dft, CompilerContext* cc)
		:DFTreeNodeBuilder(dft, cc),
		 autRoot(cacheDir + DFT2LNT::AUT_CACHE_DIR + "/")
	{ }

	virtual std::string getFileForNode(const Nodes::Node& node);
	virtual std::string getFileForTopLevel();
	virtual std::string getRoot() {
		return autRoot;
	}
	virtual int generate();

private:
	set<std::string> alreadyGenerated;
	std::string autRoot;

	int generate(const Nodes::Node &node);
};
} /* Namespace DFT */
