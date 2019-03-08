#ifndef DFTCALC_QUERY_H
#define DFTCALC_QUERY_H
#include <string>

enum query_type { UNSPECIFIED, TIMEBOUND, STEADY, EXPECTEDTIME, CUSTOM };

struct query {
	enum query_type type;
	bool min; /* If false, default to max */
	std::string lowerBound, upperBound, step;
	std::string customQuery;
	std::string errorBound;
};

#endif
