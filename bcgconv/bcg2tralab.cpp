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
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cinttypes>
#include <map>
#include <unordered_set>
#include <vector>

static void invalid_label()
{
	fprintf(stderr, "Error: State reachable both from repair and fail transitions.\n");
	exit(EXIT_FAILURE);
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
		fprintf(stderr, "Initial state non-zeron\n");
		return EXIT_FAILURE;
	}
	state_labels[0] = 1; /* Initial state is certainly not failed. */

	/* First, identify all reachable states. */
	std::unordered_set<BCG_TYPE_STATE_NUMBER> reachable;
	std::unordered_set<BCG_TYPE_STATE_NUMBER> reachable_queue;

	/* Renames: targets of interactive transitions */
	typedef std::map<BCG_TYPE_STATE_NUMBER, BCG_TYPE_STATE_NUMBER> stateStateMap;
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
				; /* Do nothing */
			} else if (interactive_target != s1) {
				fprintf(stderr, "Model has nondeterminism, aborting.\n");
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
			printf("Renaming %zu to %zu\n", (std::size_t)s1, (std::size_t)interactive_target);
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
		/* We are in a Markovian state or the block above would
		 * have skipped this part.
		 */
		BCG_OT_ITERATE_P_LN (bcg_graph, s1, labnum, s2) {
			if (reachable.insert(s2).second)
				reachable_queue.insert(s2);
		} BCG_OT_END_ITERATE;
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
				printf("Renaming %zu to %zu\n", (std::size_t)src, (std::size_t)newTgt);
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
	std::vector<std::size_t> stateNums;
	size_t i = 1;
	for (bcg_s1 = 0; bcg_s1 < bcg_nb_states; bcg_s1++) {
		if (reachable.find(bcg_s1) != reachable.end()) {
			printf("Numbering %zu to %zu labeled %d\n", (std::size_t)bcg_s1, i, state_labels[bcg_s1]);
			stateNums.push_back(i++);
		} else
			stateNums.push_back(0);
	}

	/* Header information */
	fprintf(tra, "STATES %zu\n", reachable.size());
	fprintf(tra, "TRANSITIONS ");
	long transition_count_pos = ftell(tra);
	fprintf(tra, "%llu%n\n", (unsigned long long) bcg_nb_edges, &edge_count_chars);
	fprintf(lab, "#DECLARATION\nfailed\n#END\n");

	unsigned long long n_transitions = 0;
	for (bcg_s1 = 0; bcg_s1 < bcg_nb_states; bcg_s1++) {
		std::size_t s1 = stateNums[bcg_s1];
		if (s1 == 0)
			continue;
		BCG_TYPE_LABEL_NUMBER labnum;
		BCG_TYPE_STATE_NUMBER s2;

		BCG_OT_ITERATE_P_LN (bcg_graph, bcg_s1, labnum, s2) {
			auto rename = renames.find(s2);
			if (rename != renames.end())
				s2 = rename->second;
			BCG_TYPE_C_STRING nm;
			nm = BCG_OT_LABEL_STRING(bcg_graph, labnum);
			std::size_t t = stateNums[s2];
			if (t == 0) {
				fprintf(stderr, "Error: Unreachable state %zu reached.\n", (std::size_t)s2);
				return EXIT_FAILURE;
			}
			if (strncmp(nm, "rate ", 5)) {
				fprintf(stderr, "Error: Interactive state left after Markovianization.");
				return EXIT_FAILURE;
			}
			fprintf(tra, "%zu %zu %s\n", s1, t, nm + 5);
			n_transitions++;
		} BCG_OT_END_ITERATE;
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
				if (strncmp(nm, "rate ", 5)) {
					fprintf(stderr, "Reachable interactive state, should have been caught earlier.\n");
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
			state_labels[bcg_s1] *= 2;
		}
	}

	BCG_OT_READ_BCG_END (&bcg_graph);

	for (auto it = reachable.begin(); it != reachable.end(); ++it) {
		BCG_TYPE_STATE_NUMBER s1 = *it;
		if (state_labels[s1] < 0)
			fprintf(lab, "%zu failed\n", stateNums[s1]);
	}
	fclose(lab);
	return 0;
}
