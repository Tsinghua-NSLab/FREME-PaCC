/*
 * FILE:	 main.c
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 * 
 *		This is the main entry file
 *
 */

#include "stdinc.h"
#include "spliter.h"
#include "parser.h"
#include "nfa.h"
#include "dfa.h"
#include "trace.h"

/*
 * Program entry point.
 * Please modify the main() function to add custom code.
 * The options allow to create a DFA from a list of regular expressions.
 * If a single single DFA cannot be created because state explosion occurs, then a list of DFA
 * is generated (see MAX_DFA_SIZE in dfa.h).
 * Additionally, the DFA can be exported in proprietary format for later re-use, and be imported.
 * Finally, processing a trace file is an option.
 */


#ifndef CUR_VER
#define CUR_VER		"1.0.0"
#endif

int VERBOSE = 0;
int DEBUG = 1;

/*
 * Returns the current version string
 */
void version()
{
    printf("\nVersion:: %s\n\n", CUR_VER);
}

/*
 * Usage
 */
static void usage()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: ./BFA [options]\n");
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "    --help,-h                     print this message\n");
	fprintf(stderr, "    --version,-v                  print version number\n");
	fprintf(stderr, "    --parse,-p <regex_file>       process regex file\n");
	fprintf(stderr, "    --trace,-t <trace_file>       trace file to be processed\n");
	//fprintf(stderr, "    --times,-n <number_of_times>  trace file to be processed\n");
	//fprintf(stderr, "    --i  ignore case\n");
	fprintf(stderr, "\n");
	exit(0);
}

/*
 * Configuration
 */
static struct conf
{
	char *regex_file;
	char *trace_file;
	bool generate;
	int num_of_times;
	bool i_mod;
} config;

/*
 * Initialize the configuration
 */
void init_conf()
{
	config.regex_file = NULL;
	config.trace_file = NULL;
	config.generate = false;
	config.num_of_times = 1;
	config.i_mod = false;
}

/*
 * Print the configuration
 */
void print_conf()
{
	fprintf(stderr, "\nCONFIGURATION: \n");
	if (config.regex_file)
	{
		fprintf(stderr, "- RegEx file: %s\n", config.regex_file);
	}
	if (config.trace_file)
	{
		fprintf(stderr, "- Trace file: %s\n", config.trace_file);
	}
	if (config.num_of_times)
	{
		fprintf(stderr, "- Number of times: %d\n", config.num_of_times);
	}
	if (config.i_mod)
	{
		fprintf(stderr,"- ignore case selected\n");
	}
}

/*
 * Parse the main call parameters
 */
static int parse_arguments(int argc, char **argv)
{
	int i = 1;
	if (argc < 2)
	{
		usage();
		return 0;
	}
	while (i < argc)
	{
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			usage();
			return 0;
		}
		else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
		{
			version();
			return 0;
		}
		else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--times") == 0)
		{
			i++;
			if (i == argc)
			{
				fprintf(stderr, "Times number missing.\n");
				return 0;
			}
			config.num_of_times = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--parse") == 0)
		{
			i++;
			if (i == argc)
			{
				fprintf(stderr, "Regular expression file name missing.\n");
				return 0;
			}
			config.regex_file = argv[i];
		}
		else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0)
		{
			i++;
			if (i == argc)
			{
				fprintf(stderr, "Trace file name missing.\n");
				return 0;
			}
			config.trace_file = argv[i];
		}
		else if (strcmp(argv[i], "--g") == 0)
		{
			config.generate = true;
		}
		//else if (strcmp(argv[i], "--i") == 0)
		//{
		//	config.i_mod = true;
		//}
		else
		{
			fprintf(stderr, "Ignoring invalid option %s\n", argv[i]);
		}
		i++;
	}
	return 1;
}

/*
 * Check that the given file can be read/written
 */
void check_file(char *filename, char *mode)
{
	FILE *file = fopen(filename, mode);
	if (file == NULL)
	{
		fprintf(stderr, "Unable to open file %s in %c mode", filename, mode);
		error("\n");
	}
	else
	{
		fclose(file);
	}
}

/*
 * MAIN - entry point
 */
int main(int argc, char **argv)
{
	init_conf();				// read configuration
	while (!parse_arguments(argc, argv))
	{
		exit(0);
	}

	// check that it is possible to open the files
	if (config.regex_file != NULL)
	{
		check_file(config.regex_file, "r");
	}
	if (config.trace_file != NULL)
	{
		check_file(config.trace_file, "r");
	}
	//print_conf();

	// check that either a regex file or a DFA import file are given as input
	if (config.regex_file == NULL && !config.generate)
	{
		error("No data file - please use a regex file\n");
	}

	if (config.num_of_times <= 0)
	{
		error("incorrect times number - please input a postive integer\n");
	}

	NFA *nfa = NULL;			// NFA declaration
	DFA *dfa[3] = {NULL};		// DFA declaration
	int stages;
	char cmd[256] = {0};

	// if regex file is provided, parses it and instantiate the corresponding NFA.
	// if feasible, convert the NFA to DFA
	if (config.regex_file != NULL && !config.generate)
	{
		sprintf(cmd, "mkdir -p %s; touch %s %s %s %s %s %s", TRASH, ANY_RULE, CARET_RULE, STAR_RULE, TIME_RULE, INDEX_TABLE, TSI_DATA);
		if (system(cmd) < 0)
		{
			error("Cannot generate transition files\n");
		}

		struct timeval start, end;
		gettimeofday(&start, NULL);
		FILE *regex_file = fopen(config.regex_file, "r");
		fprintf(stderr, "\n1st-Step: parsing the regular expression file %s ...\n", config.regex_file);
		spliter *split = new spliter();
		split->split(regex_file);
		fclose(regex_file);
		delete split;
		
		fprintf(stderr, "\n2nd-Step: constructing BFA for the given regular epxressions ...\n");
		unsigned int total_size = 0, total_time = 0;
		for (int i=0; i<3; i++)
		{
			FILE *split_file, *state_trans, *state_attach;
			switch (i)
			{
				case 0:
					split_file = fopen(ANY_RULE, "r");
					state_trans = fopen(DFA0_TRANS, "w");
					state_attach = fopen(DFA0_MATCH, "w");
					break;
				case 1:
					split_file = fopen(CARET_RULE, "r");
					state_trans = fopen(DFA1_TRANS, "w");
					state_attach = fopen(DFA1_MATCH, "w");
					break;
				case 2:
			//		split_file = fopen(STAR_RULE, "r");
			//		state_trans = fopen(DFA3_TRANS, "w");
			//		state_attach = fopen(DFA3_MATCH, "w");
			//		break;
			//	case 3:
					split_file = fopen(TIME_RULE, "r");
					state_trans = fopen(DFA2_TRANS, "w");
					state_attach = fopen(DFA2_MATCH, "w");
					break;
			}
			//fprintf(stderr, "\nParsing ruleset class [%d] ...\n", i);

			parser *parse = new parser(false, false, config.i_mod);
			nfa = parse->parse(split_file);
			nfa->remove_epsilon();
			nfa->reduce();
			dfa[i] = nfa->nfa2dfa();
			if (dfa[i] == NULL)
			{
				printf("Max DFA size %ld exceeded during creation: the DFA[%d] was not generated\n", MAX_DFA_SIZE, i);
				exit(1);
			}
			else 
			{
				if (dfa[i]->size() < 100000)
				{
					dfa[i]->minimize();
					printf("DFA size = %d\n", dfa[i]->size());
				}
				total_size += (dfa[i]->size() > 1) ? dfa[i]->size() : 0;

				dfa[i]->dump(state_trans, state_attach);
			}
			fclose(state_attach);
			fclose(state_trans);
			fclose(split_file);
			delete parse;
		}
		gettimeofday(&end, NULL);
		total_time += (end.tv_sec - start.tv_sec);
		printf("===========================================\n");
		printf(">> # of BFA's DFA states: %ld\n>> preprocessing time: %lds\n", total_size, total_time);
		printf("===========================================\n");
	}

	/*
	 * ADD YOUR CODE HERE 
	 * This is the right place to call the compression algorithms (see nfa.h, dfa.h),
	 * and analyze your data structures.
	 */

	/* BEGIN USER CODE */

	// frond-end DFA compression

	/* END USER CODE */

	// trace file traversal
	if (config.trace_file != NULL)
	{
		test_throughput(config.trace_file, config.num_of_times);
	}

	sprintf(cmd, "rm -rf %s", TRASH);
	//system(cmd);

	// automata de-allocation
	if (nfa != NULL)
	{
		delete nfa;
	}
	for (int i=0; i<3; i++)
	{
		if (dfa[i] != NULL)
		{
			delete dfa[i];
		}
	}

	return 0;
}
