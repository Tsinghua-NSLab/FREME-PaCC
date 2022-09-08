#include "nfa.h"
#include "sub_set.h"

bool NFA::s_mode = 0;
bool NFA::i_mode = 0;
int NFA::num_rules = 0;

NFA::NFA()
{
	id = nstate_nr++;
	if (nstate_nr == NO_STATE)
	{
		error("NFA:: Max number of states reached");
	}
	transitions = new pair_set();
	epsilon = new nfa_list();
	accepting = new link_set();
	depth = UNDEFINED;
	marked = 0;
	visited = 0;
	fully_visited = 0;
	delete_only_me = 0;
	first = this;
	last = this;
}

NFA::~NFA()
{
	if (!delete_only_me)
	{
		nfa_set *queue = new nfa_set();
		traverse(queue);
		FOREACH_SET(queue, it)
		{
			if (*it != this)
			{
				(*it)->delete_only_me = true;
				delete *it;
			}
		}
		delete queue;
	}
	FOREACH_TRANSITION(it)
	{
		delete (*it);
	}
	delete transitions;
	if (epsilon != NULL)
	{
		delete epsilon;
	}
	delete accepting;
}

void NFA::traverse(nfa_set *queue)
{
	nfa_list *q = new nfa_list();
	q->push_back(this);
	queue->insert(this);				// (insert) first NFA state
	this->marked = true;
	while (!q->empty())
	{
		NFA *qnfa = q->front();
		q->pop_front();
		FOREACH_PAIRSET(qnfa->transitions, it)
		{					// labeled transitions traversal
			NFA *nfa = (*it)->second;
			if (!nfa->marked)
			{
				nfa->marked = true;
				q->push_back(nfa);
				queue->insert(nfa);	// breath-first NFA state adding
			}
		}
		FOREACH_LIST(qnfa->epsilon, it)
		{					// epsilon transitions traversal
			NFA *nfa = *it;
			if (!nfa->marked)
			{
				nfa->marked = true;
				q->push_back(nfa);
				queue->insert(nfa);	// breath-first NFA state adding
			}
		}
	}
	FOREACH_SET(queue, it)
	{
		(*it)->marked = false;
	}
	delete q;
}

void NFA::traverse(nfa_list *queue)
{
	nfa_list *q = new nfa_list();
	q->push_back(this);
	queue->push_back(this);				// (push) first NFA state
	this->marked = true;
	while (!q->empty())
	{
		NFA *qnfa = q->front();
		q->pop_front();
		FOREACH_PAIRSET(qnfa->transitions, it)
		{
			NFA *nfa = (*it)->second;
			if (!nfa->marked)
			{
				nfa->marked = true;
				q->push_back(nfa);
				queue->push_back(nfa);
			}
		}
		FOREACH_LIST(qnfa->epsilon, it)
		{
			NFA *nfa = *it;
			if (!nfa->marked)
			{
				nfa->marked = true;
				q->push_back(nfa);
				queue->push_back(nfa);
			}
		}
	}
	FOREACH_LIST(queue, it)
	{
		(*it)->marked = false;
	}
	delete q;
}

unsigned int NFA::size()
{
	nfa_list *queue = new nfa_list();
	traverse(queue);
	unsigned int size = queue->size();
	delete queue;
	return size;
}

bool NFA::is_visited()
{
	if (!visited)
	{
		return false;
	}
	if (fully_visited)
	{
		return true;
	}
	nfa_list *q = new nfa_list();
	q->push_back(this);
	marked = true;
	while (!q->empty())
	{
		NFA *qnfa = q->front();
		q->pop_front();
		FOREACH_PAIRSET(qnfa->transitions, it)
		{
			NFA *nfa = (*it)->second;
			if (!nfa->marked)
			{
				if (!nfa->visited)
				{
					reset_marking();
					delete q;
					return false;
				}
				else if (!nfa->fully_visited)
				{
					nfa->marked = true;
					q->push_back(nfa);
				}
			}
		}
		FOREACH_LIST(qnfa->epsilon, it)
		{
			NFA *nfa = *it;
			if (!nfa->marked)
			{
				if (!nfa->visited)
				{
					reset_marking();
					delete q;
					return false;
				}
				else if (!nfa->fully_visited)
				{
					nfa->marked = true;
					q->push_back(nfa);
				}
			}
		}
	}
	reset_marking();
	delete q;
	fully_visited = true;
	return true;
}

void NFA::reset_visited()
{
	nfa_set *queue = new nfa_set();
	traverse(queue);
	FOREACH_SET(queue, it)
	{
		(*it)->visited = false;
		(*it)->fully_visited = false;
	}
	delete queue;
}

void NFA::reset_marking()
{
	if (!marked)
	{
		return;
	}
	marked = 0;
	FOREACH_TRANSITION(it)
	{
		NFA *nfa = (*it)->second;
		nfa->reset_marking();
	}
	FOREACH_LIST(epsilon, it)
	{
		NFA *nfa = (*it);
		nfa->reset_marking();
	}
}

bool NFA::has_transition(symbol_t c, NFA *nfa)
{
	FOREACH_TRANSITION(it)
	{
		if ((*it)->first==c && (*it)->second==nfa)
		{
			return true;
		}
	}
	return false;
}

void NFA::add_transition(symbol_t c, NFA *nfa)
{
	if (has_transition(c, nfa))
	{
		return;
	}
	pair<unsigned,NFA *> *tr = new pair<unsigned,NFA *>(c, nfa);
	transitions->insert(tr);
}

nfa_set *NFA::get_transitions(symbol_t c)
{
	if (transitions->empty())
	{
		return NULL;
	}
	nfa_set *result = new nfa_set();
	FOREACH_TRANSITION(it)
	{
		symbol_t ch = (*it)->first;
		NFA *nfa = (*it)->second;
		if (c == ch)
		{
			result->insert(nfa);
		}
	}
	if (result->empty())
	{ 
		delete result;
		result = NULL;
	}
	return result;
}

pair_set *NFA::get_transition_pairs(symbol_t c)
{
	pair_set *result = new pair_set();
	FOREACH_TRANSITION(it)
	{
		if ((*it)->first == c)
		{
			result->insert(*it);
		}
	}
	return result;
}

set<state_t> *NFA::set_nfa2ids(nfa_set *fas)
{
	set<state_t> *ids = new set<state_t>();
	FOREACH_SET(fas, it)
	{
		ids->insert((*it)->id);
	}
	return ids;
}

nfa_set *NFA::epsilon_closure()
{
	nfa_set *closure = new nfa_set();
	nfa_list *queue = new nfa_list();
	queue->push_back(this);				// current state
	while (!queue->empty())
	{
		NFA *nfa = queue->front();
		queue->pop_front();
		closure->insert(nfa);			// put current state in the epsilon closure
		FOREACH_LIST(nfa->epsilon, it)
		{
			if (!SET_MEMBER(closure, *it))
			{				// corresponding epsilon states
				queue->push_back(*it);
			}
		}
	}
	delete queue;
	return closure;
}

DFA *NFA::nfa2dfa()
{
	// DFA to be created
	DFA *dfa = new DFA();

	// contains mapping between DFA and NFA set of states
	sub_set *mapping = new sub_set(0);
	// queue of DFA states to be processed and of the set of NFA states they correspond to
	list<state_t> *queue = new list<state_t>();
	list<nfa_set*> *mapping_queue = new list<nfa_set*>();
	// iterators used later on
	nfa_set::iterator set_it;
	// new dfa state id
	state_t target_state = NO_STATE;
	// set of nfas state corresponding to target dfa state
	nfa_set *target;		// in FA form
	set<state_t> *ids = NULL;	// in id form

	/* Code begins here */
	// initialize data structure starting from INITIAL STATE
	target = this->epsilon_closure();
	ids = set_nfa2ids(target);
	mapping->lookup(ids, dfa, &target_state);
	delete ids;
	FOREACH_SET(target, set_it)
	{
		dfa->accepts(target_state)->add((*set_it)->accepting);
	}
	queue->push_back(target_state);
	mapping_queue->push_back(target);

	// process the states in the queue and adds the not yet processed DFA states
	// to it while creating them
	while (!queue->empty())
	{
		// dequeue an element
		state_t state = queue->front();
		queue->pop_front();
		nfa_set *cl_state = mapping_queue->front();
		mapping_queue->pop_front();
		// each state must be processed only once
		if (!dfa->marked(state))
		{
			dfa->mark(state);
			// iterate other all characters and compute the next state for each of them
			for (symbol_t i=0; i<CSIZE; i++)
			{
				target = new nfa_set();
				FOREACH_SET(cl_state, set_it)
				{
					nfa_set *state_set = (*set_it)->get_transitions(i);
					if (state_set != NULL)
					{
						FOREACH_SET(state_set, it2)
						{
							nfa_set *eps_cl = (*it2)->epsilon_closure();
							target->insert(eps_cl->begin(), eps_cl->end());
							delete eps_cl;
						}
						delete state_set;
					}
				}
				// look whether the target set of state already corresponds to a state in the DFA
				// if the target set of states does not already correspond to a state in a DFA,
				// then add it
				ids = set_nfa2ids(target);
				bool found = mapping->lookup(ids, dfa, &target_state);
				delete ids;
				if (target_state == MAX_DFA_SIZE)
				{
					delete queue;
					while (!mapping_queue->empty())
					{
						delete mapping_queue->front();
						mapping_queue->pop_front();
					}
					delete mapping_queue;
					delete mapping;
					delete dfa;
					delete target;
					delete cl_state;
					return NULL;
				}
				if (!found)
				{
					queue->push_back(target_state);
					mapping_queue->push_back(target);
					FOREACH_SET(target, set_it)
					{
						dfa->accepts(target_state)->add((*set_it)->accepting);
					}
					if (target->empty())
					{
						dfa->set_dead_state(target_state);
					}
				}
				else
				{
					delete target;
				}
				dfa->add_transition(state,i, target_state);	// add transition to the DFA
			} // endfor character i
		} // endif state marked
		delete cl_state;
	} // endwhile
	// deallocate all the sets and the state_mapping data structure
	delete queue;
	delete mapping_queue;
	delete mapping;

	return dfa;
}

void NFA::debug_set(nfa_set *s)
{
	if (s->empty())
	{
		fprintf(stderr, "EMPTY");
		return;
	}
	FOREACH_SET(s, it)
	{
		NFA *fa = *it;
		fprintf(stderr, "%ld ", fa->id);
	}
}

void NFA::reset_depth()
{
	nfa_set *queue = new nfa_set();
	traverse(queue);
	FOREACH_SET(queue, it)
	{
		(*it)->depth = UNDEFINED;
	}
	delete queue;
}

void NFA::set_depth(unsigned int d)
{
	if (depth==UNDEFINED || depth>d)
	{
		depth = d;
		FOREACH_TRANSITION(it)
		{
			((*it)->second)->set_depth(d + 1);
		}
		FOREACH_LIST(epsilon, it)
		{
			(*it)->set_depth(d + 1);
		}
	}
}

bool check_dep(unsigned int *dep, state_t from, state_t to)
{
	if (from == to)
	{
		return false;
	}
	state_t s = to;
	do
	{
		s = dep[s];
		if (s == from)
		{
			return false;
		}
	} while(s != UNDEFINED);
	return true;
}

void NFA::set_increasing_depth()
{
	reset_state_id();
	unsigned int _size = size();
	unsigned int *dependences = new unsigned int[_size];
	for (unsigned int s=0; s<_size; s++)
	{						// initialize the array dependence[]
		dependences[s] = UNDEFINED;
	}
	nfa_list *q = new nfa_list();
	this->depth = 0;
	this->marked = true;
	q->push_back(this);
	while (!q->empty())
	{
		NFA *nfa = q->front();
		q->pop_front();
		FOREACH_PAIRSET(nfa->transitions, it)
		{					// nfa -> tx, depth[nfa->id] >= depth[tx->id]
			NFA *tx = (*it)->second;
			if (tx->depth == UNDEFINED || (tx->depth <= nfa->depth && check_dep(dependences,tx->id,nfa->id)))
			{
				tx->depth = nfa->depth + 1;
				dependences[tx->id] = nfa->id;
				if (!tx->marked)
				{
					q->push_back(tx);
				}
				tx->marked = true;
			}
		}
		FOREACH_LIST(nfa->epsilon, it)
		{
			NFA *tx = *it;
			if (tx->depth == UNDEFINED || (tx->depth <= nfa->depth && check_dep(dependences,tx->id,nfa->id)))
			{
				tx->depth = nfa->depth + 1;
				dependences[tx->id] = nfa->id;
				if (!tx->marked)
				{
					q->push_back(tx);
				}
				tx->marked = true;
			}
		}
		nfa->marked = false;
	}
	delete q;
	delete [] dependences;
}

/* 
 * Returns the first state of the FA.
 * (helper function needed because we don't update the first pointer in intermediate nodes 
 * in case of concatenation)
 */
NFA *NFA::get_first()
{
	if (this->first == this)
	{
		return this;
	}
	else
	{
		return this->first->get_first();
	}
}

/* 
 * Returns the last state of the FA.
 * (helper function needed because we don't update the last pointer in intermediate nodes
 * in case of concatenation)
 */
NFA *NFA::get_last()
{
	if (this->last == this)
	{
		return this;
	}
	else
	{
		return this->last->get_last();
	}
}

/*
 * Functions used to build a NFA starting from a given regular expression.
 */
NFA *NFA::add_epsilon(NFA *nfa)
{
	if (nfa == NULL)
	{
		nfa = new NFA();
	}
	nfa->first = this;
	this->last = nfa;
	epsilon->push_back(nfa);
	return nfa;
}

/* 
 * Adds only one transition for the given symbol
 */
NFA *NFA::add_transition(symbol_t c)
{
	NFA *newstate = new NFA();
	newstate->first = this->get_first();
	this->get_first()->last = newstate;		// this is redundant, but speeds up later on
	this->last = newstate;
	this->add_transition(c, newstate);
	if (i_mode)
	{
		if (c>='A' && c<='Z')
		{
			this->add_transition(c+('a'-'A'), newstate);
		}
		if (c>='a' && c<='z')
		{
			this->add_transition(c-('a'-'A'), newstate);
		}
	}
	return newstate;
}

/*
 * Adds a set of transitions (e.g.: no case)
 */
NFA *NFA::add_transition(int_set *l)
{
	NFA *finalst = new NFA();
	for (unsigned int c=l->head(); c!=UNDEF; c=l->succ(c))
	{
		this->add_transition(c, finalst);
		if (i_mode)
		{
			if (c>='A' && c<='Z')
			{
				this->add_transition(c+('a'-'A'), finalst);
			}
			if (c>='a' && c<='z')
			{
				this->add_transition(c-('a'-'A'), finalst);
			}
		}
	}
	finalst->first = this->get_first();
	this->get_first()->last = finalst;		// this is redundant, but speeds up later on
	this->last = finalst;
	return finalst;
}

/*
 * Adds a range of transitions
 */
NFA *NFA::add_transition(symbol_t from, symbol_t to, NFA *nfa)
{
	if (nfa == NULL)
	{
		nfa = new NFA();
	}
	for (symbol_t i=from; i<=to; i++)
	{
		this->add_transition(i, nfa);
		if (i_mode)
		{
			if (i>='A' && i<='Z')
			{
				this->add_transition(i+('a'-'A'), nfa);
			}
			if (i>='a' && i<='z')
			{
				this->add_transition(i-('a'-'A'), nfa);
			}
		}
	}
	nfa->first = this->get_first();
	this->get_first()->last = nfa;			// this is redundant, but speeds up later on
	this->last = nfa;
	return nfa;
}

/*
 * Adds a wildcard
 */
NFA *NFA::add_any(NFA *nfa)
{
	/*
	 * Judge the configuration of '.' here
	 */
	if (s_mode)
	{
		if (nfa == NULL)
		{
			nfa = new NFA();
		}
		for (symbol_t i=0; i<=CSIZE-1; i++)
		{
			if (i == '\n')
			{
				continue;
			}
			this->add_transition(i, nfa);
		}
		nfa->first = this->get_first();
		this->get_first()->last = nfa;
		this->last = nfa;
		return nfa;
	}
	return add_transition(0, CSIZE-1, nfa);
}

/*
 * Links the current machine with fa and returns the last
 */
NFA *NFA::link(NFA *fa)
{
	if (fa == NULL)
	{
		return this;
	}
	NFA *second = fa->get_first();
	if (!second->connected())
	{						// fa is not connected
		second->delete_only_me = 1;
		delete second;
		return this;
	}

	/* Modify "second" in fa nodes -> it will be replaced by "this" */
	nfa_set *queue = new nfa_set();
	second->traverse(queue);
	FOREACH_SET(queue, it)
	{
		NFA *node = *it;
		if (node->first == second)
		{					// the original NFA (this) is first, the given fa (second) is last
			node->first = this;
		}
		FOREACH_PAIRSET(node->transitions, it)
		{
			if ((*it)->second == second)
			{				// point to the new linked NFA
				(*it)->second = this;
			}
		}
		bool exist = false;
		FOREACH_LIST(node->epsilon, it)
		{
			if (*it == second)
			{
				exist = true;
				break;
			}
		}
		if (exist)
		{
			node->epsilon->remove(second);
			node->epsilon->push_back(this);
		}
	}
	delete queue;

	/* Copy transitions */
	FOREACH_PAIRSET(second->transitions, it)
	{
		this->add_transition((*it)->first, (*it)->second);
	}
	FOREACH_LIST(second->epsilon, it)
	{
		this->epsilon->push_back(*it);
	}

	this->get_last()->last = second->get_last();
	this->last = second->get_last();

	second->delete_only_me = 1;
	delete second;

	return this->last;
}

/*
 * makes a machine repeatable from lb (lower bound) to ub (upper bound) times. 
 */
NFA *NFA::make_rep(int lb, int ub)
{
	
	if (ub == _INFINITY)
	{						// upper bound is _INIFINITY
		if (lb == 0)
		{
			this->get_first()->epsilon->push_back(this->get_last());
			this->get_last()->epsilon->push_back(this->get_first());
			this->get_last()->add_epsilon();
		}
		else if (lb == 1)
		{
			NFA *copy = this->make_dup();
			this->get_first()->epsilon->push_back(this->get_last());
			this->get_last()->epsilon->push_back(this->get_first());
			this->get_last()->add_epsilon();
			this->get_last()->link(copy);
		}
		else
		{
			if (!epsilon->empty())
			{				// to avoid critical situations with backward epsilon
				this->add_epsilon();
			}
			NFA *copies = this->make_mdup(lb-1);
			this->get_last()->epsilon->push_back(this->get_first());
			return copies->link(this);
		}
	}
	else
	{						// upper bound is limited
		if (lb == ub)
		{					// a fixed number of repetitions
			if (lb == 0)
			{
				error("{0} counting constraint");
			}
			if (lb == 1)
			{
				return this;
			}
			if (!epsilon->empty())
			{				// to avoid critical situations with backward epsilon
				this->add_epsilon();
			}
			NFA *copies = this->make_mdup(lb-1);
			return copies->link(this);
		}
		else
		{
			if (lb == 0)
			{
				if (ub == 1)
				{			// {0,1} 
					if (!epsilon->empty())
					{		// to avoid critical situations with backward epsilon
						this->add_epsilon();
					}
					this->get_first()->epsilon->push_back(this->get_last());
					return this->get_last();
				}
				else
				{			// {0,n}
					if (!epsilon->empty())
					{		// to avoid critical situations with backward epsilon
						this->add_epsilon();
					}
					this->get_first()->epsilon->push_back(this->get_last());
					NFA *copies = this->make_mdup(ub-1);
					return copies->link(this);
				}
			}
			else if (lb == 1)
			{				// {1,n}
					if (ub == 0)
					{
						error("{1,0} counting constraint\n");
					}
					if (!epsilon->empty())
					{		// to avoid critical situations with backward epsilon
						this->add_epsilon();
					}
					NFA *copy = this->make_dup();
					copy->get_first()->epsilon->push_back(copy->get_last());			
					if (ub == 2)
					{
						return this->get_last()->link(copy);
					}
					else
					{
						return (this->get_last()->link(copy))->link(copy->make_mdup(ub-2));
					}
			}
			else
			{				// {m,n}
				if (lb > ub)
				{
					error("{m,n} counting constraint with m>n");
				}
				if (!epsilon->empty())
				{			// to avoid critical situations with backward epsilon
					this->add_epsilon();
				}
				NFA *head = this->make_mdup(lb-1);
				NFA *tail = this->make_dup();
				tail->get_first()->epsilon->push_back(tail->get_last());
				if (ub-lb > 1)
				{
					tail = (tail->get_last())->link(tail->make_mdup(ub-lb-1));
				}
				return ((this->get_last())->link(head))->link(tail);
			}
		}
	}
	return this->get_last();
}

NFA *NFA::make_or(NFA *alt)
{
	if (alt == NULL)
	{
		return this->get_last();
	}
   	NFA *begin = new NFA();
	NFA *end = new NFA();
	/* Connect begin states to given FAs via epsilon transition */
	begin->epsilon->push_back(this->get_first());
	this->get_first()->first = begin;
	begin->epsilon->push_back(alt->get_first());
	alt->get_first()->first = begin;
	/* Connect given FAs to end states via epsilon transition */
	this->get_last()->epsilon->push_back(end);
	this->get_last()->last = end;
	alt->get_last()->epsilon->push_back(end);
	alt->get_last()->last = end;
	/* Correct fist and last of begin and end */
	begin->last = end;
	end->first = begin;
	return end;
}

void NFA::remove_epsilon()
{
	if (DEBUG)
	{
		printf("remove_epsilon()-NFA size: %d\n", size());
	}
	nfa_set *queue = new nfa_set();
	nfa_set *epsilon_states = new nfa_set();
	nfa_set *to_delete = new nfa_set();
	traverse(queue);
	FOREACH_SET(queue, it)
	{
		nfa_set *eps = (*it)->epsilon_closure();
		SET_DELETE(eps, *it);			// current state has been handled last time
		epsilon_states->insert(eps->begin(), eps->end());
		FOREACH_SET(eps, it2)
		{
			FOREACH_PAIRSET((*it2)->transitions, it3)
			{
				(*it)->add_transition((*it3)->first, (*it3)->second);
			}
			(*it)->accepting->add((*it2)->accepting);
		}
		delete eps;
	}

	/* Compute the epsilon states which can be deleted (they are reached only upon epsilon transitions) */
	FOREACH_SET(epsilon_states, it)
	{
		NFA *eps_state = (*it);
		bool reachable = false;
		FOREACH_SET(queue, it2)
		{
			FOREACH_PAIRSET((*it2)->transitions, it3)
			{
				if ((*it3)->second == eps_state)
				{
					reachable = true;
					break;
				}
			}
			if (reachable)
			{
				break;
			}
		}
		if (!reachable)
		{
			to_delete->insert(eps_state);
		}
	}

	/* Delete the epsilon transitions and the transitions to states to be deleted */
	FOREACH_SET(queue, it)
	{
		NFA *state = *it;
		if (!SET_MEMBER(to_delete, state))
		{
			state->epsilon->erase(state->epsilon->begin(), state->epsilon->end());
			FOREACH_PAIRSET(state->transitions, it2)
			{
				if (SET_MEMBER(to_delete, (*it2)->second))
				{
					error("remove_epsilon:: case should not happen.");
				}
			}
		}
	}
	FOREACH_SET(to_delete, it)
	{
		(*it)->delete_only_me = 1;
		delete (*it);
	}

	delete to_delete;
	delete queue;
	delete epsilon_states;

	if (DEBUG)
	{
		printf("remove_epsilon()-NFA size: %d\n", size());
	}
}

bool NFA::same_transitions(NFA *nfa1, NFA *nfa2)
{
	FOREACH_PAIRSET(nfa1->transitions, it)
	{
	 	 if (!nfa2->has_transition((symbol_t)((*it)->first), (NFA*)((*it)->second)))
		 {
			 return false;
		 }
	}
	if (nfa1->transitions->size() != nfa2->transitions->size())
	{
		return false;
	}
	return true;
}

void NFA::remove_duplicates()
{
	nfa_set *queue = new nfa_set();
	nfa_set *to_del = new nfa_set();
	traverse(queue);				// QUEUE is the current NFA
	FOREACH_SET(queue, it)
	{						// for all the states in QUEUE
		NFA *source = *it;
		if (!SET_MEMBER(to_del, source))
		{					// if current state SOURCE is not in the set TO_DEL
			FOREACH_PAIRSET(source->transitions, it2)
			{				// for the states achievable by labeled transitions of state SOURCE
				NFA *target = (*it2)->second;
				if (!SET_MEMBER(to_del,target) && source!=target && source->accepting->equal(target->accepting) && same_transitions(source,target))
				{			// if current state TARGET is not in the set TO_DEL and equivalent to the state SOURCE
					FOREACH_SET(queue, it3)
					{
						FOREACH_PAIRSET((*it3)->transitions, it4)
						{	// make labeled transitions to state TARGET in all other states in QUEUE changed into state SOURCE
							if ((*it4)->second == target)
							{
								(*it4)->second = source;
							} // endif it4
						} // endfor it3->trans
						if (first == target)
						{
							first = source;
						}
						if (last == target)
						{
							last = source;
						}
					} // endfor queue
					target->delete_only_me = 1;
					to_del->insert(target);	// equivalent state TARGET is considered duplicated and will be deleted
				} // endif target
			} // endfor source->trans
		} // endif source
	} // endfor queue
	FOREACH_SET(to_del, it)
	{
		delete *it;
	}
	delete queue;
	delete to_del;
}

void NFA::reduce()
{
	if (DEBUG)
	{
		printf("reduce()-NFA size: %d\n", size());
	}

	remove_duplicates();
	reset_state_id();				// because the number of states is changed
	
	sub_set *mapping = new sub_set(this->id, this);	// contain mapping between DFA and NFA set of states
	nfa_list *queue = new nfa_list();		// queue of NFA states to be processed and of the set of NFA states they correspond to
	list<nfa_set*> *mapping_queue = new list<nfa_set*>();
	NFA *nfa; 
	NFA *next_nfa;
	pair_set *this_tx = new pair_set();
	nfa_set *sub_set = new nfa_set();
	sub_set->insert(this);
	queue->push_back(this);
	mapping_queue->push_back(sub_set);

	while (!queue->empty())
	{
		nfa = queue->front();
		queue->pop_front();
		sub_set = mapping_queue->front();
		mapping_queue->pop_front();
#ifdef COMMON_TARGET_REDUCTION
		/* Compute the common target and treat it in a special way */
		nfa_set *common_target = new nfa_set();
		FOREACH_SET(sub_set, it)
		{
			nfa_set *tr = (*it)->get_transitions(0);
			if (tr != NULL)
			{
				common_target->insert(tr->begin(), tr->end());
				delete tr;
			}
		}
		for (symbol_t c=1; c<CSIZE; c++)
		{
			nfa_set *target = new nfa_set();
			FOREACH_SET(sub_set, it)
			{
				nfa_set *tr = (*it)->get_transitions(c);
				if (tr != NULL)
				{
					target->insert(tr->begin(), tr->end());
					delete tr;
				}
			}
			nfa_set *ct = new nfa_set();
			FOREACH_SET(common_target, it)
			{
				if (SET_MEMBER(target, *it))
				{
					ct->insert(*it);
				}
			}
			delete common_target;
			common_target = ct;
			delete target;
		}
#endif
		/* Compute transitions not to common target */
		for (symbol_t c=0; c<CSIZE; c++)
		{
			nfa_set *target = new nfa_set();
		 	FOREACH_SET(sub_set, it)
			{
		 		nfa_set *tr = (*it)->get_transitions(c);
		 		if (tr != NULL)
				{
		 			target->insert(tr->begin(), tr->end());
		 			delete tr;
		 		}
		 	}
#ifdef COMMON_TARGET_REDUCTION 	
		 	FOREACH_SET(common_target, it)
			{
				if (SET_MEMBER(target, *it))
				{
					SET_DELETE(target, *it);
				}
			}
#endif
		 	if (!target->empty())
			{
		 		nfa_set *self_target = new nfa_set();
		 		FOREACH_SET(target, it)
				{
					if (SET_MEMBER(sub_set, *it))
					{
						self_target->insert(*it);
					}
				}
		 		FOREACH_SET(sub_set, it)
				{
					if (SET_MEMBER(target, *it))
					{
						SET_DELETE(target, *it);
					}
				}
		 		/* Self_target */
		 		if (self_target->empty())
				{
					delete self_target;
				}
		 		else
				{
		 			set<state_t> *ids = set_nfa2ids(self_target);
					bool found = mapping->lookup(ids, &next_nfa);
					delete ids;
					if (!found)
					{
						FOREACH_SET(self_target, set_it)
						{
							next_nfa->accepting->add((*set_it)->accepting);
						}
						queue->push_back(next_nfa);
						mapping_queue->push_back(self_target);
					}
					else
					{
						delete self_target;
					}
					if (nfa == this)
					{
						this_tx->insert(new pair<unsigned,NFA*>(c, next_nfa));
					}
					else nfa->add_transition(c, next_nfa);
		 		}
		 		/* Target */
		 		if (target->empty())
				{
					delete target;
				}
				else
				{
		 			set<state_t> *ids = set_nfa2ids(target);
					bool found = mapping->lookup(ids, &next_nfa);
					delete ids;
					if (!found)
					{
						FOREACH_SET(target, set_it)
						{
							next_nfa->accepting->add((*set_it)->accepting);
						}
						queue->push_back(next_nfa);
						mapping_queue->push_back(target);
					}
					else
					{
						delete target;
					}
					if (nfa == this)
					{
						this_tx->insert(new pair<unsigned,NFA*>(c, next_nfa));
					}
					else
					{
						nfa->add_transition(c, next_nfa);
					}
		 		}
		 	}
			else
			{
				delete target;
			}
		}
		if (nfa == this)
		{
			pair_set *tmp = transitions;
			transitions = this_tx;
			this_tx = tmp;
		}
#ifdef COMMON_TARGET_REDUCTION		
		/* Process the common target */
		if (!common_target->empty())
		{
	 		nfa_set *self_target = new nfa_set();
	 		FOREACH_SET(common_target, it)
			{
				if (SET_MEMBER(sub_set, *it))
				{
					self_target->insert(*it);
				}
			}
	 		FOREACH_SET(sub_set, it)
			{
				if (SET_MEMBER(common_target, *it))
				{
					SET_DELETE(common_target, *it);
				}
			}
	 		/* Self_target */
	 		if (self_target->empty())
			{
				delete self_target;
			}
	 		else
			{
	 			set<state_t> *ids = set_nfa2ids(self_target);
				bool found = mapping->lookup(ids, &next_nfa);
				delete ids;
				if (!found)
				{
					FOREACH_SET(self_target, set_it)
					{
						next_nfa->accepting->add((*set_it)->accepting);
					}
					queue->push_back(next_nfa);
					mapping_queue->push_back(self_target);
				}
				else
				{
					delete self_target;
				}
				nfa->add_any(next_nfa);
	 		}
	 		/* Common_target */
	 		if (common_target->empty())
			{
				delete common_target;
			}
	 		else
			{
	 			set<state_t> *ids = set_nfa2ids(common_target);
				bool found = mapping->lookup(ids, &next_nfa);
				delete ids;
				if (!found)
				{
					FOREACH_SET(common_target, set_it)
					{
						next_nfa->accepting->add((*set_it)->accepting);
					}
					queue->push_back(next_nfa);
					mapping_queue->push_back(common_target);
				}
				else
				{
					delete common_target;
				}
				nfa->add_any(next_nfa);
	 		}
	 	}
		else
		{
			delete common_target;
		}
#endif
		delete sub_set;
	}

	last = this;

	/* Delete the states of the old NFA */
	nfa_set *to_delete = new nfa_set();
	FOREACH_PAIRSET(this_tx, it)
	{
		if ((*it)->second != this)
		{
			((*it)->second)->traverse(to_delete);
		}
		delete (*it);
	}

	FOREACH_SET(to_delete, it)
	{
		(*it)->delete_only_me = true;
		delete *it;
	}
	delete to_delete;
	delete this_tx;

	delete mapping;
	delete queue;
	delete mapping_queue;

	reset_state_id();
	if (DEBUG)
	{
		printf("reduce()-NFA size: %d\n", size());
	}
}

void NFA::split_states()
{
	if (DEBUG)
	{
		printf("split_states() - initial NFA size: %d\n", size());
	}
	fflush(stdout);
	nfa_list *queue = new nfa_list();
	traverse(queue);
	bool exist_tx[CSIZE];
	while (!queue->empty())
	{
		NFA *nfa = queue->front();
		queue->pop_front();
		NFA *new_nfa = new NFA();
		for (int c=0; c<CSIZE; c++)
		{
			exist_tx[c] = false;
		}
		FOREACH_PAIRSET(nfa->transitions, it)
		{					// delete a state with several outgoing transitions on the same character
			if (exist_tx[(*it)->first])
			{
				new_nfa->transitions->insert(*it);
			}
			exist_tx[(*it)->first] = true;
		}
		FOREACH_PAIRSET(new_nfa->transitions, it)
		{
			SET_DELETE(nfa->transitions, *it);
		}
		if (new_nfa->transitions->empty())
		{
			delete new_nfa;
		}
		else
		{					// add a corresponding set of states connected through epsilon transitions
			nfa->epsilon->push_back(new_nfa);
			queue->push_back(new_nfa);
		}
	}
	delete queue;
	if (DEBUG)
	{
		printf("split_states() - final NFA size: %d\n", size());
	}
	reset_state_id();
	reset_depth();
	fflush(stdout);
}

/*
 * Makes a copy of the current machine and returns it
 */
NFA *NFA::make_dup()
{
	std::map<state_t,NFA *> *mapping = new std::map<state_t,NFA *>();
	NFA *begin = this->get_first();
	NFA *copy = new NFA();
	(*mapping)[begin->id] = copy;
	copy->accepting->add(begin->accepting);
	dup_state(begin, copy, mapping);
	begin->get_first()->reset_marking();
	delete mapping;
	return copy->get_last();
}

/*
 * Makes a copy of the current state and recursively of its connected states
 */
void NFA::dup_state(NFA *nfa, NFA *copy, std::map<state_t,NFA *> *mapping)
{
	if (nfa->marked)
	{
		return;
	}
	nfa->marked = 1;
	FOREACH_PAIRSET(nfa->transitions, it)
	{
		NFA *next = (*it)->second;
		NFA *next_copy = (*mapping)[next->id];
		if (next_copy == NULL)
		{
			next_copy = new NFA();
			next_copy->accepting->add(next->accepting);
			(*mapping)[next->id] = next_copy;
			next_copy->first = copy;
		}
		copy->add_transition((*it)->first, next_copy);
		if (copy->last == copy)
		{
			copy->last = next_copy;
		}
	}
	FOREACH_LIST(nfa->epsilon, it)
	{
		NFA *next = (*it);
		NFA *next_copy = (*mapping)[next->id];
		if (next_copy == NULL)
		{
			next_copy = new NFA();
			next_copy->accepting->add(next->accepting);
			(*mapping)[next->id] = next_copy;
			next_copy->first = copy;
		}
		copy->epsilon->push_back(next_copy);
		if (copy->last == copy)
		{
			copy->last = next_copy;
		}
	}

	/* And now recursion on connected nodes */
	FOREACH_PAIRSET(nfa->transitions, it)
	{
		NFA *state = (*it)->second;
		dup_state(state, (*mapping)[state->id], mapping);
	}
	FOREACH_LIST(nfa->epsilon, it)
	{
		dup_state((*it), (*mapping)[(*it)->id], mapping);
	}
}

/*
 * Makes times concatenated copy of the current machine.
 * Used when multiple repetitions of a given subpattern are required
 */
NFA *NFA::make_mdup(int times)
{
	if (times <= 0)
	{
		error("FA::make_mdup: called with times<=0");
	}
	NFA *copy = this->make_dup();
	for (int i=0; i<times-1; i++)
	{
		copy = copy->get_last()->link(this->make_dup());
	}
	return copy->get_last();
}

void NFA::reset_state_id(state_t first_id)
{
	nstate_nr = first_id;
	nfa_list *queue = new nfa_list();
	traverse(queue);
	while (!queue->empty())
	{
		NFA *nfa = queue->front();
		queue->pop_front();
		nfa->id = nstate_nr++;
	}
	delete queue;
}

void NFA::to_dot(FILE *file, const char *title)
{
	fprintf(file, "digraph \"%s\" {\n", title);
	reset_marking();
	to_dot(file);
	fprintf(file, "}\n");
	reset_marking();
}

void NFA::to_dot(FILE *file, bool blue)
{
	const char *color = (blue) ? "blue" : "black";
	marked = 1;
	/* Print node data */
	if (accepting->empty())
	{
		fprintf(file, " N%ld [shape=circle,label=\"%ld-%d\",color=\"%s\"];\n", id, id, depth, color);
	}
	else
	{ 
		fprintf(file, " N%ld [shape=doublecircle,label=\"%ld-%d/", id, id, depth);
		link_set *ls = accepting;
		while (ls != NULL)
		{
			if (ls->succ() == NULL)
			{
				fprintf(file, "%ld", ls->value());
			}
			else
			{
				fprintf(file, "%ld,",ls->value());
			}
			ls = ls->succ();
		}
		fprintf(file, "\",color=\"%s\"];\n", color);
	}
	/* Print char-transitions */
	link_set *targets = new link_set();
	link_set *curr_target = targets;
	FOREACH_TRANSITION(it)
	{
		targets->insert((*it)->second->get_id());
	}
	while (curr_target!=NULL && !curr_target->empty())
	{
		state_t target = curr_target->value();
		link_set *chars = new link_set();
		FOREACH_TRANSITION(it)
		{
			if ((*it)->second->get_id() == target)
			{
				chars->insert((unsigned char)(*it)->first);
			}
		}
		char *label = (char *)malloc(100);
		char *temp = (char *)malloc(10);
		link_set *characters = chars;
		unsigned char c = (unsigned char)characters->value();
		bool range = false;
		if ((c>='a' && c<='z') || (c>='A' && c<='Z'))
		{
			sprintf(label, "%c", c);
		}
		else
		{
			sprintf(label, "%d", c);
		}
		characters = characters->succ();
		while (characters!=NULL && !characters->empty())
		{
			if (characters->value() == (c+1))
			{
				range = true;
			}
			else
			{
				if (range)
				{
					if ((c>='a' && c<='z') || (c>='A' && c<='Z'))
					{
						sprintf(temp, "-%c", c);
					}
					else
					{
						sprintf(temp, "-%d", c);
						label = strcat(label, temp);
						range = false;
					}
				}
				if ((characters->value()>='a' && characters->value()<='z') || (characters->value()>='A' && characters->value()<='Z'))
				{
					sprintf(temp, ",%c", characters->value());
				}
				else
				{
					sprintf(temp, ",%d", characters->value());
				}
				label = strcat(label, temp);
			}
			c = characters->value();
			characters = characters->succ();
		}
		if (range)
		{
			if ((c>='a' && c<='z') || (c>='A' && c<='Z'))
			{
				sprintf(temp, "-%c", c);
			}
			else
			{
				sprintf(temp,"-%d",c);
			}
			label = strcat(label, temp);
		}
		delete chars;
		curr_target = curr_target->succ();
		fprintf(file, "N%ld -> N%ld [label=\"%s\",color=\"%s\"];\n", id, target, label, color);
		free(label);
		free(temp);
	}
	delete targets;
	/* Epsilon transition */
	FOREACH_LIST(epsilon, it)
	{
		fprintf(file, "N%ld -> N%ld [color=\"red\"];\n", id, (*it)->id);
	}
	/* Recursion on connected nodes */
	FOREACH_TRANSITION(it)
	{
		NFA *next = (*it)->second;
		if (!next->is_marked())
		{
			next->to_dot(file, blue);
		}
	}
	FOREACH_LIST(epsilon, it)
	{
		if (!(*it)->is_marked())
		{
			(*it)->to_dot(file, blue);
		}
	}
}
