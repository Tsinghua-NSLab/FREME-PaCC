/*
 * FILE:	 dfa.h
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 * 
 * 		Implements a DFA.
 * 		State transitions are represented through a bidimensional array (state table).
 * 		The class contains:
 * 		1) getter and setter methods to access important DFA information
 * 		2) O(nlogn) minimization algorithm
 * 		3) code to export the DFA in format suitable for dot program (http://www.graphviz.org/)
 *
 */

#ifndef __DFA_H__
#define __DFA_H__

#include "stdinc.h"
#include "int_set.h"
#include "link_set.h"

#include <stdio.h>
#include <map>
#include <set>
#include <list>
#include <sys/time.h>

using namespace std;

#define MAX_DFA_SIZE_INCREMENT 50

class DFA
{

        unsigned _size;				// number of states in the DFA

	state_t **state_table;			// 2-D state transition matrix: state_table[old_state][symbol] = new_state

        link_set **accepted_rules;		// For each state, set of accepted rules (empty-set if the state is non-accepting)
 
        int *marking;				// Marking used for algorithmical purposed

        unsigned int *depth;			// For each state, its depth (minimum "distance" from the entry state 0)

        state_t dead_state;			// State involving no progression

	state_t death;
 
private:

	unsigned int entry_allocated;		// number of entry allocated (for dynamic allocation)

public:	

	/*
	 * Instantiates a new DFA
	 */
	DFA(unsigned=50);

	/*
	 * Deallocates the DFA
	 */
	~DFA();

	/*
	 * Returns the number of states in the DFA
	 */
	unsigned int size();

	/*
	 * Sets the number of states in the DFA
	 */
	void set_size(unsigned int n);

	/*
	 * Equality operator
	 */
	bool equal(DFA *);

	/*
	 * Adds a state and returns its identifier. Allocates the necessary data structures
	 */
	state_t add_state();

	/*
	 * Adds a transition from old_state to new_state on symbol c
	 */
	void add_transition(state_t old_state, symbol_t c, state_t new_state);

	/*
	 * Minimizes the DFA
	 * Implementation of Hopcroft's O(n log n) minimization algorithm, follows
   	 * description by D. Gries.
   	 *
         * Time complexity: O(n log n)
         * Space complexity: O(c n), size < 4*(5*c*n + 13*n + 3*c) byte
         */
	void minimize();

	/*
	 * Returns the next_state from state on symbol c
	 */
	state_t get_next_state(state_t state, symbol_t c);

	/*
	 * Returns the (possibly empty) list of accepted rules on a state
	 */
	link_set *accepts(state_t state);

	/*
	 * Returns the transition table
	 */
	state_t **get_state_table();

	/*
	 * Returns the array of acceptstate pointers
	 */
	link_set **get_accepted_rules();

	/*
	 * Marks the current state as visited (for algorithmical purposed)
	 */
	void mark(state_t);

	/*
	 * Returns true if a given state is marked
	 */
	bool marked(state_t);

	/*
	 * Resets the marking (algorithmical purposes)
	 */
	void reset_marking();

	/*
	 * The given state become the dead state
	 */
	void set_dead_state(state_t state);


	state_t get_dead_state();

	/*
	 * Dumps the DFA
	 */
	void dump(FILE *state_trans=stdout, FILE *state_attach=stdout);

	/*
	 * Exports the DFA to DOT format
	 * download tool to produce visual representation from: http://www.graphviz.org/
	 */
	void to_dot(FILE *file, const char *title);

	/*
	 * Sets the state depth (minimum "distance") from the entry state 0
	 */
	void set_depth();

	/*
	 * Gets the array of state depth
	 */
	unsigned int *get_depth();

	void generate(int ndfa);

};

inline void DFA::add_transition(state_t old_state, symbol_t c, state_t new_state) { state_table[old_state][c] = new_state; }

inline bool DFA::marked(state_t state) { return marking[state]; }

inline void DFA::mark(state_t state) { marking[state] = 1; }

inline state_t DFA::get_next_state(state_t state, symbol_t c) { return state_table[state][c]; }

inline link_set *DFA::accepts(state_t state) { return accepted_rules[state]; }

inline unsigned int DFA::size() { return _size; }

inline void DFA::set_size(unsigned int n) { _size = n; }

inline state_t **DFA::get_state_table() { return state_table; }

inline link_set **DFA::get_accepted_rules() { return accepted_rules; }

inline unsigned int *DFA::get_depth() { return depth; }

inline void DFA::set_dead_state(state_t state) { dead_state = state; }

#endif /*__DFA_H__*/
