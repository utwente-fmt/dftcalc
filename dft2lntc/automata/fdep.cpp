#include "automata/fdep.h"

namespace automata {
using namespace signals;

void fdep::fdep_state::initialize_outgoing() {
	const fdep *par = (const fdep *)get_parent();

	fdep_state target = *this;
	target.failed = true;
	add_transition(FAIL(1), target);

	target = *this;
	target.act = 1;
	add_transition(ACTIVATE(0, false), target);

	if (act == 1 && !par->trigger_always_active) {
		target = *this;
		target.act = 0;
		add_transition(ACTIVATE(1, true), target);
	}

	/* Nondeterministically fail the dependers after trigger
	 * failure.
	 */
	if (failed) {
		for (size_t i = 0; i < par->total; i++) {
			if (!notified[i]) {
				target = *this;
				target.notified[i] = true;
				add_transition(FAIL(i + 2), target);
			}
		}
	}
}

fdep::fdep_state::operator std::string() const {
	std::string ret = "[";
	ret += std::to_string(act);
	if (failed)
		ret += ", failed";
	ret += ", {";
	for (size_t i = 0; i < notified.size(); i++)
		ret += notified[i] ? "T" : "F";
	ret += "}]";
	return ret;
}

} /* Namespace automata */
