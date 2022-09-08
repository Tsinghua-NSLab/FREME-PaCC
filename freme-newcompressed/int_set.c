#include "int_set.h"

int_set::int_set(unsigned int Ni)
{
	N = Ni;
	item = new bool[N+1];
	for (unsigned int i=0; i<=N; i++)
	{
		item[i] = false;
	}
	num = 0;
	first = UNDEF;
}

int_set::~int_set()
{
	delete [] item;
}

void int_set::reset(unsigned int Ni)
{
	delete [] item;
	N = Ni;
	item = new bool[N+1];
	for (unsigned int i=0; i<=N; i++)
	{
		item[i] = false;
	}
	num = 0;
	first = UNDEF;
}
	
void int_set::clear()
{
	for (unsigned int i=0; i<=N; i++)
	{
		item[i] = false;
	}
	num = 0;
	first = UNDEF;
}

void int_set::print()
{
	fprintf(stderr,"[ ");
	for (unsigned int i=0; i<=N; i++)
	{
		if (item[i])
		{
			fprintf(stderr,"%d ",i);
		}
	}
	fprintf(stderr,"]\n");
}

void int_set::operator=(int_set &L)
{
	if (N != L.N)
	{
		N = L.N;
		delete [] item;
		item = new bool[L.N+1];
	}
	num = L.num;
	first = L.first;
	for (unsigned int i=0; i<=N; i++)
	{
		item[i] = L.item[i];
	}
}

void int_set::insert(unsigned int i)
{
	if (i < 0 || i > N)
	{
		error("int_set::insert: item out of range");
	}
	if (first == UNDEF || first > i)
	{
		first = i;
	}
	if (!item[i])
	{
		num++;
	}
	item[i] = true;
}

void int_set::remove(unsigned int i)
{
	if (i < 0 || i > N)
	{
		error("int_set::remove: item out of range");
	}
	if (first == i)
	{
		first = succ(i);
	}
	if (item[i])
	{
		num--;
	}
	item[i] = false;
}

bool int_set::member(unsigned int i)
{
	if (i < 0 || i > N)
	{
		error("int_set::member: item out of range");
	}
	return item[i];
}

unsigned int int_set::succ(unsigned int i)
{
	if (i < 0 || i > N)
	{
		error("int_set::succ: item out of range");
	}
	if (!item[i] || i == N)
	{
		return UNDEF;
	}
	for (unsigned int j=i+1; j<=N; j++)
	{
		if (item[j])
		{
			return j;
		}
	}
	return UNDEF;
}

void int_set::add(int_set *L)
{
	if (L->N > N)
	{
		N = L->N;
	}
	unsigned int item = L->first;
	while (item != UNDEF)
	{
		insert(item);
		item = L->succ(item);
	}
}

void int_set::negate()
{
	first = UNDEF;
	num = 0;
	for (unsigned int i=0; i<N; i++)
	{
		item[i] = !item[i];
		if (item[i])
		{
			if (first == UNDEF)
			{
				first = i;
			}
			num++;
		}
	}
}
