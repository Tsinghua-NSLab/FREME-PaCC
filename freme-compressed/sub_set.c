#include "sub_set.h"
#include "dfa.h"

sub_set::sub_set(state_t id, NFA *nfa)
{
	nfa_id = id;
	dfa_id = NO_STATE;
	next_o = NULL;
	next_v = NULL;
	nfa_obj = nfa;
}

sub_set::~sub_set()
{
	if (next_o != NULL)
	{
		delete next_o;
	}
	if (next_v != NULL)
	{
		delete next_v;
	}
}

bool sub_set::lookup(set<state_t> *nfaid_set, DFA *dfa, state_t *dfaid)
{
	if (nfaid_set->empty())
	{
		nfaid_set->insert(NO_STATE);
	}
	/* For all the NFA state-ID(s) in queried nfaid_set */
	state_t id = *(nfaid_set->begin());
	if (id == nfa_id)
	{
		nfaid_set->erase(nfaid_set->begin());
		if (nfaid_set->empty())
		{			// if all the NFA state-ID(s) are already queried
			if (dfa_id == NO_STATE)
			{		// but the resulting DFA state-ID is none, then create a new DFA state, indicating that lookup fails
				dfa_id = dfa->add_state();
				*dfaid = dfa_id;
				return false;
			}
			else
			{		// or the resulting DFA state-ID already exists, then return the current DFA state-ID, indicating that lookup succeeds
				*dfaid = dfa_id;
				return true;
			}
		}
		else
		{			// continue to select the next NFA state-ID in queried nfaid_set
			id = *(nfaid_set->begin());
			if (next_o == NULL)
			{		// if none next HORIZONTAL subset for present subset MAPPING (e.g., MAPPING->lookup(nfaid_set, dfa, dfaid)), then add the state-ID 'id' in present subset MAPPING. In fact, as following cycles, all state-ID(s) in nfaid_set will be added in present subset MAPPING, and a corresponding DFA state will be generated at last
				next_o = new sub_set(id);
			}
			else if (next_o->nfa_id > id)
			{		// if next HORIZONTAL subset exists but larger than state-ID 'id', then change the next HORIZONTAL subset into the VERTICAL subset, and add the state-ID 'id' as the next HORIZONTAL subset in present subset MAPPING
				sub_set *new_node = new sub_set(id);
				new_node->next_v = next_o;
				next_o = new_node;
			}		// after above evaluation, or already next_o->nfa_id<=id, do recursive lookup for all following HORIZONTAL subset
			return next_o->lookup(nfaid_set, dfa, dfaid);
		}
	}
	else if (id > nfa_id)
	{				// for above next_o->nfa_id<id
		if (next_v==NULL || id<next_v->nfa_id)
		{			// HORIZONTAL lookup cannot go ahead, none or smaller subset is in the VERTICAL dimension, then lookup for the next VERTICAL subset, notice that no NFA state-ID(s) in nfaid_set is removed during this turn
			sub_set *new_node = new sub_set(id);
			new_node->next_v = next_v;
			next_v = new_node;
		}			// after above evaluation, or already next_v->nfa_id>=id, do recursive lookup for all following HORIZONTAL subset
		return next_v->lookup(nfaid_set, dfa, dfaid);
	}
	else
	{
		error("sub_set::lookup: condition should never occur");
	}
}

bool sub_set::lookup(set<state_t> *nfaid_set, NFA **nfa)
{
	if (nfaid_set->empty())
	{
		error("sub_set:: NFA lookup with empty set");
	}
	/* For all the NFA state-ID(s) in queried nfaid_set */
	state_t id = *(nfaid_set->begin());
	if (id == nfa_id)
	{
		nfaid_set->erase(nfaid_set->begin());
		if (nfaid_set->empty())
		{			// if all the NFA state-ID(s) are already queried
			if (nfa_obj == NULL)
			{		// but the resulting NFA state is none, then create a new NFA state, indicating that lookup fails
				nfa_obj = new NFA();
				*nfa = nfa_obj;
				return false;
			}
			else
			{		// or the resulting NFA state already exists, then return the current NFA state, indicating that lookup succeeds
				*nfa = nfa_obj;
				return true;
			}
		}
		else
		{			// continue to select the next NFA state-ID in queried nfaid_set
			id = *(nfaid_set->begin());
			if (next_o == NULL)
			{		// if none next HORIZONTAL subset for present subset MAPPING (e.g., MAPPING->lookup(nfaid_set, nfa)), then add the state-ID 'id' in present subset MAPPING. In fact, as following cycles, all state-ID(s) in nfaid_set will be added in present subset MAPPING, and a corresponding NFA state will be generated at last
				next_o = new sub_set(id);
			}
			else if (next_o->nfa_id > id)
			{		// if next HORIZONTAL subset exists but larger than state-ID 'id', then change the next HORIZONTAL subset into the VERTICAL subset, and add the state-ID 'id' as the next HORIZONTAL subset in present subset MAPPING
				sub_set *new_node = new sub_set(id);
				new_node->next_v = next_o;
				next_o = new_node;
			}		// after above evaluation, or already next_o->nfa_id<=id, do recursive lookup for all following HORIZONTAL subset
			return next_o->lookup(nfaid_set, nfa);
		}
	}
	else if (id > nfa_id)
	{				// for above next_o->nfa_id<id
		if (next_v==NULL || id<next_v->nfa_id)
		{			// HORIZONTAL lookup cannot go ahead, none or smaller subset is in the VERTICAL dimension, then lookup for the next VERTICAL subset, notice that no NFA state-ID(s) in nfaid_set is removed during this turn
			sub_set *new_node = new sub_set(id);
			new_node->next_v = next_v;
			next_v = new_node;
		}
					// after above evaluation, or already next_v->nfa_id>=id, do recursive lookup for all following HORIZONTAL subset
		return next_v->lookup(nfaid_set, nfa);
	}
	else
	{
		error("sub_set::lookup: condition should never occur");
	}
}
