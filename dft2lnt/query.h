#ifndef DFTCALC_QUERY_H
#define DFTCALC_QUERY_H
#include <string>
#include <vector>
#include "decnumber.h"

enum query_type { UNSPECIFIED, TIMEBOUND, UNBOUNDED, STEADY, EXPECTEDTIME, CUSTOM };

class Query {
public:
	enum query_type type;
	bool min; /* If false, default to max */
	decnumber<> lowerBound, upperBound, step; /* step == -1 for single time */
	std::string customQuery;
	decnumber<> errorBound;
	bool errorBoundSet;

	Query() :type(UNSPECIFIED), lowerBound(-1), upperBound(-1), step(-1), errorBound("1e-6") {}

	std::string toString() {
		switch (type) {
		case UNSPECIFIED:
			return "?";
		case CUSTOM:
			return customQuery;
		case TIMEBOUND:
			if (step != -1)
				return "P(F[in list] FAIL)";
			if (lowerBound == 0)
				return "P(F[<=" + upperBound.str() + "] FAIL)";
			else
				return "P(F[" + lowerBound.str() + ", " + upperBound.str() + "] FAIL)";
		case STEADY:
			return "S(FAILED)";
		case EXPECTEDTIME:
			return "MTTF(FAIL)";
		case UNBOUNDED:
			return "P(F FAIL)";
		default:
			throw std::logic_error("Unknown query type");
		}
	}
};

/* Defined in mrmc.cpp */
void expandRangeQueries(std::vector<Query> &queries);

#endif
