#include "automata/insp.h"

namespace automata {
using namespace signals;

void insp::insp_state::initialize_outgoing() {
	size_t i;
	const insp *par = (const insp *)get_parent();

	for (i = 1; i <= par->total; i++) {
		insp_state target = *this;
		target.signal = true;
		add_transition(INSPECT(i), target);
	}

	if (counter < par->phases) {
		insp_state target = *this;
		target.counter++;
		if (target.counter == par->phases && !signal)
			target.counter = 0;
		add_transition(RATE_INSPECTION(1), target);
	}

	if (counter == par->phases) {
		insp_state target = *this;
		target.counter = 0;
		target.signal = false;
		add_transition(REPAIR(0), target);
	}
}

insp::insp_state::operator std::string() const {
	std::string ret = "[";
	ret += std::to_string(counter);
	if (signal)
		ret += ", signalled";
	ret += "]";
	return ret;
}

} /* Namespace automata */
