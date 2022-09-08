/*
 * FILE:	 nfa.h
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 * 
 * 		Implements a NFA.
 * 		Each NFA state is represented through a NFA object.
 *		NFA consists of:
 * 		1) a set of labeled transitions, in the form <symbol_t, next NFA state>. 
 *   		   note that several transitions leading to different states on the same input character may exist.
 *		2) a (possibly empty) set of epsilon transitions.
 * 		Additionally, each state has a unique identifier and a depth, the latter expressing the
 * 		minimum distance from the entry state.
 * 		This class provides:
 * 		1) methods to build a NFA by adding transitions, concatenating and or-ing NFAs, copying NFAs, etc.
 * 		2) function to transform a NFA into the equivalent without epsilon transitions
 * 		3) function to reduce a NFA by collapsing equivalent sub-NFAs
 *		4) function to transform a NFA into DFA (subset construction procedure)  
 * 		5) code to export the NFA into format suitable for dot program (http://www.graphviz.org/)
 *
 */

#ifndef __NFA_H__
#define __NFA_H__

#include "stdinc.h"
#include "link_set.h"
#include "dfa.h"

#include <list>
#include <set>
#include <iterator>
#include <map>
#include <utility>

using namespace std;

class NFA;

#define LIMIT 254
#define UNDEFINED 0xFFFFFFFF

/* Redefinitions used for convenience */
typedef set<NFA*> nfa_set; 			//set of NFA states
typedef list<NFA*> nfa_list;			//list of NFA states
typedef set<pair<symbol_t,NFA*>*> pair_set;	//set of labeled transitions

/* Iterator on set of NFA states */
#define FOREACH_SET(set_id,it) \
	for(nfa_set::iterator it=set_id->begin();it!=set_id->end();++it)

/* Iterator on list of NFA states */
#define FOREACH_LIST(list_id,it) \
	for(nfa_list::iterator it=list_id->begin();it!=list_id->end();++it)

/* Iterator on given set of labeled transitions */
#define FOREACH_PAIRSET(pairset,it) \
	for(pair_set::iterator it=pairset->begin();it!=pairset->end();++it)

/* Iterator on labeled transitions */
#define FOREACH_TRANSITION(it) FOREACH_PAIRSET(transitions,it)

/* Set membership */
#define SET_MEMBER(_set,item) (_set->find(item) != _set->end()) 

/* Set deletion */
#define SET_DELETE(_set,item) _set->erase(_set->find(item))

/* Identifier of the last NFA state instantiated */
static state_t nstate_nr = 0;

class NFA
{

	state_t id;			// state identifier

	pair_set *transitions;		// labeled transitions

	nfa_list *epsilon;		// epsilon transitions

	link_set *accepting;		// empty set for non-accepting states, set of rule-numbers for accepting states

	unsigned int depth;		// depth of the state in the graph

	int marked;			// marking I (used for algorithmical purposes)

	int visited, fully_visited;	// marking II(used for algorithmical purposes)
	
	int delete_only_me;		// to avoid recursion during deletion
	
	NFA *first;			// pointer to first state in the finite state machine the NFA-state belongs to
	
	NFA *last;			// pointer to last state in the finite state machine the NFA-state belongs to
	
public:

	static bool s_mode;		// indicate whether the dot have the matching newline option or not

	static bool i_mode;		// indicate whether the regular expression has the ignore case option or not

	static int num_rules;		// number of rules

public:

	/*
	 * Constructor
	 */
	NFA();

	/*
	 * (Recursive) de-allocator
	 */
	~NFA();

	/* FUNCTIONS TO QUERY A NFA STATE */

	/*
	 * Size in terms of number of states
	 */
	unsigned int size();

	/*
	 * Returns state identifier
	 */
	state_t get_id();

	/*
	 * Returns transitions on character c (set of next NFA states)
	 */
	nfa_set *get_transitions(symbol_t c);

	/*
	 * Returns transitions on character c (set of <char, next NFA state>)
	 */
	pair_set *get_transition_pairs(symbol_t c);

	/*
	 * Returns all the labeled transitions
	 */
	pair_set *get_transitions();

	/*
	 * Returns ture if there is a transition to nfa on character c
	 */
	bool has_transition(symbol_t c, NFA *nfa);

	/*
	 * Returns epsilon transitions
	 */
	nfa_list *get_epsilon();

	/*
	 * Returns the set of accepted regular expressions
	 */
	link_set *get_accepting();

	/*
	 * Returns the first state of the current NFA
	 */
	NFA *get_first();

	/*
	 * Returns the last state of the current NFA
	 */
	NFA *get_last();

	/*
	 * Returns the depth of the current NFA state
	 */
	unsigned int get_depth();

	/*
	 * Returns true if the current NFA state has at least one outgoing (labeled or epsilon) transition
	 */
	bool connected();

	/* FUNCTIONS TO MODIFY A NFA STATE */

	/*
	 * Adds a transition to nfa on character c
	 */
	void add_transition(symbol_t c, NFA *nfa);

	/*
	 * Adds a transition on symbol c and returns the last state of the new NFA
	 */
	NFA *add_transition(symbol_t c);

	/*
	 * Adds a range of transitions (from symbol from_s to symbol to_s to a given state or a new NFA) and returns the last state of the new NFA
	 */
	NFA *add_transition(symbol_t from_s, symbol_t to_s, NFA *nfa=NULL);

	/*
	 * Adds a collection of transitions (as specified in int_set) and returns the last state of the new NFA
	 */
	NFA *add_transition(int_set *l);	

	/*
	 * Adds a wildcard (to a given state or a new NFA) and returns the last state of the new NFA
	 */
	NFA *add_any(NFA *nfa=NULL);

	/*
	 * Adds an epsilon transition (to a given state or to a new state if nfa is not specified) and returns the new state
	 */
	NFA *add_epsilon(NFA *nfa=NULL);

	/*
	 * Returns the last state of the NFA obtained by concatenating the current one to tail
	 */
	NFA *link(NFA *tail);

	/*
	 * Makes a duplicate of the current NFA and returns it
	 */
	NFA *make_dup();

	/*
	 * Returns the state machine representing from lb to ub repetitions of the current one.
	 * The upper bound may be _INFINITY
	 */
	NFA *make_rep(int lb, int ub);

	/*
	 * Returns the last state of the NFA obtained by or-ing the current one to alt
	 */
	NFA *make_or(NFA *alt);

	/*
	 * Sets the current NFA-state as accepting (and increment the rule number)
	 */
	void accept(int rule);

	/*
	 * Adds the given set of rule-numbers to the accepting set
	 */ 
	void add_accepting(link_set *numbers);

	/* FUNCTIONS TO OBTAIN EQUIVALENT NFAs */

	/*
	 * Removes the epsilon transition from the NFA (building an equivalent NFA)
	 */
	void remove_epsilon();

	/*
	 * Reduces the current NFA (building an equivalent NFA) by removing equivalent sub-NFAs
	 */
	void reduce();

	/*
	 * Builds an equivalent NFA characterized by the fact that
	 * no NFA state has more than one transition on a given character.
	 * This is done by replacing a state with several outgoing transitions on the same character
	 * with a set of states connected through epsilon transitions
	 */
	void split_states();

	/* CONVERSION FUNCTIONS */

	/*
	 * Subset construction operation - converts the NFA into DFA and returns the DFA
	 */
	DFA *nfa2dfa();

	/*
	 * Exports the NFA into format suitable for dot program (http://www.graphviz.org/)
	 */
	void to_dot(FILE *file, const char *title);

	/* MISC */

	/*
	 * Resets the depth information on the whole NFA
	 */
	void reset_depth();

	/*
	 * Sets the depth information on the whole NFA
	 */
	void set_depth(unsigned int d=0);

	/*
	 * Sets the depth as the maximum distance of the entry state (excluding loops).
	 * Used only for traversal purposes
	 */
	void set_increasing_depth();

	/*
	 * Builds a set containing all the NFA states
	 */
	void traverse(nfa_set *queue);

	/*
	 * Builds a list containing all the NFA states in breath-first traversal order
	 */
	void traverse(nfa_list *queue);

	/*
	 * Resets the state identifier and reassigns them to the NFA states in breath-first traversal order (staring from first_id)
	 */
	void reset_state_id(state_t first_id=0);

	/*
	 * Returns the epsilon closure of the current NFA state
	 */
	nfa_set *epsilon_closure();

	/*
	 * Avoids recursion while deleting a NFA state
	 */
	void restrict_deletion();

	/*
	 * Returns ture if the state is marked
	 */
	bool is_marked();

	/*
	 * Marks current state
	 */
	void mark();

	/*
	 * Returns ture if the state and its descendants are marked II
	 */
	bool is_visited();

	/*
	 * Marks current state as visited
	 */
	void visit();

	/*
	 * Resets marking (algorithmical purposed) on the whole NFA
	 */
	void reset_marking();

	/*
	 * Resets marking II (algorithmical purposed) on the whole NFA
	 */
	void reset_visited();

	/*
	 * Recursive call to dot-file generation
	 */
	void to_dot(FILE *file, bool blue=false);

private:

	/*
	 * Returns the set of identifiers corresponding to the given set of NFA states
	 * (convenient method used during subset construction)
	 */
	set<state_t> *set_nfa2ids(nfa_set *fas);

	/*
	 * For debugging purposes: prints out the NFAs in the set
	 */
	void debug_set(nfa_set *s);

	/*
	 * Compares the transitions of nfa1 and nfa2
	 */
	bool same_transitions(NFA *nfa1, NFA *nfa2);

	/*
	 * Duplicates a NFA
	 */
	void dup_state(NFA *nfa, NFA *copy, std::map<state_t,NFA *> *mapping);

	/*
	 * Makes times linked copies of the current NFA
	 */
	NFA *make_mdup(int times);

	/*
	 * Removes the duplicate states from the NFA (building an equivalent NFA) - called by reduce()
	 */
	void remove_duplicates();

};

inline void NFA::accept(int rule) { this->accepting->insert(rule); }

inline state_t NFA::get_id() { return id; }

inline void NFA::add_accepting(link_set *numbers) { accepting->add(numbers); }

inline link_set *NFA::get_accepting() { return accepting; }

inline bool NFA::is_marked() { return marked; }

inline void NFA::mark() { marked = 1; }

inline void NFA::visit() { visited = 1; }

inline void NFA::restrict_deletion() { delete_only_me = 1; } 

inline bool NFA::connected() { return !(transitions->empty() && epsilon->empty()); }

inline nfa_list *NFA::get_epsilon() { return epsilon; }

inline pair_set *NFA::get_transitions() { return transitions; }

inline unsigned int NFA::get_depth() { return depth; }

#endif /*__NFA_H__*/
