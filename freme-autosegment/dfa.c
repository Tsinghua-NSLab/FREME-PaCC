#include "dfa.h"
#include "nfa.h"

DFA::DFA(unsigned N)
{
	dead_state = NO_STATE;
	entry_allocated = N;
	_size = 0;
	state_table = allocate_state_matrix(entry_allocated);
	accepted_rules = (link_set **)allocate_array(entry_allocated, sizeof(link_set *));
	marking = allocate_int_array(entry_allocated);
	for (state_t s=0; s<entry_allocated; s++)
	{
		state_table[s] = NULL;
		accepted_rules[s] = NULL;
	}
}

DFA::~DFA()
{
	for (state_t i=0; i<_size; i++)
	{
		free(state_table[i]);
		if (accepted_rules[i])
		{
			delete accepted_rules[i];
		}
	}
	free(state_table);
	free(accepted_rules);
	free(marking);
}

bool DFA::equal(DFA *dfa)
{
	if (_size != dfa->_size)
	{
		return false;
	}
	for (state_t s=0; s<_size; s++)
	{
		for (int c=0; c<CSIZE; c++)
		{
			if (state_table[s][c] != dfa->state_table[s][c])
			{
				return false;
			}
		}
		if ((accepted_rules[s]==NULL && (dfa->accepted_rules[s]!=NULL && !dfa->accepted_rules[s]->empty())) ||
			(dfa->accepted_rules[s]==NULL && (accepted_rules[s]!=NULL && !accepted_rules[s]->empty())) ||
			!accepted_rules[s]->equal(dfa->accepted_rules[s]))
			return false;
	}
	return true;
}

state_t DFA::add_state()
{
	state_t state = _size++;
	if (DEBUG && state%1000==0 && state!=0)
	{
		printf("... state = %ld\n", state);
		fflush(stdout);
	}
	if (state == NO_STATE)
	{
		error("DFA:: add_state(): too many states!");
	}
	if (state >= entry_allocated)
	{
		entry_allocated += MAX_DFA_SIZE_INCREMENT;
		state_table = reallocate_state_matrix(state_table, entry_allocated);
		accepted_rules = (link_set **)reallocate_array(accepted_rules, entry_allocated, sizeof(link_set *));
		marking = reallocate_int_array(marking, entry_allocated);
		for (state_t s=_size; s<entry_allocated; s++)
		{
			state_table[s] = NULL;
			accepted_rules[s] = NULL;
		}
	}
	state_table[state] = allocate_state_array(CSIZE);
	accepted_rules[state] = new link_set();
	marking[state] = false;
	int i;
	for (i=0; i<CSIZE; i++)
	{
		state_table[state][i] = NO_STATE;
	}
	return state;
}

void DFA::dump(FILE *state_trans, FILE *state_attach)
{
	fprintf(state_trans, "%u\n", _size);
	for (state_t i=0; i<_size; i++)
	{
		for (unsigned int j=0; j<CSIZE-1; j++)
		{
			fprintf(state_trans, "%ld\t", state_table[i][j]);
		}
		fprintf(state_trans, "%ld\n", state_table[i][CSIZE-1]);

		int dead = 1;
		for (unsigned int j=0; j<CSIZE; j++)
		{
			if (state_table[i][j] != i)
			{
				dead = 0;
				break;
			}
		}
		fprintf(state_attach, "%d", dead);

		fprintf(state_attach, "\t%d", accepted_rules[i]->size());
		if (!accepted_rules[i]->empty())
		{
			fprintf(state_attach, "\t@");
			link_set *s = accepted_rules[i];
			while (s != NULL)
			{
				fprintf(state_attach, "\t%d", s->value());
				s=s->succ();
			}
		}
		fprintf(state_attach, "\n");
	}
	/*
	fprintf(log, "Dumping DFA: %ld states\n", _size);
	for (state_t i=0; i<_size; i++)
	{
		fprintf(log, "> state # %d", i);
		if (!accepted_rules[i]->empty())
		{
			fprintf(log, " accepts ");
			link_set *s = accepted_rules[i];
			while (s != NULL)
			{
				fprintf(log, "[%ld]", s->value()); s=s->succ();
			}
		}
		fprintf(log, "\n");
		for (int j=0; j<CSIZE; j++)
		{
			if (state_table[i]!=NULL && state_table[i][j]!=0)
			{
				fprintf(log, " - [%d/%c] %d\n", j, j, state_table[i][j]);
			}
		}
	}
	*/
}

void DFA::minimize()
{
	if (VERBOSE)
	{
		fprintf(stderr, "DFA:: minimize: before minimization states = %ld\n", _size);
	}

	unsigned long i;

	// the algorithm needs the DFA to be total, so we add an error state 0,
	// and translate the rest of the states by +1
	unsigned int n = _size + 1;

	// block information:
	// [0..n-1] stores which block a state belongs to,
	// [n..2*n-1] stores how many elements each block has
	int block[2*n];
	for (i=0; i<2*n; i++)
	{
		block[i] = 0;
	}

	// implements a doubly linked list of states (these are the actual blocks)
	int b_forward[2*n];
	for (i=0; i<2*n; i++)
	{
		b_forward[i] = 0;
	}
	int b_backward[2*n];
	for (i=0; i<2*n; i++)
	{
		b_backward[i] = 0;
	}

	// the last of the blocks currently in use (in [n..2*n-1])
	// (end of list marker, points to the last used block)
	int lastBlock = n;		// at first we start with one empty block
	int b0 = n;			// the first block

	// the circular doubly linked list L of pairs (B_i, c)
	// (B_i, c) in L if l_forward[(B_i-n)*CSIZE+c] > 0 (B_i-th state, the c-th transition)
	// numeric value of block 0 = n!
	int *l_forward = allocate_int_array(n*CSIZE+1);
	for (i=0; i<n*CSIZE+1; i++)
	{
		l_forward[i] = 0;
	}
	int *l_backward = allocate_int_array(n*CSIZE+1);
	for (i=0; i<n*CSIZE+1; i++)
	{
		l_backward[i] = 0;
	}

	int anchorL = n*CSIZE;		// list anchor

	// inverse of the transition state_table
	// if t = inv_delta[s][c] then { inv_delta_set[t], inv_delta_set[t+1], ... inv_delta_set[k] }
	// is the set of states, with inv_delta_set[k] = -1 and inv_delta_set[j] >= 0 for t <= j < k  
	int *inv_delta[n];
	for (i=0; i<n; i++)
	{
		inv_delta[i] = allocate_int_array(CSIZE);
	}
	int *inv_delta_set = allocate_int_array(2*n*CSIZE);

	// twin stores two things: 
	// twin[0], ... twin[numSplit-1] is the list of blocks that have been spliter
	// twin[B_i] is the twin of block B_i
	int twin[2*n];
	int numSplit;

	// SD[B_i] is the number of states s in B_i with delta(s,a) in B_j
	// if SD[B_i] == block[B_i], there is no need to spliter
	int SD[2*n];			// [only SD[n..2*n-1] is used]

	// for fixed (B_j,a), the D[0]..D[numD-1] are the inv_delta(B_j,a)
	int D[n];
	int numD;

	// initialize inverse of transition state_table
	int lastDelta = 0;
	int inv_lists[n];		// holds a set of lists of states
	int inv_list_last[n];		// the last element

	int c,s;

	for (s=0; s<n; s++)
	{
		inv_lists[s] = -1;
	}

	for (c=0; c<CSIZE; c++)
	{
		// clear "head" and "last element" pointers
		for (s=0; s<n; s++)
		{
			inv_list_last[s] = -1;
			inv_delta[s][c] = -1;
		}

		// the error state has a transition for each character into itself
		inv_delta[0][c] = 0;
		inv_list_last[0] = 0;

		// accumulate states of inverse delta into lists (inv_delta serves as head of list)
		for (s=1; s<n; s++)
		{
			int t = state_table[s-1][c] + 1;	// check this "+1"

			if (inv_list_last[t] == -1)		// if there are no elements in the list yet
			{
				inv_delta[t][c] = s;		// mark t as first and last element
				inv_list_last[t] = s;
			}
			else
			{
				inv_lists[inv_list_last[t]] = s;// link t into chain
				inv_list_last[t] = s;		// and mark as last element
			}
		}

		// now move them to inv_delta_set in sequential order, 
		// and update inv_delta accordingly
		for (int s=0; s<n; s++)
		{
			int i = inv_delta[s][c];
			inv_delta[s][c] = lastDelta;
			int j = inv_list_last[s];
			bool go_on = (i != -1);
			while (go_on)
			{
				go_on = (i != j);
				inv_delta_set[lastDelta++] = i;
				i = inv_lists[i];
			}
			inv_delta_set[lastDelta++] = -1;
		}
	} // endfor initialize inv_delta


	// initialize blocks 
	// make b0 = {0}  where 0 = the additional error state
	b_forward[b0] = 0;
	b_backward[b0] = 0;          
	b_forward[0] = b0;
	b_backward[0] = b0;
	block[0] = b0;
	block[b0] = 1;

	for (int s=1; s<n; s++)
	{
		// search the blocks if it fits in somewhere
		// (fit in = same pushback behavior, same finalness, same lookahead behavior, same action)
		int b = b0 + 1;		// no state can be equivalent to the error state
		bool found = false;
		while (!found && b <= lastBlock)
		{
			// get some state out of the current block
			int t = b_forward[b];

			// check, if s could be equivalent with t
			found = true;	// (isPushback[s-1] == isPushback[t-1]) && (isLookEnd[s-1] == isLookEnd[t-1]);
			if (found)
			{
				found = accepted_rules[s-1]->equal(accepted_rules[t-1]); 	

				if (found)	// found -> add state s to block b
				{
					// fprintf(stderr,"Found! [%d,%d] Adding to block %d\n",s-1,t-1,(b-b0));
					// update block information
					block[s] = b;
					block[b]++;

					// chain in the new element
					int last = b_backward[b];
					b_forward[last] = s;
					b_forward[s] = b;
					b_backward[b] = s;
					b_backward[s] = last;
				}
			}
			b++;
		}

		if (!found)			// fits in nowhere -> create new block
		{
			// update block information
			block[s] = b;
			block[b]++;

			// chain in the new element
			b_forward[b] = s;
			b_forward[s] = b;
			b_backward[b] = s;
			b_backward[s] = b;

			lastBlock++;
		}
	} // endfor initialize blocks

	// initialize worklist L
	// first, find the largest block B_max, then, all other (B_i,c) go into the list
	int B_max = b0;
	int B_i;
	for (B_i=b0+1; B_i<=lastBlock; B_i++)
	{
		if (block[B_max] < block[B_i])
		{
			B_max = B_i;
		}
	}
	// L = empty
	l_forward[anchorL] = anchorL;
	l_backward[anchorL] = anchorL;

	// set up the first list element
	if (B_max == b0)
	{
		B_i = b0 + 1;
	}
	else
	{
		B_i = b0;	// there must be at least two blocks
	}

	int index = (B_i-b0)*CSIZE;	// (B_i, 0)
	while (index < (B_i+1-b0)*CSIZE)
	{
		int last = l_backward[anchorL];
		l_forward[last] = index;
		l_forward[index] = anchorL;
		l_backward[index] = last;
		l_backward[anchorL] = index;
		index++;
	}

	// now do the rest of L
	while (B_i <= lastBlock)
	{
		if (B_i != B_max)
		{
			index = (B_i-b0)*CSIZE;
			while (index < (B_i+1-b0)*CSIZE)
			{
				int last = l_backward[anchorL];
				l_forward[last] = index;
				l_forward[index] = anchorL;
				l_backward[index] = last;
				l_backward[anchorL] = index;
				index++;
			}
		}
		B_i++;
	}
	// end of setup L

	// start of "real" algorithm
	// while L not empty
	while (l_forward[anchorL] != anchorL)
	{
		// pick and delete (B_j, a) in L:
		// pick
		int B_j_a = l_forward[anchorL];
		// delete
		l_forward[anchorL] = l_forward[B_j_a];
		l_backward[l_forward[anchorL]] = anchorL;
		l_forward[B_j_a] = 0;
		// take B_j_a = (B_j-b0)*CSIZE+c apart into (B_j, a)
		int B_j = b0 + B_j_a / CSIZE;
		int a = B_j_a % CSIZE;
		// determine splittings of all blocks wrt (B_j, a)
		// i.e. D = inv_delta(B_j,a)
		numD = 0;
		int s = b_forward[B_j];
		while (s != B_j)
		{
			int t = inv_delta[s][a];
			while (inv_delta_set[t] != -1)
			{
				D[numD++] = inv_delta_set[t++];
			}
			s = b_forward[s];
		}

		// clear the twin list
		numSplit = 0;

		// clear SD and twins (only those B_i that occur in D)
		for (int indexD=0; indexD<numD; indexD++)	// for each s in D
		{
			s = D[indexD];
			B_i = block[s];
			SD[B_i] = -1; 
			twin[B_i] = 0;
		}

		// count how many states of each B_i occuring in D go with a into B_j
		// Actually we only check, if *all* t in B_i go with a into B_j.
		// In this case SD[B_i] == block[B_i] will hold.
		for (int indexD=0; indexD<numD; indexD++)// for each s in D
		{
			s = D[indexD];
			B_i = block[s];

			// only count, if we haven't checked this block already
			if (SD[B_i] < 0)
			{
				SD[B_i] = 0;
				int t = b_forward[B_i];
				while (t != B_i && (t != 0 || block[0] == B_j) && 
				      (t == 0 || block[state_table[t-1][a]+1] == B_j))
				{
					SD[B_i]++;
					t = b_forward[t];
				}
			}
		}

		// spliter each block according to D      
		for (int indexD=0; indexD<numD; indexD++)// for each s in D
		{
			s = D[indexD];
			B_i = block[s];

			if (SD[B_i] != block[B_i])
			{
				int B_k = twin[B_i];
				if (B_k == 0)
				{ 
					// no twin for B_i yet -> generate new block B_k, make it B_i's twin
					B_k = ++lastBlock;
					b_forward[B_k] = B_k;
					b_backward[B_k] = B_k;
					
					twin[B_i] = B_k;

					// mark B_i as spliter
					twin[numSplit++] = B_i;
				}
				// move s from B_i to B_k

				// remove s from B_i
				b_forward[b_backward[s]] = b_forward[s];
				b_backward[b_forward[s]] = b_backward[s];

				// add s to B_k
				int last = b_backward[B_k];
				b_forward[last] = s;
				b_forward[s] = B_k;
				b_backward[s] = last;
				b_backward[B_k] = s;

				block[s] = B_k;
				block[B_k]++;
				block[B_i]--;

				SD[B_i]--;	// there is now one state less in B_i that goes with a into B_j
			}
		} // endfor block splitting

		// update L
		for (int indexTwin=0; indexTwin<numSplit; indexTwin++)
		{
			B_i = twin[indexTwin];
			int B_k = twin[B_i];
			for (int c=0; c<CSIZE; c++)
			{
				int B_i_c = (B_i-b0)*CSIZE+c;
				int B_k_c = (B_k-b0)*CSIZE+c;
				if (l_forward[B_i_c] > 0)
				{
					// (B_i,c) already in L --> put (B_k,c) in L
					int last = l_backward[anchorL];
					l_backward[anchorL] = B_k_c;
					l_forward[last] = B_k_c;
					l_backward[B_k_c] = last;
					l_forward[B_k_c] = anchorL;
				}
				else
				{
					// put the smaller block in L
					if (block[B_i] <= block[B_k])
					{
						int last = l_backward[anchorL];
						l_backward[anchorL] = B_i_c;
						l_forward[last] = B_i_c;
						l_backward[B_i_c] = last;
						l_forward[B_i_c] = anchorL;
					}
					else
					{
						int last = l_backward[anchorL];
						l_backward[anchorL] = B_k_c;
						l_forward[last] = B_k_c;
						l_backward[B_k_c] = last;
						l_forward[B_k_c] = anchorL;
					}
				}
			}
		}
	}

	// transform the transition state_table 

	// trans[i] is the state j that will replace state i, i.e. 
	// states i and j are equivalent
	int trans[_size];

	// kill[i] is true iff state i is redundant and can be removed
	bool kill[_size];

	// move[i] is the amount line i has to be moved in the transition state_table
	// (because states j < i have been removed)
	int move[_size];

	// fill arrays trans[] and kill[] (in O(n))
	for (int b=b0+1; b<=lastBlock; b++)	// b0 contains the error state
	{
		// get the state with smallest value in current block
		int s = b_forward[b];
		int min_s = s; // there are no empty blocks!
		for (; s!=b; s=b_forward[s])
		{
			if (min_s > s)
			{
				min_s = s;
			}
		}
		// now fill trans[] and kill[] for this block
		// (and translate states back to partial DFA)
		min_s--;
		for (s=b_forward[b]-1; s!=b-1; s=b_forward[s+1]-1)
		{
			trans[s] = min_s;
			kill[s] = s != min_s;
		}
	}
	// fill array move[] (in O(n))
	int amount = 0;
	for (int i=0; i<_size; i++)
	{
		if (kill[i])
		{
			amount++;
		}
		else
		{
			move[i] = amount;
		}
	}

	int j;
	// j is the index in the new transition state_table
	// the transition state_table is transformed in place (in O(c n))
	for (i=0,j=0; i<_size; i++)
	{
		// we only copy lines that have not been removed
		if (!kill[i])
		{
			// translate the target states
			int c;
			for (c=0; c<CSIZE; c++)
			{
				if (state_table[i][c] >= 0)
				{
					state_table[j][c] = trans[state_table[i][c]];
					state_table[j][c] -= move[state_table[j][c]];
				}
				else
				{
					state_table[j][c] = state_table[i][c];
				}
			}

			if (i != j)
			{
				accepted_rules[j]->clear();
				accepted_rules[j]->add(accepted_rules[i]);
			}
			j++;
		}
	}

	_size = j;

	// free arrays
	free(l_forward);
	free(l_backward);
	free(inv_delta_set);
	for(int i=0; i<n; i++)
	{
		free(inv_delta[i]);
	}

	// free unused memory in the DFA
	for (state_t s=_size; s<entry_allocated; s++)
	{
		if (state_table[s] != NULL)
		{
			free(state_table[s]);
			state_table[s] = NULL;
		}
		if (accepted_rules[s] != NULL)
		{
			delete accepted_rules[s];
			accepted_rules[s] = NULL;
		}
	}
	state_table = reallocate_state_matrix(state_table, _size);
	accepted_rules = (link_set **)reallocate_array(accepted_rules, _size, sizeof(link_set*));
	marking = reallocate_int_array(marking, _size);
	entry_allocated = _size;

	if (VERBOSE)
	{
		fprintf(stderr, "DFA:: minimize: after minimization states = %ld\n", _size);
	}
}

void DFA::to_dot(FILE *file, const char *title)
{
	set_depth();
	fprintf(file, "digraph \"%s\" {\n", title);
	for (state_t s=0; s<_size; s++)
	{
		if (accepted_rules[s]->empty())
		{
			fprintf(file, " %ld [shape=circle,label=\"%ld", s, s);
			fprintf(file, "-%ld\"];\n", depth[s]);
		}
		else
		{
			fprintf(file, " %ld [shape=doublecircle,label=\"%ld", s, s);
			fprintf(file, "-%ld/", depth[s]);
			link_set *ls = accepted_rules[s];
			while (ls != NULL)
			{
				if (ls->succ() == NULL)
				{
					fprintf(file, "%ld", ls->value());
				}
				else
				{
					fprintf(file, "%ld,", ls->value());
				}
				ls = ls->succ();
			}
			fprintf(file, "\"];\n");
		}
	}
	int *mark = allocate_int_array(CSIZE);
	char *label = NULL;
	char *temp = (char *)malloc(100);
	state_t target = NO_STATE;
	for (state_t s=0; s<_size; s++)
	{
		for (int i=0; i<CSIZE; i++)
		{
			mark[i] = 0;
		}
		for (int c=0; c<CSIZE; c++)
		{
			if (!mark[c])
			{
				mark[c] = 1;
				if (state_table[s][c] != 0)
				{
					target = state_table[s][c];
					label = (char *)malloc(100);
					if ((c>='a' && c<='z') || (c>='A' && c<='Z'))
					{
						sprintf(label, "%c", c);
					}
					else
					{
						sprintf(label, "%d", c);
					}
					bool range = true;
					int begin_range = c;
					for (int d=c+1; d<CSIZE; d++)
					{
						if (state_table[s][d] == target)
						{
							mark[d] = 1;
							if (!range)
							{
								if ((d>='a' && d<='z') || (d>='A' && d<='Z'))
								{
									sprintf(temp, "%c", d);
								}
								else
								{
									sprintf(temp, "%d", d);
								}
								label = strcat(label, ",");
								label = strcat(label, temp);
								begin_range = d;
								range = 1;
							}
						}
						if (range && (state_table[s][d]!=target || d==CSIZE-1))
						{
							range = false;
							if (begin_range != d-1)
							{
								if (state_table[s][d] != target)
								{
									if ((d-1>='a' && d-1<='z') || (d-1>='A' && d-1<='Z'))
									{
										sprintf(temp, "%c", d-1);
									}
									else
									{
										sprintf(temp, "%d", d-1);
									}
								}
								else
								{
									if ((d>='a' && d<='z') || (d>='A' && d<='Z'))
									{
										sprintf(temp, "%c", d);
									}
									else
									{
										sprintf(temp, "%d", d);
									}
								}
								label = strcat(label, "-");
								label = strcat(label, temp);
							}
						}
					}
				}
			}
			if (label != NULL)
			{
				fprintf(file, "%ld -> %ld [label=\"%s\"];\n", s, target, label);
				free(label);
				label = NULL;
			}
		}
	}
	free(temp);
	free(mark);
	fprintf(file, "}");
}

void DFA::set_depth()
{
	std::list<state_t> *queue = new std::list<state_t>();
	if (depth == NULL)
	{
		depth = allocate_uint_array(_size);
	}
	for (state_t s=1; s<_size; s++)
	{
		depth[s] = 0xFFFFFFFF;
	}
	depth[0] = 0;
	queue->push_back(0);
	while (!queue->empty())
	{
		state_t s = queue->front();
		queue->pop_front();
		for (unsigned int c=0; c<CSIZE; c++)
		{
			if (depth[state_table[s][c]] == 0xFFFFFFFF)
			{
				depth[state_table[s][c]] = depth[s] + 1;
				queue->push_back(state_table[s][c]);
			}
		}
	}
	delete queue;
}

state_t DFA::get_dead_state()
{
	death = NO_STATE;
	for (state_t s=0; s<_size; s++)
	{
		bool dead = true;
		for (unsigned int c=0; c<CSIZE; c++)
		{
			if (state_table[s][c] != s)
			{
				dead = false;
				break;
			}
		}
		if (dead)
		{
			death = s;
			break;
		}
	}
	return death;
}

void DFA::generate(int ndfa)
{
	int dead, accepted;
	symbol_t c;
	FILE *state_trans = NULL, *state_attach = NULL;
	switch (ndfa)
	{
		case 0:
			state_trans = fopen("./result/DFA1_TRANS.data", "r");
			state_attach = fopen("./result/DFA1_MATCH.data", "r");
			break;
		case 1:
			state_trans = fopen("./result/DFA2_TRANS.data", "r");
			state_attach = fopen("./result/DFA2_MATCH.data", "r");
			break;
		case 2:
			state_trans = fopen("./result/DFA3_TRANS.data", "r");
			state_attach = fopen("./result/DFA3_MATCH.data", "r");
			break;
	}
	fscanf(state_trans, "%u\n", &_size);
	state_table = allocate_state_matrix(_size);
	accepted_rules = (link_set **)allocate_array(_size, sizeof(link_set *));

	for (state_t i=0; i<_size; i++)
	{
		state_table[i]=allocate_state_array(CSIZE);
		for (unsigned int j=0; j<CSIZE-1; j++)
		{
			fscanf(state_trans, "%ld\t", &state_table[i][j]);
		}
		fscanf(state_trans, "%ld\n", &state_table[i][CSIZE-1]);

		fscanf(state_attach, "%d", &dead);

		fscanf(state_attach, "\t%d", &accepted);
		accepted_rules[i] = new link_set();
		if (accepted > 0)
		{
			fscanf(state_attach, "\t@");
			for (int j=0; j<accepted; j++)
			{
				fscanf(state_attach, "\t%d", &c);
				accepted_rules[i]->insert(c);
			}
		}
		fscanf(state_attach, "\n");
	}
	fclose(state_attach);
	fclose(state_trans);
}
