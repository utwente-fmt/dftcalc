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

int main(int argc, char* argv[])
{
	BCG_TYPE_OBJECT_TRANSITION bcg_graph;
	BCG_TYPE_STATE_NUMBER bcg_s1;
	BCG_TYPE_STATE_NUMBER bcg_nb_states;

	char *faillabel, *repairlabel = NULL;

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

	/* Header information in JANI file */
	fprintf(jani,
	        "{\t\"jani-version\": 1,\n"
	         "\t\"name\": \"from_%s\",\n"
	         "\t\"type\": \"ma\",\n"
		 "\t\"actions\": [],\n"
	         "\t\"variables\": [{\"name\": \"marked\", "
	                            "\"type\": {\"kind\":\"bounded\",\"base\":\"int\",\"upper-bound\":1}, "
	                            "\"initial-value\": 0}],\n",
	         argv[1]);

	bcg_nb_states = BCG_OT_NB_STATES(bcg_graph);

	/* Done with constants, moving on to the automaton */
	fprintf(jani, "\t\"automata\": [{\n"
	        "\t\t\"name\": \"aut\",\n"
	        "\t\t\"locations\": [\n");

	/* We use one location per state */
	for (uintmax_t i = 0; i < bcg_nb_states; i++) {
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
				nm = BCG_OT_LABEL_STRING(bcg_graph, labelnum);
				if (strncmp(nm, "rate ", 5))
					continue;
				if (!first_transition)
					fprintf(jani, ",\n");
				first_transition = 0;
				fprintf(jani, "\t\t\t{\"location\": \"l%"PRIuMAX"\",\n"
					"\t\t\t \"rate\": {\"exp\": %s},\n"
					"\t\t\t \"destinations\": [{\"location\":\"l%"PRIuMAX"\"}]"
					"\t\t\t }",
					(uintmax_t) bcg_s1, nm+5,
					(uintmax_t) s2);
			} BCG_OT_END_ITERATE;
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
	              "\t\"system\": {\"elements\": [{\"automaton\": \"aut\"}]}\n"
	              "}");
	
	fclose(jani);
	return 0;
}
