/**
 * Tool to apply maximal progress to BCG files.
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

FILE *aut;
#ifdef USE_BCG
void write_transition(BCG_TYPE_STATE_NUMBER from, BCG_TYPE_C_STRING label,
                      BCG_TYPE_STATE_NUMBER to)
{
	BCG_IO_WRITE_BCG_EDGE(from, label, to);
}
#else
static size_t n_edges;
void write_transition(BCG_TYPE_STATE_NUMBER from, BCG_TYPE_C_STRING label,
                      BCG_TYPE_STATE_NUMBER to)
{
	n_edges++;
	fprintf(aut, "(%zu, \"%s\", %zu)\n", (size_t)from, label, (size_t)to);
}
#endif

typedef std::map<BCG_TYPE_STATE_NUMBER, BCG_TYPE_STATE_NUMBER> stateStateMap;

int main(int argc, char* argv[])
{
	BCG_TYPE_OBJECT_TRANSITION bcg_in;
	BCG_TYPE_STATE_NUMBER bcg_s1;
	BCG_TYPE_STATE_NUMBER bcg_nb_states;

	if (argc < 2) {
		fprintf(stderr, "Usage: maxprog <input.bcg> <output.aut>\n");
		return EXIT_FAILURE;
	}

	BCG_INIT();

	BCG_OT_READ_BCG_BEGIN(argv[1], &bcg_in, 1);
	bcg_nb_states = BCG_OT_NB_STATES(bcg_in);
#ifdef USE_BCG
	BCG_IO_WRITE_BCG_BEGIN(argv[2], 0, 2, NULL, BCG_FALSE);
#else
	int n_header_bytes;
	size_t n_states = 0;
	aut = fopen(argv[2], "w");
	n_header_bytes = fprintf(aut, "des (0, %zu, %zu)\n",
	                         (size_t)BCG_OT_NB_EDGES(bcg_in),
	                         (size_t)bcg_nb_states) - 1;
#endif

	/* First, identify all reachable states. */
	std::unordered_set<BCG_TYPE_STATE_NUMBER> reachable;
	std::unordered_set<BCG_TYPE_STATE_NUMBER> reachable_queue;
	/* timed_maxprog contains states that are reachable at
	 * specifix times, i.e. states from which interactive
	 * transitions do not always override timed transitions.
	 */
	std::unordered_set<BCG_TYPE_STATE_NUMBER> timed_reachable;

	/* Renames: targets of interactive transitions */
	stateStateMap renames;
	reachable.insert(0);
	reachable_queue.insert(0);
	timed_reachable.insert(0);
	while (!reachable_queue.empty()) {
		BCG_TYPE_STATE_NUMBER s2, s1 = *reachable_queue.begin();
		BCG_TYPE_LABEL_NUMBER labnum;
		bool timed = false, timedreach, interactive = false;
		reachable_queue.erase(s1);
		timedreach = (timed_reachable.find(s1)!=timed_reachable.end());

		/* If any interactive transitions exist, maximal
		 * progress means the remaining Markovian transitions
		 * cannot be taken.
		 */
		BCG_OT_ITERATE_P_LN (bcg_in, s1, labnum, s2) {
			BCG_TYPE_C_STRING label;
			label = BCG_OT_LABEL_STRING(bcg_in, labnum);
			if (!strncmp(label, "rate ", 5))
				; /* Do nothing */
			else if (!strncmp(label, "time ", 5))
				timed = true;
			else {
				interactive = true;
				if (reachable.insert(s2).second)
					reachable_queue.insert(s2);
				if (timedreach && timed_reachable.insert(s2).second)
					reachable_queue.insert(s2);
			}
		} BCG_OT_END_ITERATE;
		if (interactive && !(timed && timedreach))
			continue;
		BCG_OT_ITERATE_P_LN (bcg_in, s1, labnum, s2) {
			BCG_TYPE_C_STRING label;
			label = BCG_OT_LABEL_STRING(bcg_in, labnum);
			if (!interactive && !strncmp(label, "rate ", 5)) {
				if (reachable.insert(s2).second)
					reachable_queue.insert(s2);
			} else if (!strncmp(label, "time ", 5)) {
				if (reachable.insert(s2).second)
					reachable_queue.insert(s2);
				if (timed_reachable.insert(s2).second)
					reachable_queue.insert(s2);
			}
		} BCG_OT_END_ITERATE;
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
#ifndef USE_BCG
	n_states = i - 1;
#endif

	for (bcg_s1 = 0; bcg_s1 < bcg_nb_states; bcg_s1++) {
		BCG_TYPE_STATE_NUMBER s1 = stateNums[bcg_s1];
		if (s1 == 0)
			continue;
		s1--;

		BCG_TYPE_STATE_NUMBER s2;
		BCG_TYPE_LABEL_NUMBER labnum;
		bool timedreach, interactive = false;
		timedreach = (timed_reachable.find(bcg_s1)!=timed_reachable.end());

		BCG_OT_ITERATE_P_LN (bcg_in, bcg_s1, labnum, s2) {
			BCG_TYPE_C_STRING label;
			label = BCG_OT_LABEL_STRING(bcg_in, labnum);
			if (s2 == bcg_s1)
				continue;
			s2 = stateNums[s2] - 1;
			if (!strncmp(label, "rate ", 5))
				; /* Do nothing */
			else if (timedreach || strncmp(label, "time ", 5)) {
				write_transition(s1, label, s2);
				if (strncmp(label, "time ", 5))
					interactive = true;
			}
		} BCG_OT_END_ITERATE;
		if (interactive)
			continue;
		BCG_OT_ITERATE_P_LN (bcg_in, bcg_s1, labnum, s2) {
			BCG_TYPE_C_STRING label;
			label = BCG_OT_LABEL_STRING(bcg_in, labnum);
			if (s2 == bcg_s1)
				continue;
			s2 = stateNums[s2] - 1;
			if (!strncmp(label, "rate ", 5)) {
				write_transition(s1, label, s2);
			} else if (!timedreach && !strncmp(label, "time ", 5)) {
				write_transition(s1, label, s2);
			}
		} BCG_OT_END_ITERATE;
	}
#ifdef USE_BCG
	BCG_IO_WRITE_BCG_END();
#else
	fseek(aut, 0, SEEK_SET);
	int new_header_bytes = fprintf(aut, "des (0, %zu, %zu)",
	                               (size_t)n_edges, (size_t)n_states);
	while (new_header_bytes < n_header_bytes) {
		fprintf(aut, " ");
		new_header_bytes++;
	}
	fprintf(aut, "\n");
	fclose(aut);
#endif
	return 0;
}
