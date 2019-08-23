#include "automata/pand.h"

namespace automata {
using namespace signals;

void pand::pand_state::initialize_outgoing() {
	if (terminated)
		return;

	pand_state target(*this);
	if (impossible) {
		target.terminated = 1;
		add_transition(GATE_IMPOSSIBLE, target);
		return;
	}
	size_t i;
	const pand *par = (const pand *)get_parent();

	for (i = 1; i <= par->total; i++) {
		target = *this;
		if (!failed[i]) {
			target.failed[i] = 1;
			target.nr_failed++;
			if (i == in_order_mark + 1)
				target.in_order_mark = i;
		}
		add_transition(FAIL(i), target);
	}

	for (i = 1; i <= par->total; i++) {
		target = *this;
		if (!par->repairable || !failed[i]) {
			target.impossible = 1;
		} else {
			target.failed[i] = 0;
			target.nr_failed--;
			if (i <= in_order_mark)
				target.in_order_mark = i - 1;
		}
		add_transition(ONLINE(i), target);
	}

	if (in_order_mark == par->total && !done) {
		target = *this;
		target.done = true;
		add_transition(FAIL(0), target);
	}

	if (in_order_mark < par->total && done) {
		target = *this;
		target.done = false;
		add_transition(ONLINE(0), target);
	}

	target = *this;
	if (!par->always_active) {
		if (nr_active == 0)
			target.nr_active = 1;
	}
	add_transition(ACTIVATE(0, false), target);

	target = *this;
	target.impossible = 1;
	add_transition(DEACTIVATE(0, false), target);

	if (nr_active > 0) {
		target = *this;
		if (nr_active < par->total)
			target.nr_active = nr_active + 1;
		else
			target.nr_active = 0;
		add_transition(ACTIVATE(nr_active, true), target);
	}
}

pand::pand_state::operator std::string() const {
	if (terminated)
		return "[]";
	if (impossible)
		return "[IMPOSSIBLE]";
	std::string ret = "[";
	ret += std::to_string(nr_failed);
	ret += ", io=";
	ret += std::to_string(in_order_mark);
	ret += ", act=";
	ret += std::to_string(nr_active);
	if (done)
		ret += ", done";
	ret += ", {";
	for (size_t i = 0; i < failed.size(); i++) {
		if (!failed[i])
			ret += 'A';
		else
			ret += 'F';
	}
	ret += "}]";
	return ret;
}

} /* Namespace automata */
