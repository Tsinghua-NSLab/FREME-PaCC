#include "parser.h"

parser::parser(bool m_mod, bool s_mod, bool i_mod)
{
	m_modifier = m_mod;				// '^' and '$'
	s_modifier = s_mod;				// '.'
	i_modifier = i_mod;				// case

	NFA::s_mode = s_mod;
	NFA::i_mode = i_mod;
}

parser::~parser()
{

}

NFA *parser::parse(FILE *file, int from, int to)
{
	rewind(file);
	char *re = allocate_char_array(REGEX_BUFFER);
	int i = 0, j = 0;
	unsigned int c;					// e.g., if get the character a, c=97, (char)c='a'
	
	NFA *nfa = new NFA();				// NFA initialization
	NFA *non_anchored = nfa->add_epsilon();		// for .* regex
	NFA *caret_anchored = nfa->add_epsilon();	// for ^ anchored regex
	
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
						parse_re(nfa, re);	// regex -> nfa
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
	{						// free the regex anyway
		free(re);
	}
	
	if (m_modifier && (!caret_anchored->get_epsilon()->empty() || !caret_anchored->get_transitions()->empty()))
	{						// handle (?m)
		non_anchored->add_transition('\n', caret_anchored);
		non_anchored->add_transition('\r', caret_anchored);
	}

	if (non_anchored->get_epsilon()->empty() && non_anchored->get_transitions()->empty())
	{						// delete non_anchored, if possible
		nfa->get_epsilon()->remove(non_anchored);
		delete non_anchored;
	}
	else
	{
		non_anchored->add_any(non_anchored);
	}
	
	return nfa->get_first();
}

void *parser::parse_re(NFA* nfa, const char *re)
{
	int ptr = 0, sub_rule;
	bool caret_re = false, negate_re = false;
	NFA *non_anchored = *(nfa->get_epsilon()->begin());
	NFA *caret_anchored = *(++nfa->get_epsilon()->begin());

	sscanf(re, "(%d)", &sub_rule);
	while (re[ptr] != ')')
	{
		ptr++;
	}
	ptr++;
	/* Check whether the text must match at the beginning of the regular expression */
	if (re[ptr] == _CARET_)
	{
		caret_re = true;
		ptr++;
	}
	NFA *fa = parse_re(re, &ptr, false, negate_re);	// start to parse
	fa->get_last()->accept(sub_rule);		// mark the accept state for each regex rule
	if (!caret_re)
	{
		non_anchored->add_epsilon(fa->get_first());
	}
	else
	{
		caret_anchored->add_epsilon(fa->get_first());
	}
}

NFA *parser::parse_re(const char *re, int *ptr, bool bracket, bool negate_re)
{
	NFA *fa = new NFA();
	NFA *to_link = NULL;
	bool open_b = bracket, close_b = false;

	/* Processing the given regex character by character via the ptr */
	while ((*ptr) < strlen(re))
	{
		if (re[(*ptr)] == _ESCAPE_)
		{				// when meet the meta character '\'
			int_set *chars = new int_set(CSIZE);
			(*ptr) = process_escape(re, (*ptr)+1, chars);
			if (negate_re)
			{
				chars->negate();
			}
			if ((*ptr) == strlen(re) || !is_repetition(re[(*ptr)]))
			{			// for character concatenation
				fa = fa->add_transition(chars);
			}
			else
			{			// for prior sub-regex repetition
				to_link = new NFA();
				to_link = to_link->add_transition(chars);
			}
			delete chars;
		}
		else if (!is_meta(re[(*ptr)]))
		{				// when meet the common character
			char c = re[(*ptr)];
			int_set *chars = new int_set(CSIZE);
			chars->insert(c);
			if (negate_re)
			{
				chars->negate();
			}
			if ((*ptr) == (strlen(re)-1) || !is_repetition(re[(*ptr)+1]))
			{
				fa = fa->add_transition(chars);
			}
			else
			{
				to_link = new NFA();
				to_link = to_link->add_transition(chars);
			}
			(*ptr)++;
			delete chars;
		}
		else if (re[(*ptr)] == _DOT_)
		{				// when meet the meta character '.'
			int_set *chars = new int_set(CSIZE);
			if (s_modifier)
			{
				chars->insert('\n');
			}
			if ((*ptr) == (strlen(re)-1) || !is_repetition(re[(*ptr)+1]))
			{
				if (negate_re)
				{
					fa = fa->add_transition(chars);
				}
				else
				{
					fa = fa->add_any();
				}
			}
			else
			{
				to_link = new NFA();
				if (negate_re)
				{
					to_link = to_link->add_transition(chars);
				}
				else
				{
					to_link = to_link->add_any();
				}
			}
			(*ptr)++;
			delete chars;
		}
		else if (re[(*ptr)] == _STAR_)
		{				// when meet the meta character '*'
			(*ptr)++;
			if (close_b)
			{
				return fa->make_rep(0, _INFINITY);
			}
			else
			{
				to_link = to_link->make_rep(0, _INFINITY);
				fa = fa->link(to_link);
			}
		}
		else if (re[(*ptr)] == _QUES_)
		{				// when meet the meta character '?'
			(*ptr)++;
			if (close_b)
			{
				return fa->make_rep(0, 1);
			}
			else
			{
				to_link = to_link->make_rep(0, 1);
				fa = fa->link(to_link);
			}
		}
		else if (re[(*ptr)] == _PLUS_)
		{				// when meet the meta character '+'
			(*ptr)++;
			if (close_b)
			{
				return fa->make_rep(1, _INFINITY);
			}
			else
			{
				to_link = to_link->make_rep(1, _INFINITY);
				fa = fa->link(to_link);
			}
		}
		else if (re[(*ptr)] == _OPEN_LBRACKET_)
		{				// when meet the meta character '{'
			if ((*ptr) == (strlen(re)-1))
			{
				error("parser:: parse_re: { in last position.");
			}
			else
			{
				int lb = 0, ub = _INFINITY;
				(*ptr) = process_quantifier(re, (*ptr)+1, &lb, &ub);
				if (close_b)
				{
					return fa->make_rep(lb, ub);
				}
				else
				{
					to_link = to_link->make_rep(lb, ub);
					fa = fa->link(to_link);
				}
			}
		}
		else if (re[(*ptr)] == _OPEN_MBRACKET_)
		{				// when meet the meta character '['
			if ((*ptr) == (strlen(re)-1))
			{
				error("parser:: parse_re: [ in last position.");
			}
			else
			{
				(*ptr) = process_range(re, (*ptr)+1, &fa, &to_link, negate_re);
			}
		}
		else if (re[(*ptr)] == _OR_)
		{				// when meet the meta character '|'
			(*ptr)++;
			fa = fa->make_or(parse_re(re, ptr, false, negate_re));
			//fa = fa->make_or(parse_re(re, ptr, open_b, negate_re));
		}
		else if ((re[(*ptr)] == _OPEN_SBRACKET_))
		{				// when meet the meta character '('
			(*ptr)++;
			fa = fa->get_last()->link(parse_re(re, ptr, true, negate_re));
		}
		else if ((re[(*ptr)] == _CLOSE_SBRACKET_))
		{				// when meet the meta character ')'
			if (open_b)
			{
				close_b = true;
				(*ptr)++;
				if ((*ptr) == strlen(re) || !is_repetition(re[(*ptr)]))
				{
					return fa;
				}
			}
			else
			{
				return fa;
				//error("parser:: parse_re: close ')' without opening '('.");
			}
		}
		/*
		   else if (re[(*ptr)] == _CARET_)
		   {				// when meet the meta character '^'
		   if ((*ptr) == (strlen(re)-1) || !is_repetition(re[(*ptr)+1]))
		   {
		   fa = fa->add_transition(re[(*ptr)]);
		   }
		   else
		   {
		   to_link = new NFA();
		   to_link = to_link->add_transition(re[(*ptr)]);
		   }
		   (*ptr)++;
		   }
		   else if (re[(*ptr)] == _DOLLAR_)
		   {				// when meet the meta character '$'
		   if ((*ptr) == (strlen(re)-1) || !is_repetition(re[(*ptr)+1]))
		   {
		   fa = fa->add_transition(re[(*ptr)]);
		   }
		   else
		   {
		   to_link = new NFA();
		   to_link = to_link->add_transition(re[(*ptr)]);
		   }
		   (*ptr)++;
		   }
		 */
	}
	return fa->get_first();
}

int parser::process_escape(const char *re, int ptr, int_set *chars)
{
	if (ptr == strlen(re))
	{					// basic regex syntax checking
		error("parser:: process_escape: uncomplete escape sequence.");
	}

	char c = re[ptr];			// the character behind the meta character '\'
	unsigned int next;			// the value equal to the corresponding character

	if (is_x(c))
	{					// hex escape sequence
		if (ptr > (strlen(re)-3))
		{				// handle the illegal case \x and \xN
			error("parser::process_escape: invalid hex escape sequence.");
		}
		else if (!is_hex_digit(re[ptr+1]) || !is_hex_digit(re[ptr+2]))
		{				// handle the illegal case \xNN with N is not hex
			error("parser::process_escape: invalid hex escape sequence.");
		}
		else
		{				// handle the legal case \xNN standing for a ASCII character
			char tmp[5];
			tmp[0] = '0';
			tmp[1] = c;
			tmp[2] = re[ptr+1];
			tmp[3] = re[ptr+2];
			tmp[4] = '\0';
			sscanf(tmp,"0x%x", &next);
			chars->insert(next);
			ptr = ptr + 3;
		}
	}
	else if (is_oct_digit(c))
	{					// octal escape sequence
		if (ptr > (strlen(re)-3))
		{				// handle the common case \N
			next = escaped(c);
			chars->insert(next);
			ptr++;
		}
		else if (!is_oct_digit(re[ptr+1]) || !is_oct_digit(re[ptr+2]))
		{				// handle the common case \N
			next = escaped(c);
			chars->insert(next);
			ptr++;
		}
		else if (c > '3')
		{				// handle the common case \N
			next = escaped(c);
			chars->insert(next);
			ptr++;
		}
		else
		{				//handle the legal case \NNN standing for a ASCII character
			char tmp[5];
			tmp[0] = '0';
			tmp[1] = c;
			tmp[2] = re[ptr+1];
			tmp[3] = re[ptr+2];
			tmp[4] = '\0';
			sscanf(tmp, "0%o", &next);
			chars->insert(next);
			ptr = ptr + 3;
		}
	}
	else if (c == 's')
	{
		chars->insert('\t');
		chars->insert('\n');
		chars->insert('\v');
		chars->insert('\f');
		chars->insert('\r');
		chars->insert('\x20');
		ptr++;
	}
	else if (c == 'S')
	{
		chars->insert('\t');
		chars->insert('\n');
		chars->insert('\v');
		chars->insert('\f');
		chars->insert('\r');
		chars->insert('\x20');
		chars->negate();
		ptr++;
	}
	else if (c == 'd')
	{
		chars->insert('0');chars->insert('1');chars->insert('2');
		chars->insert('3');chars->insert('4');chars->insert('5');
		chars->insert('6');chars->insert('7');chars->insert('8');
		chars->insert('9');
		ptr++;
	}
	else if (c == 'D')
	{
		chars->insert('0');chars->insert('1');chars->insert('2');
		chars->insert('3');chars->insert('4');chars->insert('5');
		chars->insert('6');chars->insert('7');chars->insert('8');
		chars->insert('9');
		chars->negate();
		ptr++;
	}
	else if (c == 'w')
	{
		chars->insert('_');
		chars->insert('0');chars->insert('1');chars->insert('2');
		chars->insert('3');chars->insert('4');chars->insert('5');
		chars->insert('6');chars->insert('7');chars->insert('8');
		chars->insert('9');
		chars->insert('a');chars->insert('b');chars->insert('c');
		chars->insert('d');chars->insert('e');chars->insert('f');
		chars->insert('g');chars->insert('h');chars->insert('i');
		chars->insert('j');chars->insert('k');chars->insert('l');
		chars->insert('m');chars->insert('n');chars->insert('o');
		chars->insert('p');chars->insert('q');chars->insert('r');
		chars->insert('s');chars->insert('t');chars->insert('u');
		chars->insert('v');chars->insert('w');chars->insert('x');
		chars->insert('y');chars->insert('z');
		chars->insert('A');chars->insert('B');chars->insert('C');
		chars->insert('D');chars->insert('E');chars->insert('F');
		chars->insert('G');chars->insert('H');chars->insert('I');
		chars->insert('J');chars->insert('K');chars->insert('L');
		chars->insert('M');chars->insert('N');chars->insert('O');
		chars->insert('P');chars->insert('Q');chars->insert('R');
		chars->insert('S');chars->insert('T');chars->insert('U');
		chars->insert('V');chars->insert('W');chars->insert('X');
		chars->insert('Y');chars->insert('Z');
		ptr++;
	}
	else if (c == 'W')
	{
		chars->insert('_');
		chars->insert('0');chars->insert('1');chars->insert('2');
		chars->insert('3');chars->insert('4');chars->insert('5');
		chars->insert('6');chars->insert('7');chars->insert('8');
		chars->insert('9');
		chars->insert('a');chars->insert('b');chars->insert('c');
		chars->insert('d');chars->insert('e');chars->insert('f');
		chars->insert('g');chars->insert('h');chars->insert('i');
		chars->insert('j');chars->insert('k');chars->insert('l');
		chars->insert('m');chars->insert('n');chars->insert('o');
		chars->insert('p');chars->insert('q');chars->insert('r');
		chars->insert('s');chars->insert('t');chars->insert('u');
		chars->insert('v');chars->insert('w');chars->insert('x');
		chars->insert('y');chars->insert('z');
		chars->insert('A');chars->insert('B');chars->insert('C');
		chars->insert('D');chars->insert('E');chars->insert('F');
		chars->insert('G');chars->insert('H');chars->insert('I');
		chars->insert('J');chars->insert('K');chars->insert('L');
		chars->insert('M');chars->insert('N');chars->insert('O');
		chars->insert('P');chars->insert('Q');chars->insert('R');
		chars->insert('S');chars->insert('T');chars->insert('U');
		chars->insert('V');chars->insert('W');chars->insert('X');
		chars->insert('Y');chars->insert('Z');
		chars->negate();
		ptr++;										
	}
	else
	{					// default escaped processing
		next = escaped(c);
		chars->insert(next);
		ptr++;
	}
	return ptr;
}

int parser::process_range(const char *re, int ptr, NFA **fa, NFA **to_link, bool negate_re)
{
	if (ptr == strlen(re))
	{					// basic regex syntax checking
		error("parser:: process_range: range expression not closed.");
	}
	if (re[ptr] == _CLOSE_MBRACKET_)
	{					// basic regex syntax checking
		error("parser:: process_range: invalid range expression.");
	}

	bool negate = false;
	if (re[ptr] == _CARET_)
	{					// handle the case [^...]
		negate = true;
		ptr++;
	}

	unsigned int from = CSIZE + 1, to;
	int_set *range = new int_set(CSIZE);	// in range, only '^', '-', '\' and ']' are special character

	while (ptr < (strlen(re)-1) && re[ptr] != _CLOSE_MBRACKET_)
	{
		to = re[ptr];
		if (to == _ESCAPE_)
		{				// get the value of escaped character
			int_set *chars = new int_set(CSIZE);
			ptr = process_escape(re, ptr+1, chars);
			to = chars->head();
			delete chars;
		}
		else
		{				// how about [^^...] and [^-...]
			ptr++;
		}
		if (from == (CSIZE + 1))
		{				// from record the lower limit of the range
			from = to;		// to record the upper limit of the range
		}
		if (ptr < (strlen(re)-1) && re[ptr] == _DASH_ && re[ptr+1] != _CLOSE_MBRACKET_)
		{				// avoid the situation [!-] and [!--]
			ptr++;
		}
		else
		{
			if (from > to)
			{
				error("parser:: process_range: invalid range expression.");
			}
			for (unsigned int i=from; i<=to; i++)
			{			// handle the case [a-z]
				range->insert(i);
				if (i == 255)
				{
					break;
				}
			}
			from = CSIZE + 1;
		}
	}

	if (re[ptr] != _CLOSE_MBRACKET_)
	{					// basic regex syntax checking
		error("parser:: process_range: range expression not closed.");
	}

	if (i_modifier)
	{					// case insensitive
		int_set *is = new int_set(CSIZE);
		for (unsigned v=range->head(); v!=UNDEF; v=range->succ(v))
		{
			if (v >= 'A' && v <= 'Z')
			{
				is->insert(v+('a'-'A')); 
			}
			if (v >= 'a' && v <= 'z')
			{
				is->insert(v-('a'-'A'));
			}
		}
		range->add(is);
		delete is;
	}

	if (negate != negate_re)
	{					// completement the range
		range->negate();
	}

	ptr++;
	if (ptr == strlen(re) || !is_repetition(re[ptr]))
	{
		(*fa) = (*fa)->add_transition(range);
	}
	else
	{
		(*to_link) = new NFA();
		(*to_link) = (*to_link)->add_transition(range);
	}
	delete range;
	return ptr;
}

int parser::process_quantifier(const char *re, int ptr, int *lb_p, int *ub_p)
{
	int lb = 0, ub = _INFINITY;		// default {0,}
	int res = sscanf(re+ptr, "%d", &lb);	// Actually read the number from the string starting from regex[ptr]
	if (res != 1)
	{
		error("parser:: process_quantifier: invalid quantified expression.");
	}
	while (ptr < strlen(re) && re[ptr] != _COMMA_ && re[ptr] != _CLOSE_LBRACKET_)
	{					// skip the number scanned before
		ptr++;
	}
	if (ptr == strlen(re) || (re[ptr] == _COMMA_ && ptr == (strlen(re)-1)))
	{					// avoid the situation {N and {N,
		error("parser:: process_quantifier: quantified expression not closed.");
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
				error("parser:: process_quantifier: invalid quantified expression.");
			}
		}				// default the case {N,}
	}
	else
	{
		error("parser:: process_quantifier: invalid quantified expression.");
	}
	while (re[ptr] != _CLOSE_LBRACKET_)
	{					// skip the number scanned before
		ptr++;
		if (ptr == strlen(re))
		{
			error("parser:: process_quantifier: quantified expression not closed.");
		}
	}
	(*lb_p) = lb;
	(*ub_p) = ub;
	ptr++;
	return ptr;
	}
