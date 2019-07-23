/**
 * Tool to convert BCG automata to .tra/.lab files for (I)MRMC.
 *
 * Author: Enno Ruijters (2019), University of Twente
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "bcg_user.h"
#include "../dft2lnt/decnumber.h"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cinttypes>
#include <map>
#include <unordered_set>
#include <vector>

typedef std::map<BCG_TYPE_STATE_NUMBER, BCG_TYPE_STATE_NUMBER> stateStateMap;

static void invalid_label()
{
	fprintf(stderr, "Error: State reachable both from repair and fail transitions.\n");
	exit(EXIT_FAILURE);
}

static std::vector<decnumber<>> get_times(BCG_TYPE_OBJECT_TRANSITION g,
			std::vector<BCG_TYPE_STATE_NUMBER> stateNums)
{
	std::vector<decnumber<>> ret;
	BCG_TYPE_STATE_NUMBER s1, s2;
	BCG_TYPE_STATE_NUMBER bcg_nb_states;
	bcg_nb_states = BCG_OT_NB_STATES(g);
	for (s1 = 0; s1 < bcg_nb_states; s1++) {
		if (stateNums[s1] == 0)
			continue;
		BCG_TYPE_LABEL_NUMBER labnum;

		BCG_OT_ITERATE_P_LN (g, s1, labnum, s2) {
			bool found = 0;
			BCG_TYPE_C_STRING nm;
			nm = BCG_OT_LABEL_STRING(g, labnum);
			if (strncmp(nm, "time ", 5))
				continue;
			decnumber<> t(nm + 5);
			for (decnumber<> s : ret) {
				if (s == t) {
					found = true;
					break;
				}
			}
			if (!found)
				ret.push_back(t);
		} BCG_OT_END_ITERATE;
	}
	return ret;
}

static std::map<decnumber<>, std::vector<decnumber<>>>
		expand_times(const std::vector<decnumber<>> &times)
{
	std::map<decnumber<>, std::vector<decnumber<>>> ret;
	if (times.empty())
		return ret;
	std::vector<uintmax_t> n;
	bool done = false;
	for (size_t i = times.size(); i > 0; i--)
		n.push_back(0);
	decnumber<> time(0);

	while (!done) {
		std::vector<decnumber<>> thisTime;
		done = true;
		time = times[0] * (n[0] + 1);
		thisTime.push_back(times[0]);
		for (size_t i = times.size() - 1; i > 0; i--) {
			decnumber<> t = times[i] * (n[i] + 1);
			if (t < time) {
				done = false;
				thisTime = std::vector<decnumber<>>();
				time = t;
				thisTime.push_back(times[i]);
			} else if (t == time) {
				thisTime.push_back(times[i]);
			} else {
				done = false;
			}
		}
		for (size_t i = 0; i < times.size(); i++) {
			if (times[i] * (n[i] + 1) == time)
				n[i]++;
		}
		ret[time] = thisTime;
	}
	return ret;
}

BCG_TYPE_STATE_NUMBER get_timed_target(BCG_TYPE_OBJECT_TRANSITION g,
                                       decnumber<> t,
                                       BCG_TYPE_STATE_NUMBER from)
{
	BCG_TYPE_STATE_NUMBER to;
	BCG_TYPE_LABEL_NUMBER labnum;
	BCG_OT_ITERATE_P_LN (g, from, labnum, to) {
		BCG_TYPE_C_STRING label;
		label = BCG_OT_LABEL_STRING(g, labnum);
		if (!strncmp(label, "time ", 5)) {
			decnumber<> v(label + 5);
			if (v == t)
				return to;
		}
	} BCG_OT_END_ITERATE;
	return from;
}

static BCG_TYPE_STATE_NUMBER get_eventual_target(BCG_TYPE_OBJECT_TRANSITION g,
                                                 std::vector<decnumber<>> ts,
                                                 BCG_TYPE_STATE_NUMBER from)
{
	if (ts.empty())
		return from;
	size_t imm = get_timed_target(g, ts[0], from);
	if (ts.size() == 1)
		return imm;
	std::vector<decnumber<>> rest = ts;
	rest.erase(rest.begin());
	BCG_TYPE_STATE_NUMBER claim = get_eventual_target(g, rest, imm);
	for (size_t i = 1; i < ts.size(); i++) {
		imm = get_timed_target(g, ts[i], from);
		rest = ts;
		rest.erase(rest.begin() + i);
		BCG_TYPE_STATE_NUMBER fin = get_eventual_target(g, rest, imm);
		if (fin != claim)
			fprintf(stderr, "Non-spurious nondeterminism in equal-timed transitions at time %s from state %zu.\n", ts[i].str().c_str(), (size_t)from);
	}
	return claim;
}

std::vector<BCG_TYPE_STATE_NUMBER>
	get_timed_transitions(BCG_TYPE_OBJECT_TRANSITION g,
	                      std::map<decnumber<>, std::vector<decnumber<>>> ts,
	                      BCG_TYPE_STATE_NUMBER from)
{
	std::vector<BCG_TYPE_STATE_NUMBER> ret;
	for (auto tl : ts) {
		std::vector<decnumber<>> times = tl.second;
		size_t tgt = get_eventual_target(g, times, from);
		ret.push_back(tgt);
	}
	return ret;
}

static std::map<BCG_TYPE_STATE_NUMBER, std::string>
	get_rate_targets(BCG_TYPE_OBJECT_TRANSITION g,
	                 std::vector<BCG_TYPE_STATE_NUMBER> timed,
			 stateStateMap renames,
                         BCG_TYPE_STATE_NUMBER from)
{
	std::map<BCG_TYPE_STATE_NUMBER, std::string> ret;
	BCG_TYPE_STATE_NUMBER to;
	BCG_TYPE_LABEL_NUMBER labnum;
	BCG_OT_ITERATE_P_LN (g, from, labnum, to) {
		BCG_TYPE_C_STRING label;
		label = BCG_OT_LABEL_STRING(g, labnum);
		if (!strncmp(label, "rate ", 5)) {
			auto ren = renames.find(to);
			if (ren != renames.end())
				to = ren->second;
			ret[to] = std::string(label + 5);
		}
	} BCG_OT_END_ITERATE;
	for (BCG_TYPE_STATE_NUMBER n : timed) {
		if (ret.find(n) == ret.end())
			ret[n] = "0";
	}
	return ret;
}

int main(int argc, char* argv[])
{
	BCG_TYPE_OBJECT_TRANSITION bcg_graph;
	BCG_TYPE_STATE_NUMBER bcg_s1;
	BCG_TYPE_STATE_NUMBER bcg_nb_states;
	BCG_TYPE_EDGE_NUMBER bcg_nb_edges;
	int edge_count_chars;

	char *faillabel, *repairlabel = NULL;

	if (argc < 3) {
		fprintf(stderr, "Usage: bcg2tralab <input.bcg> <output_prefix> <failure transition> [<repair transition>]\n");
		return EXIT_FAILURE;
	}
	faillabel = argv[3];
	if (argc > 3)
		repairlabel = argv[4];

	BCG_INIT();

	char *filename;
	filename = (char *)malloc(strlen(argv[2]) + 5);
	if (!filename) {
		fprintf(stderr, "Out of memory allocating file names.");
		return EXIT_FAILURE;
	}
	strcpy(filename, argv[2]);
	strcat(filename, ".tra");
	FILE *tra, *lab;
	tra = fopen(filename, "w");
	if (!tra) {
		fprintf(stderr, "Unable to open '%s'\n", filename);
		return EXIT_FAILURE;
	}
	strcpy(filename, argv[2]);
	strcat(filename, ".lab");
	lab = fopen(filename, "w");
	if (!lab) {
		fprintf(stderr, "Unable to open '%s'\n", filename);
		return EXIT_FAILURE;
	}
	BCG_OT_READ_BCG_BEGIN(argv[1], &bcg_graph, 1);

	/* Meaning of state labels:
	 * 0  - Unknown
	 * 1  - Certainly not failed but not yet propagated
	 * 2  - Certainly not failed and propagated
	 * -1 - Certainly failed but not yet propagated
	 * -2 - Certainly failed and propagated
	 */
	bcg_nb_states = BCG_OT_NB_STATES(bcg_graph);
	bcg_nb_edges = BCG_OT_NB_EDGES(bcg_graph);
	std::vector<signed char> state_labels (bcg_nb_states);

	if (BCG_OT_INITIAL_STATE(bcg_graph) != 0) {
		fprintf(stderr, "Initial state non-zero\n");
		return EXIT_FAILURE;
	}
	state_labels[0] = 1; /* Initial state is certainly not failed. */

	/* First, identify all reachable states. */
	std::unordered_set<BCG_TYPE_STATE_NUMBER> reachable;
	std::unordered_set<BCG_TYPE_STATE_NUMBER> reachable_queue;
	/* timed_maxprog contains states that are reachable only from
	 * Markovian transitions, i.e. states from which interactive
	 * transitions override timed transitions.
	 */
	std::unordered_set<BCG_TYPE_STATE_NUMBER> timed_maxprog;

	/* Renames: targets of interactive transitions */
	stateStateMap renames;
	reachable.insert(0);
	reachable_queue.insert(0);
	while (!reachable_queue.empty()) {
		BCG_TYPE_STATE_NUMBER s2, s1 = *reachable_queue.begin();
		BCG_TYPE_STATE_NUMBER interactive_target = s1;
		BCG_TYPE_LABEL_NUMBER labnum;
		int target_label = 0;
		reachable_queue.erase(s1);

		/* If any interactive transitions exist, maximal
		 * progress means the remaining Markovian transitions
		 * cannot be taken.
		 */
		BCG_OT_ITERATE_P_LN (bcg_graph, s1, labnum, s2) {
			BCG_TYPE_C_STRING label;
			label = BCG_OT_LABEL_STRING(bcg_graph, labnum);
			if (!strncmp(label, "rate ", 5)) {
				if (reachable.insert(s2).second)
					reachable_queue.insert(s2);
			} else if (!strncmp(label, "time ", 5)) {
				if (reachable.insert(s2).second)
					reachable_queue.insert(s2);
			} else if (interactive_target != s1) {
				fprintf(stderr, "Model has nondeterminism in state %zu, aborting.\n", (size_t)s1);
				return EXIT_FAILURE;
			} else if (!strcmp(label, faillabel)) {
				interactive_target = s2;
				target_label = -1;
			} else if (repairlabel && !strcmp(label, repairlabel)) {
				interactive_target = s2;
				target_label = 1;
			} else if (!strcmp(label, "i")) {
				interactive_target = s2;
			} else {
				fprintf(stderr, "Warning: unknown transition label '%s', assuming no effect on labeling\n", label);
				interactive_target = s2;
			}
		} BCG_OT_END_ITERATE;
		if (interactive_target != s1) {
			renames[s1] = interactive_target;
			if (state_labels[interactive_target] != target_label
			    && state_labels[interactive_target] != 0)
			{
				fprintf(stderr, "Error: State %zu reachable from interactive transition from state %zu resulting in different label %d.", (std::size_t) interactive_target, (std::size_t)s1, target_label);
				return EXIT_FAILURE;
			}
			state_labels[interactive_target] = target_label;
			if (reachable.insert(interactive_target).second)
				reachable_queue.insert(interactive_target);
			continue;
		}
	}

	/* Next, collapse any chains of renames. */
	bool done = 0;
	while (!done) {
		done = 1;
		for (auto it = renames.begin(); it != renames.end(); ++it)
		{
			BCG_TYPE_STATE_NUMBER src = it->first;
			BCG_TYPE_STATE_NUMBER tgt = it->second;
			stateStateMap::iterator tgtIt = renames.find(tgt);
			if (tgtIt != renames.end()) {
				done = 0;
				BCG_TYPE_STATE_NUMBER newTgt = tgtIt->second;
				if (state_labels[tgt] != state_labels[newTgt])
					invalid_label();
				renames[src] = newTgt;
				it = renames.begin();
			}
		}
	}

	/* Any renamed states are, by definition, no longer reachable */
	for (auto it = renames.begin(); it != renames.end(); ++it)
		reachable.erase(it->first);
	if (reachable.find(0) == reachable.end()) {
		fprintf(stderr, "Initial state is interactive, not supported.\n");
		return EXIT_FAILURE;
	}

	/* Renumber the reachable states consecutively */
	std::vector<BCG_TYPE_STATE_NUMBER> stateNums;
	size_t i = 1;
	for (bcg_s1 = 0; bcg_s1 < bcg_nb_states; bcg_s1++) {
		if (reachable.find(bcg_s1) != reachable.end()) {
			stateNums.push_back(i++);
		} else
			stateNums.push_back(0);
	}
	std::vector<decnumber<>> times = get_times(bcg_graph, stateNums);
	auto allTimes = expand_times(times);

	/* Header information */
	if (times.size() > 0) {
		fprintf(tra, "MODES");
		for (size_t i = allTimes.size(); i > 0; i--)
			fprintf(tra, " ctmc dtmc");
		fprintf(tra, "\n");
		fprintf(tra, "TIMES");
		decnumber<> prev(0);
		for (auto time : allTimes) {
			decnumber<> t = time.first - prev;
			fprintf(tra, " %s 1", t.str().c_str());
			prev = time.first;
		}
		fprintf(tra, "\n");
	}
	fprintf(tra, "STATES %zu\n", reachable.size());
	fprintf(tra, "TRANSITIONS ");
	long transition_count_pos = ftell(tra);
	fprintf(tra, "%llu     %n\n", (unsigned long long) bcg_nb_edges, &edge_count_chars);
	fprintf(lab, "#DECLARATION\nmarked\n#END\n");

	unsigned long long n_transitions = 0;
	for (bcg_s1 = 0; bcg_s1 < bcg_nb_states; bcg_s1++) {
		BCG_TYPE_STATE_NUMBER s1 = stateNums[bcg_s1];
		if (s1 == 0)
			continue;

		std::vector<BCG_TYPE_STATE_NUMBER> ttgts
			= get_timed_transitions(bcg_graph, allTimes, bcg_s1);
		for (size_t i = 0; i < ttgts.size(); i++) {
			auto rename = renames.find(ttgts[i]);
			if (rename != renames.end()) {
				ttgts[i] = rename->second;
			}
		}
		std::map<BCG_TYPE_STATE_NUMBER, std::string> tgts;
		tgts = get_rate_targets(bcg_graph, ttgts, renames, bcg_s1);
		for (auto pair : tgts) {
			BCG_TYPE_STATE_NUMBER s2 = pair.first;
			BCG_TYPE_STATE_NUMBER t = stateNums[s2];
			if (t == 0) {
				fprintf(stderr, "Error: Unreachable state %zu reached from %zu via timed transition.\n", (std::size_t)s2, (size_t)bcg_s1);
				return EXIT_FAILURE;
			}
			const char *rate = pair.second.c_str();
			fprintf(tra, "%zu %zu %s", s1, t, rate);
			auto rename = renames.find(s2);
			if (rename != renames.end())
				s2 = rename->second;
			for (size_t i = 0; i < ttgts.size(); i++) {
				if (ttgts[i] == s2)
					fprintf(tra, " 1");
				else
					fprintf(tra, " 0");
				if (i != ttgts.size() - 1)
					fprintf(tra, " %s", rate);
			}
			fprintf(tra, "\n");
			n_transitions++;
		}
	}
	char fstring[6+(sizeof(edge_count_chars)*CHAR_BIT)/3+1];
	sprintf(fstring, "%%%dllu", edge_count_chars);
	fseek(tra, transition_count_pos, SEEK_SET);
	fprintf(tra, fstring, n_transitions);
	fclose(tra);

	/* Calculating labels. Definition: Anything that is reachable by
	 * Markovian transitions from a failed state is also failed,
	 * unless the target state is interactively renamed a repaired
	 * state. Similarly, anything that is reachable by Markovian
	 * transitions from a non-failed state is non-failed unless the
	 * target is renamed by a fail transition. Conflicts between
	 * these rules indicate a serious modeling error.
	 */
	done = 0;
	while (!done) {
		done = 1;
		for (auto it = reachable.begin(); it != reachable.end(); ++it) {
			BCG_TYPE_STATE_NUMBER s1 = *it;
			if (state_labels[s1] == 0) {
				done = 0;
				continue;
			}
			if (state_labels[s1] != -1 && state_labels[s1] != 1)
				continue;

			signed char state_label = state_labels[s1];
			BCG_TYPE_LABEL_NUMBER labnum;
			BCG_TYPE_STATE_NUMBER s2;
			BCG_OT_ITERATE_P_LN (bcg_graph, s1, labnum, s2) {
				BCG_TYPE_C_STRING nm;
				nm = BCG_OT_LABEL_STRING(bcg_graph, labnum);
				if (!strncmp(nm, "time ", 5))
					continue;
				if (strncmp(nm, "rate ", 5)) {
					fprintf(stderr, "Reachable interactive state %zu (label %s), should have been caught earlier.\n", (size_t)s1, nm);
					return EXIT_FAILURE;
				}
				if (state_label == 1) {
					if (state_labels[s2] == 0)
						state_labels[s2] = 1;
					else if (state_labels[s2] < 0)
						invalid_label();
				} else if (state_label == -1) {
					if (state_labels[s2] == 0)
						state_labels[s2] = -1;
					else if (state_labels[s2] > 0)
						invalid_label();
				}
			} BCG_OT_END_ITERATE;
			state_labels[s1] *= 2;
		}
	}

	BCG_OT_READ_BCG_END (&bcg_graph);

	for (auto it = reachable.begin(); it != reachable.end(); ++it) {
		BCG_TYPE_STATE_NUMBER s1 = *it;
		if (state_labels[s1] < 0)
			fprintf(lab, "%zu marked\n", stateNums[s1]);
	}
	fclose(lab);
	return 0;
}
