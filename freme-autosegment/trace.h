/*
 * FILE:	 trace.h
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 *
 *		This class is used to test the automaton traversal.
 *		Each trace has associated the file representing the input text.
 *		Traversal functions are provided.
 *
 */

#ifndef __TRACE_H__
#define __TRACE_H__

#include "nfa.h"
#include "dfa.h"

#include <sys/time.h>
#include <set>
#include <queue>
#include <vector>
#include <iterator>
#include <algorithm>
#include <pthread.h>

using namespace std;

struct run_state_unit
{
	unsigned long cptr;
	int ndfa;
	link_set *anterule;
};
typedef struct run_state_unit run_state_unit_t;

class trace
{


	char *tracename;			// name of the trace file

	FILE *tracefile;			// trace file

public:

	/*
	 * Constructor
	 */
	trace(char *filename=NULL);

	/*
	 * De-allocator
	 */
	~trace();

	/*
	 * Sets the trace
	 */
	void set_trace(char *filename);

	/*
	 * Traverses the given nfa and prints statistics to stream
	 * Note: assume that spliter-state was not called (epsilon auto-loop meaningless)
	 */
	void traverse(NFA *nfa, FILE *stream=stderr);

	/*
	 * Traverses the given dfa and prints statistics to stream
	 */
	void traverse(DFA *dfa, FILE *stream=stderr);

	/*
	 * Traverses the given front-end fa and generates the events with time tag
	 */
	void event_generate(DFA **dfa, FILE *output=stdout, FILE *stream=stderr);

	//void pXthread(run_state_unit_t *init);

};

#endif /*__TRACE_H__*/
