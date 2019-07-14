#include "automata/automaton.h"

void automaton::write(std::ostream &out)
{
	const state *init = initial_state();
	add_state(init->copy());
	size_t pos = 0;
	size_t n_transitions = 0;
	while (pos < states.size()) {
		state *s = states[pos];
		s->initialize_outgoing();
		pos++;
	}
	tau_collapse();
	for (const state *s : states)
		n_transitions += s->outgoing.size();
	out << "des (0, " << n_transitions << ", " << states.size()
	<< ")\n";
	for (pos = 0; pos < states.size(); pos++) {
		for (auto transition : states[pos]->outgoing) {
			out << "(" << pos << ", \""
			<< transition.first << "\", "
			<< transition.second << ") "
			//<< (std::string) *states[pos]
			<< "\n";
		}
	}
}

static void merge(std::multimap<std::string, size_t> &target,
                  const std::multimap<std::string, size_t> &source)
{
	for (auto &entry : source) {
		auto it = target.find(entry.first);
		bool needs_add = true;
		while (it != target.end() && it->first == entry.first) {
			if (it->second == entry.second) {
				needs_add = false;
				break;
			}
			++it;
		}
		if (needs_add)
			target.emplace(entry.first, entry.second);
	}
}

void automaton::tau_collapse()
{
	for (state *s : states) {
		auto it = s->outgoing.begin();
		for (; it != s->outgoing.end(); ++it) {
			if (it->first == "") {
				size_t t = it->second;
				s->outgoing.erase(it);
				merge(s->outgoing, states[t]->outgoing);
				it = s->outgoing.begin();
			}
		}
	}
	std::vector<bool> reachable(states.size());
	reachable[0] = true;
	bool change = true;
	while (change) {
		change = false;
		for (size_t i = 0; i < states.size(); i++) {
			if (!reachable[i])
				continue;
			for (auto &trans : states[i]->outgoing) {
				if (!reachable[trans.second]) {
					change = true;
					reachable[trans.second] = true;
				}
			}
		}
	}
	for (size_t i = 0; i < states.size(); i++) {
		if (!reachable[i])
			states[i]->outgoing.clear();
	}
}
