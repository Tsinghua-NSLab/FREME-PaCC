/*
 * FILE:	 sub_set.h
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 *
 * 		Data structure used in order to store the mapping between a DFA/NFA state-ID and the 
 * 		corresponding set of NFA state-IDs. 
 * 		This data structure is used:
 * 		1) during the subset construction operation (that is, when transforming a NFA into its DFA counterpart). 
 * 		2) when reducing a NFA by collapsing common prefixes.
 * 
 */

#ifndef __SUBSET_H__
#define __SUBSET_H__

#include "stdinc.h"
#include "dfa.h"
#include "nfa.h"

#include <set>
#include <iterator>

using namespace std;
 
class sub_set
{

	state_t nfa_id;		// NFA state-ID

	state_t dfa_id;		// DFA state-ID which the set of NFA state subsets corresponds to - for subset construction

	NFA *nfa_obj;		// NFA target state which the set of NFA state subsets correpdonds to - for NFA reduction

	sub_set *next_o;	// the next subset in the HORIZONTAL dimension

	sub_set *next_v;	// the next subset in the VERTICAL dimension

public:

	/* 
	 * Instantiates a new subset with the given state id
	 */
	sub_set(state_t id, NFA *nfa=NULL);

	/*
	 * (Recursively) deallocates the subset
	 */
	~sub_set();

	/* For subset construction:
	 * - Queries a subset
	 * - If the subset is not present, then:
	 *	(1) creates a new DFA state
	 *	(2) creates a new subset (updating the whole data structure), and associates it to the newly created DFA state 
	 * nfaid_set:	set of nfa_id to be queried
	 * dfa:		dfa to be updated, in case a new state has to be created
	 * dfaid:	pointer to the variable containing the value of the (new) dfa_id
	 * Returns true if the subset (for nfaid_sets) was preexisting, and false if not. In the latter case,
	 * a new DFA state is created
	 */
	bool lookup(set<state_t> *nfaid_set, DFA *dfa, state_t *dfaid);

	/* For NFA reduction:
	 * - Queries a subset
	 * - If the subset is not present, then:
	 *	(1) creates a new NFA state 
	 *	(2) creates a new subset (updating the whole data structure), and associates it to the newly created NFA state 
	 * nfaid_set:	set of nfa_id to be queried
	 * nfa:		pointer to the variable containing the value of the (new) NFA state
	 * Returns true if the subset (for nfaid_sets) was preexisting, and false if not. In the latter case,
	 * a new NFA state is created
	 */
	bool lookup(set<state_t> *nfaid_set, NFA **nfa);

};

#endif /*__SUBSET_H__*/
