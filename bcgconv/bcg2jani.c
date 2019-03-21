/**
 * Tool to convert BCG automata to MDPs in the JANI format.
 *
 * Author: Enno Ruijters (2017), University of Twente
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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

/* A collapsible state is a state that has exactly one interactive
 * transition (and can therefore be collapsed if it is reached after a
 * Markovian, non-assigning transition).
 * The structure also has the state number of the target of the
 * interactive transition, and the assignment (if any).
 */
struct collapsible {
	BCG_TYPE_STATE_NUMBER from, to;
	const char *variable;
	int val;
};

static char *faillabel, *repairlabel;

/* Returns a list of states that can be collapsed (i.e. that have
 * exactly one interactive transition).
 */
struct collapsible *find_collapsibles(size_t *n, BCG_TYPE_OBJECT_TRANSITION g,
                                      _Bool *is_ctmc)
{
	BCG_TYPE_STATE_NUMBER bcg_nb_states;
	BCG_TYPE_STATE_NUMBER bcg_s1;
	struct collapsible *ret = NULL;
	*n = 0;
	*is_ctmc = 1;
	bcg_nb_states = BCG_OT_NB_STATES(g);

	for (bcg_s1 = 0; bcg_s1 < bcg_nb_states; bcg_s1++) {
		_Bool collapsible = 0;
		BCG_TYPE_STATE_NUMBER s2;
		BCG_TYPE_LABEL_NUMBER labelnum;

		/* Check if the state has exactly one interactive transition. */
		BCG_OT_ITERATE_P_LN (g, bcg_s1, labelnum, s2) {
			BCG_TYPE_C_STRING labelname;
			labelname = BCG_OT_LABEL_STRING(g, labelnum);
			if (strncmp(labelname, "rate ", 5)) {
				if (!collapsible) {
					collapsible = 1;
				} else {
					*is_ctmc = 0;
					collapsible = 0;
					break;
				}
			}
		} BCG_OT_END_ITERATE;

		if (!collapsible)
			continue;

		BCG_OT_ITERATE_P_LN (g, bcg_s1, labelnum, s2) {
			BCG_TYPE_C_STRING nm;
			nm = BCG_OT_LABEL_STRING(g, labelnum);
			if (!strncmp(nm, "rate ", 5))
				continue;

			ret = realloc(ret, (*n + 1) * sizeof(*ret));
			if (!ret) {
				fprintf(stderr, "Out of memory, not collapsing interactive states.\n");
				*n = 0;
				*is_ctmc = 0;
				return NULL;
			}
			ret[*n].from = bcg_s1;
			ret[*n].to = s2;
			if (!strcmp(nm, faillabel)) {
				ret[*n].variable = "marked";
				ret[*n].val = 1;
			} else if (repairlabel && !strcmp(nm, repairlabel)) {
				ret[*n].variable = "marked";
				ret[*n].val = 0;
			} else {
				ret[*n].variable = NULL;
				fprintf(stderr, "Unknown label: %s\n", nm);
			}
			(*n)++;
		} BCG_OT_END_ITERATE;
	}
	return ret;
}

int main(int argc, char* argv[])
{
	BCG_TYPE_OBJECT_TRANSITION bcg_graph;
	BCG_TYPE_STATE_NUMBER bcg_s1;
	BCG_TYPE_STATE_NUMBER bcg_nb_states;

	if (argc < 3) {
		fprintf(stderr, "Usage: bcg2jani <input.bcg> <output.json> <failure transition> [<repair transition>]\n");
		return EXIT_FAILURE;
	}
	faillabel = argv[3];
	if (argc > 3)
		repairlabel = argv[4];

	BCG_INIT();

	FILE* jani;
	jani = fopen(argv[2], "w");
	BCG_OT_READ_BCG_BEGIN (argv[1], &bcg_graph, 1);

	struct collapsible *collapsible;
	size_t n_collapsibles;
	_Bool is_ctmc;

	collapsible = find_collapsibles(&n_collapsibles, bcg_graph, &is_ctmc);

	/* Header information in JANI file */
	fprintf(jani,
	        "{\t\"jani-version\": 1,\n"
	         "\t\"name\": \"from_%s\",\n", argv[1]);
	fprintf(jani, "\t\"type\": \"%s\",\n", is_ctmc ? "ctmc" : "ma");
	if (!is_ctmc)
		fprintf(jani, "\t\"actions\": [],\n");
	fprintf(jani,
	         "\t\"constants\": [{\"name\": \"T\", "
	                            "\"type\": \"real\"},\n"
	         "\t                {\"name\": \"L\", "
	                            "\"type\": \"real\"}],\n"
	         "\t\"variables\": [{\"name\": \"marked\", "
	                            "\"type\": {\"kind\":\"bounded\",\"base\":\"int\",\"upper-bound\":1}, "
	                            "\"initial-value\": 0}],\n"
	         );

	bcg_nb_states = BCG_OT_NB_STATES(bcg_graph);

	/* Done with constants, moving on to the automaton */
	fprintf(jani, "\t\"automata\": [{\n"
	        "\t\t\"name\": \"aut\",\n"
	        "\t\t\"locations\": [\n");

	/* We use one location per state */
	for (uintmax_t i = 0; i < bcg_nb_states; i++) {
		_Bool write = 1;
		for (size_t j = 0; j < n_collapsibles; j++) {
			if (collapsible[j].from == i) {
				write = 0;
				break;
			} else if (collapsible[j].from > i) {
				break;
			}
		}
		if (i && !write)
			continue;
		fprintf(jani, "\t\t\t%s{\"name\": \"l%"PRIuMAX"\"}\n",
			i ? "," : "", i);
	}
	fprintf(jani, "\t\t],\n"
	              "\t\t\"initial-locations\": [\"l%"PRIuMAX"\"],\n",
		(uintmax_t) BCG_OT_INITIAL_STATE(bcg_graph));
	fprintf(jani, "\t\t\"edges\": [\n");

	_Bool first_transition = 1;
	for (bcg_s1 = 0; bcg_s1 < bcg_nb_states; bcg_s1++) {
		_Bool has_nonmarkov = 0;
		BCG_TYPE_LABEL_NUMBER num_markov = 0, labelnum;
		BCG_TYPE_STATE_NUMBER s2;

		/* First see whether there are any edges. */
		BCG_OT_ITERATE_P_LN (bcg_graph, bcg_s1, labelnum, s2) {
			BCG_TYPE_C_STRING labelname;
			labelname = BCG_OT_LABEL_STRING(bcg_graph, labelnum);
			if (!strncmp(labelname, "rate ", 5))
				num_markov++;
			else
				has_nonmarkov = 1;
		} BCG_OT_END_ITERATE;

		if (!has_nonmarkov && num_markov) {
			BCG_OT_ITERATE_P_LN (bcg_graph, bcg_s1, labelnum, s2) {
				BCG_TYPE_C_STRING nm;
				struct collapsible *c = NULL;
				size_t i;

				nm = BCG_OT_LABEL_STRING(bcg_graph, labelnum);
				if (strncmp(nm, "rate ", 5))
					continue;
				for (i = 0; i < n_collapsibles; i++) {
					if (collapsible[i].from == s2) {
						c = &collapsible[i];
					} else if (collapsible[i].from > s2) {
						break;
					}
				}
				if (!first_transition)
					fprintf(jani, ",\n");
				first_transition = 0;
				if (c)
					s2 = c->to;
				fprintf(jani, "\t\t\t{\"location\": \"l%"PRIuMAX"\",\n"
					"\t\t\t \"rate\": {\"exp\": %s},\n"
					"\t\t\t \"destinations\":[{\"location\":\"l%"PRIuMAX"\"",
					(uintmax_t) bcg_s1, nm+5,
					(uintmax_t) s2);
				if (c == NULL || c->variable == NULL) {
					fprintf(jani, "}]}\n");
				} else {
					fprintf(jani, ", \"assignments\": [{\"ref\": \"marked\", "
					                          "\"value\": %d}]}]}\n", c->val);
				}
			} BCG_OT_END_ITERATE;
		}

		for (size_t i = 0; i < n_collapsibles; i++) {
			if (collapsible[i].from == bcg_s1) {
				has_nonmarkov = 0;
			} else if (collapsible[i].from > bcg_s1) {
				break;
			}
		}

		if (has_nonmarkov) {
			BCG_OT_ITERATE_P_LN (bcg_graph, bcg_s1, labelnum, s2) {
				BCG_TYPE_C_STRING nm;
				nm = BCG_OT_LABEL_STRING(bcg_graph, labelnum);
				if (!strncmp(nm, "rate ", 5))
					continue;

				if (!first_transition)
					fprintf(jani, ",\n");
				first_transition = 0;
				fprintf(jani,
				        "\t\t\t{\"location\":\"l%"PRIuMAX"\",\n"
			                "\t\t\t \"destinations\": [\n"
				        "\t\t\t\t{\"location\": \"l%"PRIuMAX"\"\n",
				        (uintmax_t) bcg_s1, (uintmax_t) s2);
				if (!strcmp(nm, faillabel)) {
					fprintf(jani, ", \"assignments\": [{\"ref\": \"marked\", "
					                                   "\"value\": 1}]");
				} else if (repairlabel && !strcmp(nm, repairlabel)) {
					fprintf(jani, ", \"assignments\": [{\"ref\": \"marked\", "
					                                "\"value\": 0}]");
				} else {
					fprintf(stderr, "Unknown label: %s\n", nm);
					fprintf(stderr, "Repair label: %s\n", repairlabel);
				}
				fprintf(jani, "}]}\n");
			} BCG_OT_END_ITERATE;
		}
	}
	BCG_OT_READ_BCG_END (&bcg_graph);

	fprintf(jani, "]}],\n"
	              "\t\"system\": {\"elements\": [{\"automaton\":\"aut\"}]},\n");
	fprintf(jani, "\t\"properties\": [\n"
	              "\t\t{\"name\":\"TBLmax_Unreliability\",\n"
				  "\t\t \"expression\":{\n"
				  "\t\t\t\"fun\":\"max\",\n"
				  "\t\t\t\"op\":\"filter\",\n"
				  "\t\t\t\"states\":{\"op\":\"initial\"},\n"
				  "\t\t\t\"values\":{\n"
				  "\t\t\t\t\"op\":\"Pmax\",\n"
				  "\t\t\t\t\"exp\":{\"op\":\"U\", \"left\":true, \"right\":{\"op\":\">\", \"left\":\"marked\", \"right\":0}, \"time-bounds\":{\"upper\": \"T\", \"lower\":\"L\"}}\n"
				  "\t\t\t\t}\n"
				  "\t\t\t}\n"
				  "\t\t},\n"
		   );
	/* Special output for 'lower = 0' since Modest only supports such
	 * queries when no lower bound is given.
	 */
	fprintf(jani, "\t\t{\"name\":\"TBmax_Unreliability\",\n"
				  "\t\t \"expression\":{\n"
				  "\t\t\t\"fun\":\"max\",\n"
				  "\t\t\t\"op\":\"filter\",\n"
				  "\t\t\t\"states\":{\"op\":\"initial\"},\n"
				  "\t\t\t\"values\":{\n"
				  "\t\t\t\t\"op\":\"Pmax\",\n"
				  "\t\t\t\t\"exp\":{\"op\":\"U\", \"left\":true, \"right\":{\"op\":\">\", \"left\":\"marked\", \"right\":0}, \"time-bounds\":{\"upper\": \"T\"}}\n"
				  "\t\t\t\t}\n"
				  "\t\t\t}\n"
				  "\t\t},\n"
		   );
	fprintf(jani, "\t\t{\"name\":\"TBmin_Unreliability\",\n"
				  "\t\t \"expression\":{\n"
				  "\t\t\t\"fun\":\"max\",\n"
				  "\t\t\t\"op\":\"filter\",\n"
				  "\t\t\t\"states\":{\"op\":\"initial\"},\n"
				  "\t\t\t\"values\":{\n"
				  "\t\t\t\t\"op\":\"Pmin\",\n"
				  "\t\t\t\t\"exp\":{\"op\":\"U\", \"left\":true, \"right\":{\"op\":\">\", \"left\":\"marked\", \"right\":0}, \"time-bounds\":{\"upper\": \"T\", \"lower\":\"L\"}}\n"
				  "\t\t\t\t}\n"
				  "\t\t\t}\n"
				  "\t\t},\n"
		   );
	fprintf(jani, "\t\t{\"name\":\"UBmax_Unreliability\",\n"
				  "\t\t \"expression\":{\n"
				  "\t\t\t\"fun\":\"max\",\n"
				  "\t\t\t\"op\":\"filter\",\n"
				  "\t\t\t\"states\":{\"op\":\"initial\"},\n"
				  "\t\t\t\"values\":{\n"
				  "\t\t\t\t\"op\":\"Pmax\",\n"
				  "\t\t\t\t\"exp\":{\"op\":\"U\", \"left\":true, \"right\":{\"op\":\">\", \"left\":\"marked\", \"right\":0}}\n"
				  "\t\t\t\t}\n"
				  "\t\t\t}\n"
				  "\t\t},\n"
		   );
	fprintf(jani, "\t\t{\"name\":\"UBmin_Unreliability\",\n"
				  "\t\t \"expression\":{\n"
				  "\t\t\t\"fun\":\"max\",\n"
				  "\t\t\t\"op\":\"filter\",\n"
				  "\t\t\t\"states\":{\"op\":\"initial\"},\n"
				  "\t\t\t\"values\":{\n"
				  "\t\t\t\t\"op\":\"Pmin\",\n"
				  "\t\t\t\t\"exp\":{\"op\":\">\", \"left\":\"marked\", \"right\":0}\n"
				  "\t\t\t\t}\n"
				  "\t\t\t}\n"
				  "\t\t},\n"
		   );
	fprintf(jani, "\t\t{\"name\":\"min_Unavailability\",\n"
				  "\t\t \"expression\":{\n"
				  "\t\t\t\"fun\":\"max\",\n"
				  "\t\t\t\"op\":\"filter\",\n"
				  "\t\t\t\"states\":{\"op\":\"initial\"},\n"
				  "\t\t\t\"values\":{\n"
				  "\t\t\t\t\"op\":\"Smin\",\n"
				  "\t\t\t\t\"exp\":{\"op\":\">\", \"left\":\"marked\", \"right\":0}\n"
				  "\t\t\t\t}\n"
				  "\t\t\t}\n"
				  "\t\t},\n"
		   );
	fprintf(jani, "\t\t{\"name\":\"max_Unavailability\",\n"
				  "\t\t \"expression\":{\n"
				  "\t\t\t\"fun\":\"max\",\n"
				  "\t\t\t\"op\":\"filter\",\n"
				  "\t\t\t\"states\":{\"op\":\"initial\"},\n"
				  "\t\t\t\"values\":{\n"
				  "\t\t\t\t\"op\":\"Smax\",\n"
				  "\t\t\t\t\"exp\":{\"op\":\">\", \"left\":\"marked\", \"right\":0}\n"
				  "\t\t\t\t}\n"
				  "\t\t\t}\n"
				  "\t\t},\n"
		   );
	fprintf(jani, "\t\t{\"name\":\"max_MTTF\",\n"
				  "\t\t \"expression\":{\n"
				  "\t\t\t\"fun\":\"max\",\n"
				  "\t\t\t\"op\":\"filter\",\n"
				  "\t\t\t\"states\":{\"op\":\"initial\"},\n"
				  "\t\t\t\"values\":{\n"
				  "\t\t\t\t\"op\":\"Emax\",\n"
				  "\t\t\t\t\"accumulate\": [\"time\"],\n"
				  "\t\t\t\t\"reach\":{\"op\":\">\", \"left\":\"marked\", \"right\":0},\n"
				  "\t\t\t\t\"exp\":1\n"
				  "\t\t\t\t}\n"
				  "\t\t\t}\n"
				  "\t\t},\n"
		   );
	fprintf(jani, "\t\t{\"name\":\"min_MTTF\",\n"
				  "\t\t \"expression\":{\n"
				  "\t\t\t\"fun\":\"max\",\n"
				  "\t\t\t\"op\":\"filter\",\n"
				  "\t\t\t\"states\":{\"op\":\"initial\"},\n"
				  "\t\t\t\"values\":{\n"
				  "\t\t\t\t\"op\":\"Emin\",\n"
				  "\t\t\t\t\"accumulate\": [\"time\"],\n"
				  "\t\t\t\t\"reach\":{\"op\":\">\", \"left\":\"marked\", \"right\":0},\n"
				  "\t\t\t\t\"exp\":1\n"
				  "\t\t\t\t}\n"
				  "\t\t\t}\n"
				  "\t\t}\n"
				  "\t]\n"
		   );
	fprintf(jani, "}");
	fclose(jani);
	return 0;
}
