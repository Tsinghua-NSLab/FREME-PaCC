#include "trace.h"

#define MAX_THREAD_NUM	3

link_set *global_accepted_rules = new link_set();

bool index_close[4096];

int _TSIS_[4096][3], thread_num = 0;

state_t _DEAD_[MAX_DFAS_NUM] = {NO_STATE};

pthread_spinlock_t key;

pthread_mutex_t return_key[MAX_THREAD_NUM];

unsigned long fsize = 0, xsize = 0, ysize = 0, zsize = 0;

unsigned char *buffer = NULL;

DFA **_DFAS_ = NULL;

queue<run_state_unit_t *> *_WORK_QUEUE_ = new queue<run_state_unit_t *>();

void *pXthread(void *)
{
	if (pthread_spin_lock(&key) != 0)
	{
		printf("Lock key error!\n");
		return (void *)(-1);
	}
	int id = thread_num;
	thread_num++;
	if (pthread_spin_unlock(&key) != 0)
	{
		printf("Unlock key error!\n");
		return (void *)(-1);
	}
	printf("thread %d creates\n", id);
	link_set *accepted_rules = NULL;
	while (true)
	{
		if (pthread_mutex_lock(&return_key[id]) != 0)
		{
			printf("Lock key error!\n");
			return (void *)(-1);
		}
		link_set *updaterule = NULL, *createrule[MAX_DFAS_NUM] = {NULL};
		int rule, ndfa;
		bool mark = false;
		state_t state;
		unsigned char c;
		run_state_unit_t *init;
		while (true)
		{
			if (pthread_spin_lock(&key) != 0)
			{
				printf("Lock key error!\n");
				return (void *)(-1);
			}
			//printf("queue size = %d, thread_num = %d\n", _WORK_QUEUE_->size(), id);
			if (!_WORK_QUEUE_->empty())
			{
				init = _WORK_QUEUE_->front();
				_WORK_QUEUE_->pop();
				mark = false;
			}
			else
			{
				mark = true;
			}
			if (pthread_spin_unlock(&key) != 0)
			{
				printf("Unlock key error!\n");
				return (void *)(-1);
			}
			//printf("state %u + input %ld in DFA%d\n", state, init->cptr, init->ndfa);
			if (mark)
			{
				break;
			}
			state = 0;
			while (init->cptr < fsize)
			{
				c = buffer[init->cptr];
				//printf("state %u + input %ld in DFA%d", state, init->cptr, init->ndfa);
				if (!mark)
				{
					state = _DFAS_[init->ndfa]->get_next_state(state, c);
				}
				else
				{
					mark = false;
				}
				if (state == _DEAD_[init->ndfa])
				{
					break;
				}
				link_set *x = _DFAS_[init->ndfa]->accepts(state);
				//printf(" --> state %u\n", state);
				if (!x->empty())
				{
					while (x != NULL)
					{
						rule = x->value() - 1;
						if (init->anterule->size() > xsize)
						{
							xsize = init->anterule->size();
							printf("anterule size = %ld, ysize = %ld, zsize = %ld\n", init->anterule->size(), ysize, zsize);
						}
						ysize++;
						if (!index_close[rule] && init->anterule->member(_TSIS_[rule][0]))
						{
							zsize++;
							if (_TSIS_[rule][2] > 0)
							{
								//printf("matched rule = %d\n", _TSIS_[rule][2]);
								if (accepted_rules == NULL)
									accepted_rules = new link_set(_TSIS_[rule][2]);
								else
									accepted_rules->insert(_TSIS_[rule][2]);
							}
							else
							{
								//printf("state = %u, anterule = %d\n", state, rule + 1);
								if (!mark)
								{
									state = _DFAS_[init->ndfa]->get_next_state(state, buffer[init->cptr+1]);
									mark = true;
								}
								ndfa = _TSIS_[rule][1] - 1;
								if (state == 0 && ndfa == init->ndfa)
								{
									if (updaterule == NULL)
										updaterule = new link_set(rule + 1);
									else
										updaterule->insert(rule + 1);
								}
								else if (ndfa == init->ndfa && state == _DFAS_[init->ndfa]->get_next_state(0, buffer[init->cptr+1]))
								{
									if (updaterule == NULL)
										updaterule = new link_set(rule + 1);
									else
										updaterule->insert(rule + 1);
								}
								else
								{
									if (createrule[ndfa] == NULL)
										createrule[ndfa] = new link_set(rule + 1);
									else
										createrule[ndfa]->insert(rule + 1);
								}
								if (ndfa == 0)
								{
									if (pthread_spin_lock(&key) != 0)
									{
										printf("Lock key error!\n");
										return (void *)(-1);
									}
									index_close[rule] = true;
									if (pthread_spin_unlock(&key) != 0)
									{
										printf("Unlock key error!\n");
										return (void *)(-1);
									}
								}

							}
						}
						x = x->succ();
					}
				}
				for (ndfa=0; ndfa<MAX_DFAS_NUM; ndfa++)
				{
					if (createrule[ndfa] != NULL)
					{
						run_state_unit_t *unit_first = new run_state_unit_t();
						unit_first->cptr = init->cptr + 1;
						unit_first->ndfa = ndfa;
						unit_first->anterule = createrule[ndfa];
						if (pthread_spin_lock(&key) != 0)
						{
							printf("Lock key error!\n");
							return (void *)(-1);
						}
						_WORK_QUEUE_->push(unit_first);
						if (pthread_spin_unlock(&key) != 0)
						{
							printf("Lock key error!\n");
							return (void *)(-1);
						}
						createrule[ndfa] = NULL;
					}
				}
				if (updaterule != NULL)
				{
					init->anterule->add(updaterule);
					delete updaterule;
					updaterule = NULL;
				}
				init->cptr++;
			}
			delete init->anterule;
			delete init;
		}
		if (pthread_mutex_unlock(&return_key[id]) != 0)
		{
			printf("Unlock key error!\n");
			return (void *)(-1);
		}
		bool reboot_mark = false;
		for (int i=0; i<MAX_THREAD_NUM; i++)
		{
			while (pthread_mutex_trylock(&return_key[i]) != 0)
			{
				if (!_WORK_QUEUE_->empty())
				{
					reboot_mark = true;
					break;
				}
			}
			if (reboot_mark)
			{
				break;
			}
			else
			{
				if (pthread_mutex_unlock(&return_key[i]) != 0)
				{
					printf("Unlock key error!\n");
					return (void *)(-1);
				}
			}
		}
		if (reboot_mark) continue;
		printf("Thread %d exits\n", id);
		break;
	}
	if (accepted_rules != NULL)
	{
		if (pthread_spin_lock(&key) != 0)
		{
			printf("Lock key error!\n");
			return (void *)(-1);
		}
		global_accepted_rules->add(accepted_rules);
		if (pthread_spin_unlock(&key) != 0)
		{
			printf("Unlock key error!\n");
			return (void *)(-1);
		}
		delete accepted_rules;
	}
	return (void *)(0);
}

trace::trace(char *filename)
{
	tracefile = NULL;
	if (filename != NULL)
	{
		set_trace(filename);
	}
	else
	{
		tracename = NULL;
	}
}

trace::~trace()
{
	if (tracefile != NULL)
	{
		fclose(tracefile);
	}
}

void trace::set_trace(char *filename)
{
	if (tracefile != NULL)
	{
		fclose(tracefile);
	}
	tracename = filename;
	tracefile = fopen(tracename, "r");
	if (tracefile == NULL)
	{
		error("trace:: set_trace: error opening trace-file\n");
	}
}

void trace::traverse(DFA *dfa, FILE *stream)
{
	if (tracefile == NULL)
	{
		error("trace file is NULL!");
	}
	rewind(tracefile);

	if (VERBOSE)
	{
		fprintf(stderr, "\n=>trace::traverse DFA on file %s\n...", tracename);
	}

	if (dfa->get_depth() == NULL)
	{
		dfa->set_depth();
	}
	unsigned int *dfa_depth = dfa->get_depth();

	state_t state = 0;
	unsigned int c = fgetc(tracefile);
	long inputs = 0;

	unsigned int *stats = allocate_uint_array(dfa->size());
	for (int j=1; j<dfa->size(); j++)
	{
		stats[j] = 0;
	}
	stats[0] = 1;
	link_set *accepted_rules = new link_set();

	while (c != EOF)
	{
		state = dfa->get_next_state(state, (unsigned char)c);
		stats[state]++;
		if (!dfa->accepts(state)->empty())
		{
			accepted_rules->add(dfa->accepts(state));
			if (DEBUG)
			{
				char *label = NULL;
				link_set *acc = dfa->accepts(state);
				while (acc!=NULL && !acc->empty())
				{
					if (label == NULL)
					{
						label = (char *)malloc(100);
						sprintf(label, "%d", acc->value());
					}
					else
					{
						char *tmp = (char *)malloc(5);
						sprintf(tmp, ",%d", acc->value());
						label = strcat(label, tmp);
						free(tmp);
					}
					acc = acc->succ();
				}
				fprintf(stream, "\nrules: %s reached at character %ld \n", label, inputs);
				free(label);
			}
		}
		inputs++;
		c = fgetc(tracefile);
	}
	fprintf(stream, "\ntraversal statistics:: [state #, depth, # traversals, %%time]\n");
	int num = 0;
	for (int j=0; j<dfa->size(); j++)
	{
		if (stats[j] != 0)
		{
			fprintf(stream, "[%ld, %ld, %ld, %f %%]\n", j, dfa_depth[j], stats[j], (float)stats[j]*100/inputs);
			num++;
		}
	}
	fprintf(stream, "%ld out of %ld states traversed (%f %%)\n", num, dfa->size(), (float)num*100/dfa->size());
	fprintf(stream, "rules matched: %ld\n", accepted_rules->size());
	free(stats);
	delete accepted_rules;
}

void trace::traverse(NFA *nfa, FILE *stream)
{
	if (tracefile == NULL)
	{
		error("trace file is NULL!");
	}
	rewind(tracefile);

	if (VERBOSE)
	{
		fprintf(stderr, "\n=>trace::traverse NFA on file %s\n...", tracename);
	}

	nfa->reset_state_id();			// reset state identifiers in breath-first order

	/* Statistics */
	unsigned int *active = allocate_uint_array(nfa->size());	// active[x]=y if x states are active for y times
	unsigned int *stats = allocate_uint_array(nfa->size());		// stats[x]=y if state x was active y times
	for (int j=0; j<nfa->size(); j++)
	{
		stats[j] = 0;
		active[j] = 0;
	}

	/* Code */
	nfa_set *nfa_state = new nfa_set();
	nfa_set *next_state = new nfa_set();
	link_set *accepted_rules = new link_set();

	nfa_set *closure = nfa->epsilon_closure();

	nfa_state->insert(closure->begin(), closure->end());
	delete closure;

	FOREACH_SET(nfa_state, it)
	{
		stats[(*it)->get_id()] = 1;
	}
	active[nfa_state->size()] = 1;

	int inputs = 0;
	unsigned int c = fgetc(tracefile);
	while (c != EOF)
	{
		FOREACH_SET(nfa_state, it)
		{
			nfa_set *target = (*it)->get_transitions(c);
			if (target != NULL)
			{
				FOREACH_SET(target, it2)
				{
					nfa_set *target_closure = (*it2)->epsilon_closure();
					next_state->insert(target_closure->begin(), target_closure->end());
					delete target_closure;
				}
				delete target;
			}
		}
		delete nfa_state;
		nfa_state = next_state;
		next_state = new nfa_set();
		active[nfa_state->size()]++;
		FOREACH_SET(nfa_state, it)
		{
			stats[(*it)->get_id()]++;
		}
		link_set *rules = new link_set();

		FOREACH_SET(nfa_state, it)
		{
			rules->add((*it)->get_accepting());
		}
		
		if (!rules->empty())
		{
			accepted_rules->add(rules);
			if (DEBUG)
			{
				char *label = (char *)malloc(100);
				sprintf(label, "%d", rules->value());
				link_set *acc = rules->succ();
				while (acc!=NULL && !acc->empty())
				{
					char *tmp = (char *)malloc(5);
					sprintf(tmp, ",%d", acc->value());
					label = strcat(label, tmp);
					free(tmp);
					acc = acc->succ();
				}
				fprintf(stream, "\nrules: %s reached at character %ld \n", label, inputs);
				free(label);
			}
		}
		delete rules;
		inputs++;
		c = fgetc(tracefile);
	} // endwhile traversal

	fprintf(stream, "\ntraversal statistics:: [state #,#traversal, %%time]\n");
	unsigned long num = 0;
	for (int j=0; j<nfa->size(); j++)
	{
		if (stats[j] != 0)
		{
			fprintf(stream, "[%ld,%ld,%f %%]\n", j, stats[j], (float)stats[j]*100/inputs);
			num++;
		}
	}
	fprintf(stream, "%ld out of %ld states traversed (%f %%)\n", num, nfa->size(), (float)num*100/nfa->size());
	fprintf(stream, "\ntraversal statistics:: [size of active state vector #,frequency, %%time]\n");
	num = 0;
	for (int j=0; j<nfa->size(); j++)
	{
		if (active[j] != 0)
		{
			fprintf(stream, "[%ld,%ld,%f %%]\n", j, active[j], (float)active[j]*100/inputs);
			num += j*active[j];
		}
	}
	fprintf(stream, "average size of active state vector %f\n", (float)num/inputs);
	fprintf(stream, "rules matched: %ld\n", accepted_rules->size());
	free(stats);
	free(active);
	delete nfa_state;
	delete next_state;
	delete accepted_rules;
}

void trace::event_generate(DFA **dfa, FILE *output, FILE *stream)
{
	if (tracefile == NULL)
	{
		error("trace file is NULL!");
	}
	rewind(tracefile);

	if (VERBOSE)
	{
		fprintf(stderr, "\n=>trace::event_generate DFA on file %s\n...", tracename);
	}

	int xi, yj;
	FILE *fp = fopen("./result/TSI.data", "r");
	fscanf(fp, "%d\t%d\n", &xi, &yj);
	for (int i=0; i<xi; i++)
	{
		index_close[i] = false;		// Matching is open
		for (int j=0; j<yj; j++)
		{
			fscanf(fp, "%d", &_TSIS_[i][j]);
		}
	}
	fclose(fp);

	for (int i=0; i<MAX_DFAS_NUM; i++)
	{
		_DEAD_[i] = dfa[i]->get_dead_state();
	}

	_DFAS_ = dfa;

	fseek(tracefile, 0, SEEK_END);
	fsize = ftell(tracefile);
	rewind(tracefile);
	buffer = (unsigned char *)malloc(sizeof(unsigned char)*fsize);
	if (buffer == NULL)
	{
		printf("File load error!\n");
		return;
	}
	unsigned long result = fread(buffer, 1, fsize, tracefile);
	if (result != fsize)
	{
		printf("File load error!\n");
		return;
	}

	pthread_t tid[MAX_THREAD_NUM];
	pthread_spin_init(&key, 0);
	struct timeval start, end;

	printf("\n================================================\n");
	gettimeofday(&start, NULL);
	for (int i=0; i<MAX_DFAS_NUM; i++)
	{
		run_state_unit_t *unit_first = new run_state_unit_t();
		unit_first->cptr = 0;
		unit_first->ndfa = i;
		unit_first->anterule = new link_set(0);
		_WORK_QUEUE_->push(unit_first);
	}
	for (int i=0; i<MAX_THREAD_NUM; i++)
	{
		return_key[i] = PTHREAD_MUTEX_INITIALIZER;
		if (pthread_mutex_lock(&return_key[i]) != 0)
		{
			printf("Lock key error!\n");
			return;
		}
		if (pthread_create(&tid[i], NULL, pXthread, NULL) != 0)
		{
			printf("Create %uth thread error!\n", i+1);
			return;
		}
	}
	for (int i=0; i<MAX_THREAD_NUM; i++)
	{
		if (pthread_mutex_unlock(&return_key[i]) != 0)
		{
			printf("Unlock key error!\n");
			return;
		}
	}
	for (int i=0; i<MAX_THREAD_NUM; i++)
	{
		pthread_join(tid[i], NULL);
	}
	gettimeofday(&end, NULL);
	delete _WORK_QUEUE_;
	fprintf(stream, ">> processing time: %ldms\n>> # of matched rules: %d\n", (end.tv_sec-start.tv_sec)*1000+(end.tv_usec-start.tv_usec)/1000, global_accepted_rules->size());
}
