#ifndef DFTCALC_CHECKER_H
#define DFTCALC_CHECKER_H

#include "MessageFormatter.h"
#include "query.h"
#include "executor.h"
#include "DFTCalculationResult.h"
#include <unordered_map>
#include <vector>

namespace DFT {
	extern const int VERBOSITY_FLOW; /* Defined in dftcalc.cpp */
};

class Checker {
protected:
	MessageFormatter *messageFormatter;
	DFT::CommandExecutor *exec;

public:
	Checker(MessageFormatter *mf, DFT::CommandExecutor *exec)
		:messageFormatter(mf), exec(exec)
	{}

	/* Analyze the provided queries on the given model, returning
	 * lower and upper bounds on the results.
	 * The returned queries need not be the same ones (in
	 * particular, a [min, max, step] query should probably be
	 * returned as one query per time), but the set of returned
	 * queries should be identical when given the same set of input
	 * queries.
	 */
	virtual std::vector<DFT::DFTCalculationResultItem> analyze(
			std::vector<Query> queries) = 0;

	virtual ~Checker()
	{}
};
#endif
