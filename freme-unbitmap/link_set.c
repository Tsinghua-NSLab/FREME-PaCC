#include "link_set.h"

link_set::link_set(unsigned int item)
{
	data = item;
	next = NULL;
}

link_set::~link_set()
{
	if (next != NULL)
	{
		delete next;
	}
}

bool link_set::empty()
{
	return (data == UNDEF);
}

bool link_set::equal(link_set *s)
{
	if (s == NULL && !empty())
	{
		return false;
	}
	if (data != s->data)
	{
		return false;
	}
	if (next == NULL && s->next == NULL)
	{
		return true;
	}
	if (next == NULL || s->next == NULL)
	{
		return false;
	}
	return next->equal(s->next);
}

void link_set::insert(unsigned int item)
{
	if (data == UNDEF)
	{
		data = item;
	}
	else if (item < data)
	{				// can occurr just when this is the first
		link_set *ns = new link_set(data);
		data = item;
		ns->next = next;
		next = ns;
	}
	else if (item == data)
	{
		return;
	}
	else
	{
		if (next == NULL)
		{
			next = new link_set(item);
		}
		else if (item < next->data)
		{
			link_set *ns = new link_set(item);
			ns->next = next;
			next = ns;
		}
		else
		{
			next->insert(item);
		}
	}
}

void link_set::add(link_set *s)
{
	link_set *ls = s;
	while (ls != NULL && ls->data != UNDEF)
	{
		insert(ls->data);
		ls = ls->next;
	}
}

void link_set::clear()
{
	if (next != NULL)
	{
		delete next;
	}
	next = NULL;
	data = UNDEF;
}

bool link_set::member(unsigned int item)
{
	if (data == item)
	{
		return true;
	}
	if (next == NULL)
	{
		return false;
	}
	return next->member(item);		
}

unsigned int link_set::size()
{
	if (data == UNDEF)
	{
		return 0;
	}
	if (next == NULL)
	{
		return 1;
	}
	return (1 + next->size());		
}
