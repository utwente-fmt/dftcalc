#include "automata/voting.h"

namespace automata {
using namespace signals;

void voting::voting_state::initialize_outgoing() {
	if (terminated)
		return;

	voting_state target(*this);
	if (impossible) {
		target.terminated = 1;
		add_transition(GATE_IMPOSSIBLE, target);
		return;
	}
	size_t nr_act, i;
	const voting *par = (const voting *)get_parent();

	if (running) {
		nr_act = 1;
		while (nr_act > 0 && activated[nr_act]) {
			if (nr_act == par->total)
				nr_act = 0;
			else
				nr_act++;
		}
	} else {
		nr_act = par->total;
		while (nr_act > 0 && !activated[nr_act])
			nr_act--;
	}

	for (i = 1; i <= par->total; i++) {
		target = *this;
		if (!received[i]) {
			target.received[i] = 1;
			target.nr_failed++;
		}
		add_transition(FAIL(i), target);
	}

	if (nr_failed >= par->threshold && !done) {
		target = *this;
		target.done = 1;
		add_transition(FAIL(0), target);
	}

	for (i = 1; i <= par->total; i++) {
		target = *this;
		if (i > par->repairable || !received[i]) {
			target.impossible = 1;
		} else {
			target.received[i] = 0;
			target.nr_failed--;
		}
		add_transition(ONLINE(i), target);
	}

	if (done && nr_failed < par->threshold) {
		target = *this;
		target.done = 0;
		add_transition(ONLINE(0), target);
	}

	if (!par->always_active) {
		target = *this;
		target.running = true;
		add_transition(ACTIVATE(0, false), target);
	}

	target = *this;
	if (!par->always_active) {
		target.running = false;
	} else {
		target.impossible = true;
	}
	add_transition(DEACTIVATE(0, false), target);

	if (running && nr_act) {
		target = *this;
		target.activated[nr_act] = 1;
		add_transition(ACTIVATE(nr_act, true), target);
	}

	if (!running && nr_act) {
		target = *this;
		target.activated[nr_act] = 0;
		add_transition(DEACTIVATE(nr_act, true), target);
	}
}

voting::voting_state::operator std::string() const {
	if (terminated)
		return "[]";
	if (impossible)
		return "[IMPOSSIBLE]";
	std::string ret = "[";
	ret += std::to_string(nr_failed);
	if (done)
		ret += ", done";
	if (running)
		ret += ", running";
	ret += ", {";
	for (size_t i = 0; i < received.size(); i++) {
		if (!received[i] && !activated[i])
			ret += 'I';
		if (!received[i] && activated[i])
			ret += 'A';
		if (received[i] && !activated[i])
			ret += 'F';
		if (!received[i] && !activated[i])
			ret += 'f';
	}
	ret += "}]";
	return ret;
}

} /* Namespace automata */
