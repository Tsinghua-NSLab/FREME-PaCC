#include "spliter.h"

spliter::spliter()
{
	front_end = new pair_list();
	back_end = new pair_list();
	removes = new attr_list();
}

spliter::~spliter()
{
	for (pair_list::iterator it=front_end->begin(); it!=front_end->end(); ++it)
	{
		delete (*it);
	}
	for (pair_list::iterator it=back_end->begin(); it!=back_end->end(); ++it)
	{
		delete (*it);
	}
	for (attr_list::iterator it=removes->begin(); it!=removes->end(); ++it)
	{
		delete (*it);
	}
	delete front_end;
	delete back_end;
	delete removes;
}

void spliter::split(FILE *file, int from, int to)
{
	rewind(file);
	char *re = allocate_char_array(REGEX_BUFFER);
	int i = 0, j = 0, k = 0;
	unsigned int c;					// e.g., if get the character a, c=97, (char)c='a'
	
	/* Parsing the regex rules in file and putting them in a NFA */
	do
	{
		c = fgetc(file);
		if (c=='\n' || c=='\r' || c==EOF)	// for the regex rule of each line
		{
			if (i != 0)			// for the regex (line) that is not blank
			{
				re[i] = EOS;
				if (re[0] != '#')	// the regex is not annotated
				{			// the processing of rule description can be done here
					j++;
					if (j>=from && (to==-1 || j<=to))
					{
						if (DEBUG)
						{
							fprintf(stderr, "\n@%-4d  %s\n", j, re);
						}
						split_re(re, j, k);	// regex -> regular regex
					} // endif j
				} // endif re
				i = 0;
				free(re);
				re = allocate_char_array(REGEX_BUFFER);
			} // endif i
		} // endif c
		else
		{
			re[i++] = c;
		}
	} while (c != EOF);

	if (re != NULL)
	{					// free the regex anyway
		free(re);
	}

	to_file();
}

void spliter::split_re(const char *re, int rule, int &sub_rule)
{
	int ptr = 0, times = 0;

	str_list *fa = split_re(re, &ptr, false, &times, false);

	for (str_list::iterator it=fa->begin(); it!=fa->end(); ++it)
	{
		mark_point(&(*it));
		//cout << endl << "=>     " << (*it) << endl << endl;
		const char *chars = (*it).c_str();
		//parse_re(chars, rule, ++sub_rule);
		parse_re(re, rule, ++sub_rule);
	}

	delete fa;
}

str_list *spliter::split_re(const char *re, int *ptr, bool b_status, int *b_times, bool encap_mask)
{
	str_list *fa = new str_list();
	str_list *to_link = NULL;
	string res(re);
	bool b_return = false;

	/* Processing the given regex character by character via the ptr */
	while ((*ptr) < strlen(re))
	{
		int tmp;
		string s;
		if (!be_meta(re[(*ptr)]))
		{				// when meet the common character
			s.assign(res, (*ptr), 1);
			(*ptr)++;
			if ((*ptr) == strlen(re) || !be_repetition(re[(*ptr)]))
			{
				make_link(&fa, &s);
			}
			else
			{
				to_link = new str_list();
				make_link(&to_link, &s);
				encap_mask = false;
			}
		}
		else if (re[(*ptr)] == _ESCAPE_)
		{				// when meet the meta character '\'
			if ((*ptr) == (strlen(re)-1))
			{
				error("spliter:: split_re: '\' in last position.");
			}
			else
			{
				tmp = (*ptr);
				(*ptr) = process_escape(re, (*ptr)+1);
				s.assign(res, tmp, (*ptr)-tmp);
				if ((*ptr) == strlen(re) || !be_repetition(re[(*ptr)]))
				{			// for character concatenation
					make_link(&fa, &s);
				}
				else
				{			// for prior sub-regex repetition
					to_link = new str_list();
					make_link(&to_link, &s);
					encap_mask = false;
				}
			}
		}
		else if (re[(*ptr)] == _DOT_)
		{				// when meet the meta character '.'
			s.assign(res, (*ptr), 1);
			(*ptr)++;
			if ((*ptr) == strlen(re) || !be_repetition(re[(*ptr)]))
			{
				make_link(&fa, &s);
			}
			else
			{
				to_link = new str_list();
				make_link(&to_link, &s);
				encap_mask = true;
			}
		}
		else if (re[(*ptr)] == _QUES_)
		{				// when meet the meta character '?'
			s.assign("?");
			(*ptr)++;
			if (b_return)
			{
				make_link(&fa, &s);
				return fa;
			}
			else
			{
				make_link(&to_link, &s);
				make_and(&fa, to_link);
			}
		}
		else if (re[(*ptr)] == _STAR_)
		{				// when meet the meta character '*'
			s.assign("*");
			(*ptr)++;
			if (b_return)
			{
				make_link(&fa, &s);
				return fa;
			}
			else
			{
				make_link(&to_link, &s);
				if (encap_mask)
				{
					encap(&to_link);
				}
				make_and(&fa, to_link);
			}
		}
		else if (re[(*ptr)] == _PLUS_)
		{				// when meet the meta character '+'
			s.assign("+");
			(*ptr)++;
			if (b_return)
			{
				make_link(&fa, &s);
				return fa;
			}
			else
			{
				make_link(&to_link, &s);
				if (encap_mask)
				{
					encap(&to_link);
				}
				make_and(&fa, to_link);
			}
		}
		else if (re[(*ptr)] == _OPEN_LBRACKET_)
		{				// when meet the meta character '{'
			if ((*ptr) == (strlen(re)-1))
			{
				error("spliter:: split_re: '{' in last position.");
			}
			else
			{
				tmp = (*ptr);
				int lb = 0, ub = _INFINITY;
				(*ptr) = process_quantifier(re, (*ptr)+1, &lb, &ub);
				s.assign(res, tmp, (*ptr)-tmp);
				if (b_return)
				{
					make_link(&fa, &s);
					return fa;
				}
				else
				{
					make_link(&to_link, &s);
					if (encap_mask)
					{
						encap(&to_link);
					}
					make_and(&fa, to_link);
				}
			}
		}
		else if (re[(*ptr)] == _OPEN_MBRACKET_)
		{				// when meet the meta character '['
			if ((*ptr) == (strlen(re)-1))
			{
				error("spliter:: split_re: '[' in last position.");
			}
			else
			{
				tmp = (*ptr);
				bool sb = false;
				(*ptr) = process_range(re, (*ptr)+1, &sb);
				s.assign(res, tmp, (*ptr)-tmp);
				if ((*ptr) == strlen(re) || !be_repetition(re[(*ptr)]))
				{
					make_link(&fa, &s);
				}
				else
				{
					to_link = new str_list();
					make_link(&to_link, &s);
					encap_mask = sb;
				}
			}
		}
		else if (re[(*ptr)] == _OR_)
		{				// when meet the meta character '|'
			if ((*ptr) == (strlen(re)-1))
			{
				error("spliter:: split_re: '|' in last position.");
			}

			s.assign(res, (*ptr), 1);
			(*ptr)++;
			if (b_status && !be_repetition(re[(*ptr)]))
			{
				make_link(&fa, &s);
			}
			else if (!b_status)
			{
				make_or(&fa, split_re(re, ptr, b_status, b_times, encap_mask));
			}
			else
			{
				error("spliter:: split_re: '|' has repetition.");
			}
		}
		else if (re[(*ptr)] == _OPEN_SBRACKET_)
		{				// when meet the meta character '('
			if ((*ptr) == (strlen(re)-1))
			{
				error("spliter:: split_re: '(' in last position.");
			}
			else
			{
				s.assign(res, (*ptr), 1);
				(*ptr)++; (*b_times)++;
				to_link = new str_list();
				make_link(&to_link, &s);
				make_and(&to_link, split_re(re, ptr, true, b_times, encap_mask));
				if ((*ptr) == strlen(re) || !be_repetition(re[(*ptr)]))
				{
					make_and(&fa, to_link);
				}
			}
		}
		else if (re[(*ptr)] == _CLOSE_SBRACKET_)
		{				// when meet the meta character ')'
			if (b_status)
			{
				b_return = true;
				s.assign(res, (*ptr), 1);
				(*ptr)++; (*b_times)--;
				make_link(&fa, &s);
				if ((*ptr) == strlen(re) || !be_repetition(re[(*ptr)]))
				{
					return fa;
				}
				// else continue to deal with the repetition of the sub-regex embraced by "()"
			}
			else
			{
				error("spliter:: split_re: close ')' without opening '('.");
			}
		}
		s.clear();
	} // endwhile ptr

	if ((*b_times) != 0)
	{
		error("spliter:: split_re: open '(' without closing ')'.");
	}

	return fa;
}

int spliter::process_escape(const char *re, int ptr)
{
	if (ptr == strlen(re))
	{					// basic regex syntax checking
		error("spliter:: process_escape: uncomplete escape sequence.");
	}

	char c = re[ptr];			// the character behind the meta character '\'

	if (be_x(c))
	{					// hex escape sequence
		if (ptr > (strlen(re)-3))
		{				// handle the illegal case \x and \xN
			error("spliter::process_escape: invalid hex escape sequence.");
		}
		else if (!be_hex_digit(re[ptr+1]) || !be_hex_digit(re[ptr+2]))
		{				// handle the illegal case \xNN with N is not hex
			error("spliter::process_escape: invalid hex escape sequence.");
		}
		else
		{				// handle the legal case \xNN standing for a ASCII character
			ptr = ptr + 3;
		}
	}
	else if (be_oct_digit(c))
	{					// octal escape sequence
		if (ptr > (strlen(re)-3))
		{				// handle the common case \N
			ptr++;
		}
		else if (!be_oct_digit(re[ptr+1]) || !be_oct_digit(re[ptr+2]))
		{				// handle the common case \N
			ptr++;
		}
		else if (c > '3')
		{				// handle the common case \N
			ptr++;
		}
		else
		{				//handle the legal case \NNN standing for a ASCII character
			ptr = ptr + 3;
		}
	}
	else
	{					// default escaped processing
		ptr++;
	}
	return ptr;
}

int spliter::process_range(const char *re, int ptr, bool *sb_p)
{
	bool sb = false;
	if (ptr == strlen(re))
	{					// basic regex syntax checking
		error("spliter:: process_range: range expression not closed.");
	}
	if (re[ptr] == _CLOSE_MBRACKET_)
	{					// basic regex syntax checking
		error("spliter:: process_range: invalid range expression.");
	}

	if (re[ptr] == _CARET_)
	{					// handle the case [^...]
		ptr++;
		sb = true;
	}

	while (ptr < (strlen(re)-1) && re[ptr] != _CLOSE_MBRACKET_)
	{
		if (re[ptr] == _ESCAPE_)
		{				// get the value of escaped character
			ptr = process_escape(re, ptr+1);
		}
		else
		{				// how about [^^...] and [^-...]
			ptr++;
		}
		if (ptr < (strlen(re)-1) && re[ptr] == _DASH_ && re[ptr+1] != _CLOSE_MBRACKET_)
		{				// avoid the situation [!-] and [!--]
			ptr++;
		}
	}

	if (re[ptr] != _CLOSE_MBRACKET_)
	{					// basic regex syntax checking
		error("spliter:: process_range: range expression not closed.");
	}
	else
	{
		ptr++;
	}
	(*sb_p) = sb;
	return ptr;
}

int spliter::process_quantifier(const char *re, int ptr, int *lb_p, int *ub_p)
{
	int lb = 0, ub = _INFINITY;		// default {0,}
	int res = sscanf(re+ptr, "%d", &lb);	// Actually read the number from the string starting from regex[ptr]
	if (res != 1)
	{
		error("spliter:: process_quantifier: invalid quantified expression.");
	}
	while (ptr < strlen(re) && re[ptr] != _COMMA_ && re[ptr] != _CLOSE_LBRACKET_)
	{					// skip the number scanned before
		ptr++;
	}
	if (ptr == strlen(re) || (re[ptr] == _COMMA_ && ptr == (strlen(re)-1)))
	{					// avoid the situation {N and {N,
		error("spliter:: process_quantifier: quantified expression not closed.");
	}
	if (re[ptr] == _CLOSE_LBRACKET_)
	{					// handle the case {N}
		ub = lb;
	}
	else if (re[ptr] == _COMMA_)
	{
		ptr++;
		if (re[ptr] != _CLOSE_LBRACKET_)
		{				// handle the case {N,M}
			res = sscanf(re+ptr, "%d}", &ub);
			if (res != 1)
			{
				error("spliter:: process_quantifier: invalid quantified expression.");
			}
		}				// default the case {N,}
	}
	else
	{
		error("spliter:: process_quantifier: invalid quantified expression.");
	}
	while (re[ptr] != _CLOSE_LBRACKET_)
	{					// skip the number scanned before
		ptr++;
		if (ptr == strlen(re))
		{
			error("spliter:: process_quantifier: quantified expression not closed.");
		}
	}
	ptr++;
	(*lb_p) = lb;
	(*ub_p) = ub;
	return ptr;
}

void spliter::make_form(str_list **fa, string *s)
{
	if ((*fa) == NULL)
	{
		(*fa) = new str_list();
	}
	if ((*fa)->empty())
	{
		string s_copy(*s);
		(*fa)->push_back(s_copy);
	}
	else
	{
		for (str_list::iterator it1=(*fa)->begin(), it2=(*fa)->begin(); it1!=(*fa)->end();)
		{
			string s_mark(*it2);
			string s_copz(*s);
			(*it1).append(s_copz);
			(*it1).append(s_mark);
			(*it1).append("?");
			++it1;
			++it2;
		}
	}
}

void spliter::make_copy(str_list **fa)
{
	if ((*fa) == NULL)
	{
		(*fa) = new str_list();
	}
	else
	{
		for (str_list::iterator it1=(*fa)->begin(), it2=(*fa)->begin(); it1!=(*fa)->end();)
		{
			(*it1).append(*it2);
			++it1;
			++it2;
		}
	}
}

void spliter::make_link(str_list **fa, string *s)
{
	if ((*fa) == NULL)
	{
		(*fa) = new str_list();
	}
	if ((*fa)->empty())
	{
		string s_copy(*s);
		(*fa)->push_back(s_copy);
	}
	else
	{
		for (str_list::iterator it=(*fa)->begin(); it!=(*fa)->end(); ++it)
		{
			string scopz(*s);
			(*it).append(scopz);
		}
	}
}

void spliter::make_and(str_list **fa, str_list *to_link)
{
	if ((*fa) == NULL)
	{
		(*fa) = new str_list();
	}
	if (to_link == NULL)
	{
		return;
	}
	else
	{
		if ((*fa)->empty())
		{
			for (str_list::iterator it=to_link->begin(); it!=to_link->end(); ++it)
			{
				string s_copy(*it);
				(*fa)->push_back(s_copy);
			}
		}
		else
		{
			for (str_list::iterator it1=(*fa)->begin(); it1!=(*fa)->end();)
			{
				for (int i=1; i<to_link->size(); i++)
				{
					string s_copz(*it1);
					it1 = (*fa)->insert(it1, s_copz);
				}
				for (str_list::iterator it2=to_link->begin(); it2!=to_link->end(); ++it2)
				{
					string s_copx(*it2);
					(*it1).append(s_copx);
					++it1;
				}
			}
		}
	}
	delete to_link;
}

void spliter::make_or(str_list **fa, str_list *to_link)
{
	if ((*fa) == NULL)
	{
		(*fa) = new str_list();
	}
	if (to_link == NULL)
	{
		string s_copy;
		(*fa)->push_back(s_copy);
	}
	else
	{
		for (str_list::iterator it=to_link->begin(); it!=to_link->end(); ++it)
		{
			string s_copy(*it);
			(*fa)->push_back(s_copy);
		}
	}
	delete to_link;
}

void spliter::encap(str_list **fa)
{
	if ((*fa) == NULL)
	{
		return;
	}
	else
	{
		for (str_list::iterator it=(*fa)->begin(); it!=(*fa)->end(); ++it)
		{
			(*it).insert(0, "\xFA");
			(*it).insert((*it).size(), "\xFB");
		}
	}
}

void spliter::mark_point(string *res)
{
	bool b_status = false, r_status = false, lock = false;
	int b_times = 0, bkt_ptrs = 0, rep_ptrs = 0, step = 0;
	int val_ptrs = 0;
	int *ptrs = &val_ptrs;
	int val_ptre = 0;
	int *ptre = &val_ptre;

	char dfatype = '\xFF';
	const char *re0 = res->c_str();
	if (re0[(*ptre)] == _CARET_)
	{				// when meet the meta character '^'
		(*ptre)++;
		dfatype = '\xFE';
	}
	else
	{
		res->insert((*ptrs), 1, dfatype);
		(*ptre)++;
		r_status = true;
		lock = true;
	}

	/* Processing the given regex character by character via the ptr */
	while ((*ptre) < res->size())
	{
		const char *re = res->c_str();
		if (re[(*ptre)] == _DOT_)
		{				// when meet the meta character '.'
			int ptmp = (*ptre);
			(*ptre)++;
			if (b_status)
			{
				dfatype = '\xFE';
				if (re[(*ptre)] == _OPEN_LBRACKET_)
				{
					if (!r_status && ptmp == (*ptrs)+1)
					{
						dfatype = '\xFC';
					}
					else if (ptmp == (*ptrs))
					{
						dfatype = '\xFC';
					}
				}
				else
				{
					if (!r_status || !lock)
					{
						res->insert((*ptrs), 1, dfatype);
						(*ptre)++;
						ptmp = ptmp + 1;
					}
					dfatype = '\xFF';
					(*ptrs) = ptmp;
				}
			}
			else if (!b_status && b_times == 0)
			{
				step++;
				if ((*ptre) == strlen(re))
				{
					if (!lock)
					{
						res->insert((*ptrs), 1, dfatype);
						(*ptre)++;
					}
				}
				else if (be_repetition(re[(*ptre)]))
				{
					rep_ptrs = ptmp;
				}
				else if (re[(*ptre)] == '\xFA')
				{
					if (r_status)
					{
						if (step == 1)
						{
							(*ptrs) = (*ptre);
						}
						else if (step == 2)
						{
							(*ptrs) = ptmp;
						}
						else
						{
							if (dfatype == '\xFF')
							{
								(*ptrs) = ptmp;
							}
						}
					}
				}
				else
				{
					if (r_status && step == 1)
					{
						if (dfatype != '\xFF')
						{
							dfatype = '\xFE';
							(*ptrs) = (*ptre);
							lock = false;
						}
					}
				}
			}
		}
		else if (re[(*ptre)] == _ESCAPE_)
		{				// when meet the meta character '\'
			int ptmp = (*ptre);
			(*ptre) = process_escape(re, (*ptre)+1);
			if (!b_status && b_times == 0)
			{
				step++;
				if ((*ptre) == strlen(re))
				{
					if (!lock)
					{
						res->insert((*ptrs), 1, dfatype);
						(*ptre)++;
					}
				}
				else if (be_repetition(re[(*ptre)]))
				{
					rep_ptrs = ptmp;
				}
				else if (re[(*ptre)] == '\xFA')
				{
					if (r_status)
					{
						if (step == 1)
						{
							(*ptrs) = (*ptre);
						}
						else if (step == 2)
						{
							(*ptrs) = ptmp;
						}
						else
						{
							if (dfatype == '\xFF')
							{
								(*ptrs) = ptmp;
							}
						}
					}
				}
				else
				{
					if (r_status && step == 1)
					{
						if (dfatype != '\xFF')
						{
							dfatype = '\xFE';
							(*ptrs) = (*ptre);
							lock = false;
						}
					}
				}
			}
		}
		else if (re[(*ptre)] == _OPEN_MBRACKET_)
		{				// when meet the meta character '['
			int ptmp = (*ptre);
			bool k = false;
			(*ptre) = process_range(re, (*ptre)+1, &k);
			if (b_status)
			{
				dfatype = '\xFE';
				if (re[(*ptre)] == _OPEN_LBRACKET_)
				{
					if (!r_status && ptmp == (*ptrs)+1)
					{
						dfatype = '\xFC';
					}
					else if (ptmp == (*ptrs))
					{
						dfatype = '\xFC';
					}
				}
				else
				{
					if (!r_status && ptmp == (*ptrs)+1)
					{
						dfatype = '\xFD';
					}
					else if (ptmp == (*ptrs))
					{
						dfatype = '\xFD';
					}
				}
			}
			else if (!b_status && b_times == 0)
			{
				step++;
				if ((*ptre) == strlen(re))
				{
					if (!lock)
					{
						res->insert((*ptrs), 1, dfatype);
						(*ptre)++;
					}
				}
				else if (be_repetition(re[(*ptre)]))
				{
					rep_ptrs = ptmp;
				}
				else if (re[(*ptre)] == '\xFA')
				{
					if (r_status)
					{
						if (step == 1)
						{
							(*ptrs) = (*ptre);
						}
						else if (step == 2)
						{
							(*ptrs) = ptmp;
						}
						else
						{
							if (dfatype == '\xFF')
							{
								(*ptrs) = ptmp;
							}
						}
					}
				}
				else
				{
					if (r_status && step == 1)
					{
						if (dfatype != '\xFF')
						{
							dfatype = '\xFE';
							(*ptrs) = (*ptre);
							lock = false;
						}
					}
				}
			}
		}
		else if (re[(*ptre)] == '\xFA')
		{				// when meet the meta character '\xFE'
			b_status = true;
			res->erase((*ptre), 1);
		}
		else if (re[(*ptre)] == '\xFB')
		{				// when meet the meta character '\xFF'
			b_status = false;
			res->erase((*ptre), 1);
			res->insert((*ptrs), 1, dfatype);
			r_status = true;
			lock = true;
			(*ptre)++;
			(*ptrs) = (*ptre);
			step = 0;
		}
		else if ((re[(*ptre)] == _STAR_) || (re[(*ptre)] == _PLUS_) || (re[(*ptre)] == _QUES_))
		{				// when meet the meta character '*'
			(*ptre)++;
			if (!b_status && b_times == 0)
			{
				if ((*ptre) == strlen(re))
				{
					if (!lock)
					{
						res->insert((*ptrs), 1, dfatype);
						(*ptre)++;
					}
				}
				else if (re[(*ptre)] == '\xFA')
				{
					if (r_status)
					{
						if (step == 1)
						{
							(*ptrs) = (*ptre);
						}
						else if (step == 2)
						{
							(*ptrs) = rep_ptrs;
						}
						else
						{
							if (dfatype == '\xFF')
							{
								(*ptrs) = rep_ptrs;
							}
						}
					}
				}
				else
				{
					if (r_status && step == 1)
					{
						if (dfatype != '\xFF')
						{
							dfatype = '\xFE';
							(*ptrs) = (*ptre);
							lock = false;
						}
					}
				}
			}
		}
		else if (re[(*ptre)] == _OPEN_LBRACKET_)
		{				// when meet the meta character '{'
			int i=0, j=_INFINITY;
			(*ptre) = process_quantifier(re, (*ptre)+1, &i, &j);
			if (!b_status && b_times == 0)
			{
				if ((*ptre) == strlen(re))
				{
					if (!lock)
					{
						res->insert((*ptrs), 1, dfatype);
						(*ptre)++;
					}
				}
				else if (re[(*ptre)] == '\xFA')
				{
					if (r_status)
					{
						if (step == 1)
						{
							(*ptrs) = (*ptre);
						}
						else if (step == 2)
						{
							(*ptrs) = rep_ptrs;
						}
						else
						{
							if (dfatype == '\xFF')
							{
								(*ptrs) = rep_ptrs;
							}
						}
					}
				}
				else
				{
					if (r_status && step == 1)
					{
						if (dfatype != '\xFF')
						{
							dfatype = '\xFE';
							(*ptrs) = (*ptre);
							lock = false;
						}
					}
				}
			}
		}
		else if (re[(*ptre)] == _OPEN_SBRACKET_)
		{
			if (!b_status && b_times == 0)
			{
				bkt_ptrs = (*ptre);
			}
			++b_times;
			(*ptre)++;
		}
		else if (re[(*ptre)] == _CLOSE_SBRACKET_)
		{
			(*ptre)++;
			--b_times;
			if (!b_status && b_times == 0)
			{
				step++;
				if ((*ptre) == strlen(re))
				{
					if (!lock)
					{
						res->insert((*ptrs), 1, dfatype);
						(*ptre)++;
					}
				}
				else if (be_repetition(re[(*ptre)]))
				{
					rep_ptrs = bkt_ptrs;
				}
				else if (re[(*ptre)] == '\xFA')
				{
					if (r_status)
					{
						if (step == 1)
						{
							(*ptrs) = (*ptre);
						}
						else if (step == 2)
						{
							(*ptrs) = bkt_ptrs;
						}
						else
						{
							if (dfatype == '\xFF')
							{
								(*ptrs) = bkt_ptrs;
							}
						}
					}
				}
				else
				{
					if (r_status && step == 1)
					{
						if (dfatype != '\xFF')
						{
							dfatype = '\xFE';
							(*ptrs) = (*ptre);
							lock = false;
						}
					}
				}
			}
		}
		else
		{				// when meet the common character
			int ptmp = (*ptre);
			(*ptre)++;
			if (!b_status && b_times == 0)
			{
				step++;
				if ((*ptre) == strlen(re))
				{
					if (!lock)
					{
						res->insert((*ptrs), 1, dfatype);
						(*ptre)++;
					}
				}
				else if (be_repetition(re[(*ptre)]))
				{
					rep_ptrs = ptmp;
				}
				else if (re[(*ptre)] == '\xFA')
				{
					if (r_status)
					{
						if (step == 1)
						{
							(*ptrs) = (*ptre);
						}
						else if (step == 2)
						{
							(*ptrs) = ptmp;
						}
						else
						{
							if (dfatype == '\xFF')
							{
								(*ptrs) = ptmp;
							}
						}
					}
				}
				else
				{
					if (r_status && step == 1)
					{
						if (dfatype != '\xFF')
						{
							dfatype = '\xFE';
							(*ptrs) = (*ptre);
							lock = false;
						}
					}
				}
			}
		}
	} // endwhile ptr
}

void spliter::parse_re(const char *re, int rule, int sub_rule)
{
	int dfatype = 0, stage = 0;
	string res(re);
	int val_ptrs = 0;
	int *ptrs = &val_ptrs;
	int val_ptre = 0;
	int *ptre = &val_ptre;

	/* Processing the given regex character by character via the ptr */
	while ((*ptre) < strlen(re))
	{
		if (re[(*ptre)] == '@')
		{
			if ((*ptre) > 0)
			{
				string s(res, (*ptrs), (*ptre)-(*ptrs));
				if ((*ptrs) > 1)
				{
					s.insert(0, "^");
				}
				inserts(&s, dfatype, 0, rule, sub_rule, stage++, false);
			}
			dfatype = 0;
			(*ptrs) = (*ptre) + 1;
		}
		else if (re[(*ptre)] == '#')
		{
			if ((*ptre) > 0)
			{
				string s(res, (*ptrs), (*ptre)-(*ptrs));
				if ((*ptrs) > 1)
				{
					s.insert(0, "^");
				}
				inserts(&s, dfatype, 1, rule, sub_rule, stage++, false);
			}
			dfatype = 1;
			(*ptrs) = (*ptre) + 1;
		}
		else if (re[(*ptre)] == '\xFD')
		{
			if ((*ptre) > 0)
			{
				string s(res, (*ptrs), (*ptre)-(*ptrs));
				if ((*ptrs) > 1)
				{
					s.insert(0, "^");
				}
				inserts(&s, dfatype, 2, rule, sub_rule, stage++, false);
			}
			dfatype = 2;
			(*ptrs) = (*ptre) + 1;
		}
		else if (re[(*ptre)] == '\xFC')
		{
			if ((*ptre) > 0)
			{
				string s(res, (*ptrs), (*ptre)-(*ptrs));
				if ((*ptrs) > 1)
				{
					s.insert(0, "^");
				}
				inserts(&s, dfatype, 3, rule, sub_rule, stage++, false);
			}
			dfatype = 3;
			(*ptrs) = (*ptre) + 1;
		}
		(*ptre)++;
		if ((*ptre) == strlen(re))
		{
			string s(res, (*ptrs), (*ptre)-(*ptrs));
			if ((*ptrs) > 1)
			{
				s.insert(0, "^");
			}
			inserts(&s, dfatype, 0, rule, sub_rule, stage++, true);
		}
	} // endwhile ptr
}

void spliter::inserts(string *s, int dfatype, int next_dfatype, int rule, int sub_rule, int stage, bool ending)
{
	int event = 0;
	char chars[16];

	string s_temp1(*s, 0, 3);
	string s_temp2("^.*");
	string s_copy;
	if (s_temp1.compare(s_temp2) != 0)
	{
		s_copy.assign(*s);
	}
	else
	{
		s_copy.assign(*s, 3, s->size()-3);
	}

	if (front_end->empty())
	{
		pair<int, string> *sub_front = new pair<int, string>(dfatype, s_copy);
		front_end->push_back(sub_front);
		++event;
		entry_attr_t *sub_removes = new entry_attr_t();
		sub_removes->stage = stage;
		sub_removes->ending = ending;
		sub_removes->next_dfatype = next_dfatype;
		removes->push_back(sub_removes);
	}
	else
	{
		int find_entry = 0;
		pair_list::iterator it;
		attr_list::iterator is;
		for (it=front_end->begin(),is=removes->begin(); it!=front_end->end(); ++it,++is)
		{
			//ending = true;
			if (ending)
			{
				event = front_end->size();
			}
			else
			{
				++event;
				// removing conditions that each sub_rule must meet
				if ((*it)->second.compare(s_copy)==0 && (*it)->first==dfatype && (*is)->stage==stage && !(*is)->ending && (*is)->next_dfatype==next_dfatype)
				{
					// sort the ruleset, sub_rule cannot be identical for the entry within the same rule, so ">" rather than ">="
					if (stage == 0 || event > anchor_entry)
					{
						find_entry = event;
					}
				}
			}
		}
		if (find_entry == 0)
		{
			pair<int, string> *sub_front = new pair<int, string>(dfatype, s_copy);
			front_end->push_back(sub_front);
			++event;
			entry_attr_t *sub_removes = new entry_attr_t();
			sub_removes->stage = stage;
			sub_removes->ending = ending;
			sub_removes->next_dfatype = next_dfatype;
			removes->push_back(sub_removes);
		}
		else
		{
			event = find_entry;
		}
		anchor_entry = event;
	}

	if (back_end->empty() || sub_rule > back_end->size())
	{
		sprintf(chars, "#%d", event);
		string s_copz(chars);
		pair<int, string> *sub_back = new pair<int, string>(rule, s_copz);
		back_end->push_back(sub_back);
	}
	else
	{
		sprintf(chars, "@%d#%d", dfatype+1, event);
		string s_copx(chars);
		back_end->back()->second.append(s_copx);
	}
}

void spliter::to_file()
{
	ofstream index_f(INDEX_TABLE);
	ofstream caret_f(CARET_RULE);
	ofstream any_f(ANY_RULE);
	ofstream star_f(STAR_RULE);
	ofstream time_f(TIME_RULE);
	ofstream tsi_f(TSI_DATA);

	int max_rule = 0;
	for (pair_list::iterator it=front_end->begin(); it!=front_end->end(); ++it)
	{
		switch ((*it)->first)
		{
			case 0:
				any_f << "(" << (++max_rule) << ")" << (*it)->second << endl;
				break;
			case 1:
				caret_f << "(" << (++max_rule) << ")" << (*it)->second << endl;
				break;
			case 2:
				star_f << "(" << (++max_rule) << ")" << (*it)->second << endl;
				break;
			case 3:
				time_f << "(" << (++max_rule) << ")" << (*it)->second << endl;
				break;
		}
	}

	int max_stage = 0;
	for (pair_list::iterator it=back_end->begin(); it!=back_end->end(); ++it)
	{
		int stage = 0;
		for (string::iterator iter = (*it)->second.begin(); iter != (*it)->second.end(); iter++)
		{
			if ((*iter) == '#')
			{
				stage++;
			}
		}
		(*it)->second.append("@0$");
		if (max_stage < stage)
		{
			max_stage = stage;
		}
	}
	index_f << max_rule << "\t" << max_stage+2 << endl;
	tsi_f << max_rule << "\t" << 3 << endl;
	printf("===========================================\n");
	printf(">> # of sub-rules: %d\n>> # of stages: %d\n", max_rule, max_stage);
	printf("===========================================\n");

	int row = 1, cur_row;
	string cur_rule, pre_rule;
	for (pair_list::iterator it=back_end->begin(); it!=back_end->end(); ++it)
	{
		int array = 0, i;
		cur_rule.assign("0");
		string next_dfatype;
		for (string::iterator iter = (*it)->second.begin(); iter != (*it)->second.end();)
		{
			if ((*iter) == '#' || (*iter) == '$')
			{
				cur_row = atoi(cur_rule.c_str());
				if (cur_row == 0)
				{
					pre_rule.assign(cur_rule);
					cur_rule.clear();
				}
				else if (cur_row < row)
				{
					pre_rule.assign(cur_rule);
					cur_rule.clear();
					array++;
				}
				else if (cur_row == row)
				{
					tsi_f << pre_rule << "\t" << (next_dfatype.compare("4")==0 ? "3":next_dfatype);
					index_f << next_dfatype << "\t\t" << pre_rule;
					pre_rule.assign(cur_rule);
					cur_rule.clear();
					for (i=0; i<array; i++)
					{
						index_f << "\t\t0";
					}
					array++;
					if ((*iter) == '$')
					{
						tsi_f << "\t" << (*it)->first;
						index_f << "\t\t" << (*it)->first;
					}
					else
					{
						tsi_f << "\t0";
						index_f << "\t\t-1";
					}
					for (i=array; i<max_stage; i++)
					{
						index_f << "\t\t0";
					}
					tsi_f << endl;
					index_f << endl;
					row++;
				}
				iter++;
			}
			else
			{
				if ((*iter) == '@')
				{
					next_dfatype.assign(1, *(++iter));
				}
				else
				{
					cur_rule.append(1, (*iter));
				}
				iter++;
			}
		}
	}

	index_f.close();
	caret_f.close();
	any_f.close();
	star_f.close();
	time_f.close();
	tsi_f.close();
}
