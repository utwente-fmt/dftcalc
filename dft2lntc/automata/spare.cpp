#include "automata/voting.h"

namespace automata {
using namespace signals;

void spare::spare_state::initialize_outgoing() {
	if (terminated)
		return;

	spare_state target(*this);
	if (impossible) {
		target.terminated = 1;
		add_transition(GATE_IMPOSSIBLE, target);
		return;
	}
	size_t nr_act, i;
	const spare *par = (const spare *)get_parent();
	set<size_t> avail;
	for (size_t j : unfailed) {
		if (unclaimed.find(j) != unclaimed.end())
			avail.insert(j);
	}

	for (i = 1; i <= par->total; i++) {
		target = *this;
		target.unfailed.erase(i);
		if (i == cur_using)
			target.prev_using = cur_using;
		add_transition(FAIL(i), target);
	}

	if (prev_using) {
		target = *this;
		target.prev_using = 0;
		target.cur_using = 0;
		add_transition(DEACTIVATE(prev_using, true), target);
	} else if (repairing_deactivate) {
		target = *this;
		target.repairing_deactivate = 0;
		add_transition(DEACTIVATE(repairing_deactivate, true), target);
	} else if (!done && avail.empty()) {
		target = *this;
		target.done = true;
		add_transition(FAIL(0), target);
	}

	for (i = 1; i <= par->total; i++) {
		target = *this;
		if (!par->repairable) {
			target.impossible = 1;
		} else {
			target.unfailed.insert(i);
		}
		add_transition(ONLINE(i), target);
	}

	if (cur_using && avail.find(cur_using) != avail.end() && done) {
		target = *this;
		target.done = false;
		add_transition(ONLINE(0), target);
	}

	if (done && !avail.empty() && !activated) {
		target = *this;
		target.done = false;
		add_transition(ONLINE(0), target);
	}

	target = *this;
	target.activated = true;
	add_transition(ACTIVATE(0, false), target);

	target.activated = false;
	target.prev_using = cur_using;
	if (par->always_active)
		target.impossible = true;
	add_transition(DEACTIVATE(0, false), target);

	for (i = 1; i < par->total; i++) {
		if (i == cur_using)
			continue;
		target = *this;
		if (i == 1 && par->always_active)
			target.impossible = 1;
		target.unclaimed.erase(i);
		add_transition(ACTIVATE(i, false), target);
	}

	for (i = 1; i < par->total; i++) {
		target = *this;
		if (i == 1 && par->always_active)
			target.impossible = 1;
		if (i == prev_using)
			target.impossible = 1;
		if (i == repairing_deactivate)
			target.impossible = 1;
		target.unclaimed.insert(i);
		add_transition(DEACTIVATE(i, false), target);
	}

	if (!cur_using && !avail.empty()) {
		if (activated) {
			target = *this;
			target.cur_using = *avail.begin();
			if (repairing_deactivate == target.cur_using)
				target.repairing_deactivate = 0;
			add_transition(ACTIVATE(target.cur_using, true), target);
		} else if (repairing_deactivate == target.cur_using) {
			target = *this;
			target.repairing_deactivate = 0;
			add_transition("", target);
		}
	}

	add_transition(REPAIRED(0), *this);

	if (activated && !(avail.empty() || prev_using && repairing_deactivate))
	{
		size_t new_using = *avail.begin();
		if (new_using != cur_using) {
			target = *this;
			target.repairing_deactivate = cur_using;
			target.cur_using = new_using;
			if (new_using > 1 || !par->always_active)
				add_transition(ACTIVATE(new_using, true), target);
			else
				add_transition("", target);
		}
	}
}

void spare::spare_state::add_transition(std::string label, spare_state &target)
{
	const spare *par = (const spare *)get_parent();
	if (par->always_active) {
		set<size_t> avail;
		for (size_t i : target.unfailed) {
			if (target.unclaimed.find(i) != target.unclaimed.end())
				avail.insert(i);
		}
		if (target.cur_using != 1 && avail.find(1) != avail.end()) {
			target.prev_using = target.cur_using;
			target.cur_using = 1;
			if (target.repairing_deactivate == 1)
				target.repairing_deactivate = 0;
		}
		if (target.prev_using == 1) {
			target.prev_using = target.cur_using = 0;
		}
	}
	state::add_transition(label, target);
}

spare::spare_state::operator std::string() const {
	if (terminated)
		return "[]";
	if (impossible)
		return "[IMPOSSIBLE]";
			set<size_t> unfailed;
			set<size_t> unclaimed;
	std::string ret = "[";
	ret += std::to_string(cur_using);
	if (done)
		ret += ", done";
	if (activated)
		ret += ", active";
	ret += ", ";
	ret += prev_using;
	ret += ", ";
	ret += repairing_deactivate;
	ret += ", {";
	bool first = true;
	for (size_t i : unfailed) {
		if (!first)
			ret += ", ";
		first = false;
		ret += std::to_string(i);
	}
	ret += "}, {";
	first = true;
	for (size_t i : unclaimed) {
		if (!first)
			ret += ", ";
		first = false;
		ret += std::to_string(i);
	}
	ret += "}]";
	return ret;
}

} /* Namespace automata */
