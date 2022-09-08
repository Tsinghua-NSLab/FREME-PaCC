#include "segment.h"

segment::segment()
{
}

segment::~segment()
{
}

void segment::scan(FILE *file, int from, int to)
{
	rewind(file);
	int i = 0, j = 0;
	unsigned int c;
	char *re = allocate_char_array(2048);
	seg_t *segs[4096] = {NULL};
	
	do
	{
		c = fgetc(file);
		if (c=='\n' || c=='\r' || c==EOF)
		{
			if (i != 0)
			{
				re[i] = EOS;
				if (re[0] != '#')
				{
					j++;
					if (j>=from && (to==-1 || j<=to))
					{
						if (DEBUG)
						{
							fprintf(stderr, "\n@%-4d  %s\n", j, re);
						}
						parse_re(re, j-1, segs);
					} // endif j
				} // endif re
				i = 0;
				free(re);
				re = allocate_char_array(2048);
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

	enum_segs(segs, j);
	swap_segs(segs, j);
	mark_segs(segs, j);
	puts_file(segs, j, "segment.result");
}

int segment::puts_file(seg_t *segs[], int rulenum, const char *newfile)
{
	FILE *fp = fopen(newfile, "w");
	for (int i=0; i<rulenum; i++)
	{
		if (segs[i]->id == -1)
		{
			fprintf(fp, "%s\n", segs[i]->tail.c_str());
			continue;
		}
		for (int j=0; j<segs[i]->len-segs[i]->offset; j++)
		{
			fprintf(fp, "%s", segs[i]->head->front().c_str());
			segs[i]->head->pop_front();
		}
		fprintf(fp, "(");
		for (int j=0; j<segs[i]->offset; j++)
		{
			fprintf(fp, "%s", segs[i]->head->front().c_str());
			segs[i]->head->pop_front();
		}
		fprintf(fp, ")%s%s\n", segs[i]->cf.c_str(), segs[i]->tail.c_str());
		free(segs[i]);
	}
	fclose(fp);
	return 0;
}

int segment::mark_segs(seg_t *segs[], int rulenum)
{
	string itstr, isstr;
	for (int i=0; i<rulenum; i++)
	{
		if (segs[i]->id == -1)
		{
			continue;
		}
		itstr.clear();
		str_list::iterator it=segs[i]->head->end();
		for (int k=0; k<segs[i]->offset; k++)
		{
			it--;
			itstr.append(*it);
		}
		for (int j=0; j<i; j++)
		{
			if (segs[j]->id == -1)
			{
				continue;
			}
			if (segs[i]->id != segs[j]->id && segs[i]->offset == segs[j]->offset)
			{
				isstr.clear();
				str_list::iterator is=segs[j]->head->end();
				for (int k=0; k<segs[j]->offset; k++)
				{
					is--;
					isstr.append(*is);
				}
				if (itstr.compare(isstr) == 0)
				{
					if (segs[i]->offset < segs[i]->len-1)
					{
						segs[i]->offset++;
					}
				}
			}
		}
	}
	return 0;
}

int segment::swap_segs(seg_t *segs[], int rulenum)
{
	const char *srcfile = "segsrc.file";
	const char *dstfile = "segdst.file";
	seg_t *tmpsegs[4096] = {NULL};
	int  seq[4096] = {0};
	char cmd[2048] = {0};
	FILE *fp = NULL;
	string itstr;
	fp = fopen(srcfile, "w");
	for (int i=0; i<rulenum; i++)
	{
		if (segs[i]->id == -1)
		{
			fprintf(fp, "0 #%04d\n", i);
			continue;
		}
		itstr.clear();
		itstr.assign(segs[i]->cf);
		str_list::iterator it=segs[i]->head->end();
		for (int j=0; j<segs[i]->len; j++)
		{
			it--;
			itstr.append(*it);
		}
		fprintf(fp, "%s #%04d\n", itstr.c_str(), i);
	}
	fclose(fp);
	sprintf(cmd, "sort %s > %s", srcfile, dstfile);
	system(cmd);
	fp = fopen(dstfile, "r");
	for (int i=0; i<rulenum; i++)
	{
		fscanf(fp, "%s #%d\n", cmd, &seq[i]);
		tmpsegs[i] = segs[seq[i]];
	}
	fclose(fp);
	//sprintf(cmd, "rm -f %s %s", srcfile, dstfile);
	//system(cmd);
	for (int i=0; i<rulenum; i++)
	{
		segs[i] = tmpsegs[i];
	}
	return 0;
}

int segment::enum_segs(seg_t *segs[], int rulenum)
{
	int id = 0;
	bool done = false;

	for (int i=0; i<rulenum; i++)
	{
		if (segs[i]->id == -1)
		{
			continue;
		}
		for (int j=0; j<i; j++)
		{
			if (segs[j]->id == -1)
			{
				continue;
			}
			if (segs[i]->cf.compare(segs[j]->cf) == 0)
			{
				segs[i]->id = segs[j]->id;
				done = true;
			}
		}
		if (!done)
		{
			id++;
			segs[i]->id = id;
		}
	}
	return 0;
}

int segment::parse_re(const char *re, int rule, seg_t *segs[])
{
	segs[rule] = new seg_t();
	segs[rule]->head = new str_list();
	seg_t *seg = segs[rule];
	string res(re);
	int ptr = 0, b_start = 0, b_times = 0;
	bool chaos_mark = false, start_mark = true;

	/* Processing the given regex character by character via the ptr */
	while (ptr < strlen(re))
	{
		int tmp;
		string s;
		if (re[ptr] == _ESCAPE_)
		{				// when meet the meta character '\'
			if (ptr == (strlen(re)-1))
			{
				error("segment:: split_re: '\\' in last position.");
			}
			else
			{
				tmp = ptr;
				ptr = process_escape(re, ptr+1);
				s.assign(res, tmp, ptr-tmp);
				if (ptr == strlen(re))
				{
					segs[rule]->id = -1;
					break;
				}
				else if (b_times == 0)
				{
					segs[rule]->head->push_back(s);
					chaos_mark = false;
				}
			}
		}
		else if (re[ptr] == _DOT_)
		{				// when meet the meta character '.'
			s.assign(res, ptr, 1);
			ptr++;
			if (ptr == strlen(re))
			{
				segs[rule]->id = -1;
				break;
			}
			else if (b_times == 0)
			{
				segs[rule]->head->push_back(s);
				chaos_mark = true;
			}
		}
		else if (re[ptr] == _QUES_)
		{				// when meet the meta character '?'
			s.assign(res, ptr, 1);
			ptr++;
			if (ptr == strlen(re))
			{
				segs[rule]->id = -1;
				break;
			}
			else if (b_times == 0)
			{
				segs[rule]->head->back().append(s);
				chaos_mark = false;
			}
		}
		else if (re[ptr] == _STAR_)
		{				// when meet the meta character '*'
			s.assign(res, ptr, 1);
			ptr++;
			if (ptr == strlen(re))
			{
				segs[rule]->id = -1;
				break;
			}
			else if (b_times == 0)
			{
				segs[rule]->head->back().append(s);
				if (chaos_mark)
				{
					segs[rule]->id = 0;
					break;
				}
				else
				{
					chaos_mark = false;
				}
			}
		}
		else if (re[ptr] == _PLUS_)
		{				// when meet the meta character '+'
			s.assign(res, ptr, 1);
			ptr++;
			if (ptr == strlen(re))
			{
				segs[rule]->id = -1;
				break;
			}
			else if (b_times == 0)
			{
				segs[rule]->head->back().append(s);
				if (chaos_mark)
				{
					segs[rule]->id = 0;
					break;
				}
				else
				{
					chaos_mark = false;
				}
			}
		}
		else if (re[ptr] == _OPEN_LBRACKET_)
		{				// when meet the meta character '{'
			if (ptr == (strlen(re)-1))
			{
				error("segment:: split_re: '{' in last position.");
			}
			else
			{
				tmp = ptr;
				int lb = 0, ub = _INFINITY;
				ptr = process_quantifier(re, ptr+1, &lb, &ub);
				s.assign(res, tmp, ptr-tmp);
				if (ptr == strlen(re))
				{
					segs[rule]->id = -1;
					break;
				}
				else if (b_times == 0)
				{
					segs[rule]->head->back().append(s);
					if (chaos_mark)
					{
						segs[rule]->id = 0;
						break;
					}
					else
					{
						chaos_mark = false;
					}
				}
			}
		}
		else if (re[ptr] == _OPEN_MBRACKET_)
		{				// when meet the meta character '['
			if (ptr == (strlen(re)-1))
			{
				error("segment:: split_re: '[' in last position.");
			}
			else
			{
				tmp = ptr;
				bool sb = false;
				ptr = process_range(re, ptr+1, &sb);
				s.assign(res, tmp, ptr-tmp);
				if (ptr == strlen(re))
				{
					segs[rule]->id = -1;
					break;
				}
				else if (b_times == 0)
				{
					segs[rule]->head->push_back(s);
					chaos_mark = sb;
				}
			}
		}
		else if (re[ptr] == _OPEN_SBRACKET_)
		{				// when meet the meta character '('
			if (ptr == (strlen(re)-1))
			{
				error("segment:: split_re: '(' in last position.");
			}
			else
			{
				if (b_times == 0)
				{
					b_start = ptr;
				}
				ptr++;
				b_times++;
			}
		}
		else if (re[ptr] == _CLOSE_SBRACKET_)
		{				// when meet the meta character ')'
			ptr++;
			b_times--;
			if (b_times == 0)
			{
				s.assign(res, b_start, ptr-b_start);
				if (ptr == strlen(re))
				{
					segs[rule]->id = -1;
					break;
				}
				else
				{
					segs[rule]->head->push_back(s);
					chaos_mark = false;
				}
			}
			else if (b_times < 0)
			{
				error("segment:: split_re: close ')' without opening '('.");
			}
			else if (ptr == strlen(re))
			{
				error("segment:: split_re: open '(' without closing ')'.");
			}
		}
		else
		{				// when meet the common character
			s.assign(res, ptr, 1);
			ptr++;
			if (ptr == strlen(re))
			{
				segs[rule]->id = -1;
				break;
			}
			else if (b_times == 0)
			{
				segs[rule]->head->push_back(s);
				chaos_mark = false;
			}
		}
		s.clear();
	} // endwhile ptr
	if (segs[rule]->id == 0)
	{
		segs[rule]->cf.assign(segs[rule]->head->back());
		segs[rule]->head->pop_back();
		segs[rule]->len = segs[rule]->head->size();
		segs[rule]->offset = 1; // offset must be smaller than len
		segs[rule]->tail.assign(res, ptr, strlen(re));
	}
	else
	{
		segs[rule]->tail.assign(res, 0, strlen(re));
		while (!segs[rule]->head->empty())
		{
			segs[rule]->head->pop_front();
		}
	}
	return 0;
}

int segment::process_escape(const char *re, int ptr)
{
	if (ptr == strlen(re))
	{					// basic regex syntax checking
		error("segment:: process_escape: uncomplete escape sequence.");
	}

	char c = re[ptr];			// the character behind the meta character '\'

	if (being_x(c))
	{					// hex escape sequence
		if (ptr > (strlen(re)-3))
		{				// handle the illegal case \x and \xN
			error("segment::process_escape: invalid hex escape sequence.");
		}
		else if (!being_hex_digit(re[ptr+1]) || !being_hex_digit(re[ptr+2]))
		{				// handle the illegal case \xNN with N is not hex
			error("segment::process_escape: invalid hex escape sequence.");
		}
		else
		{				// handle the legal case \xNN standing for a ASCII character
			ptr = ptr + 3;
		}
	}
	else if (being_oct_digit(c))
	{					// octal escape sequence
		if (ptr > (strlen(re)-3))
		{				// handle the common case \N
			ptr++;
		}
		else if (!being_oct_digit(re[ptr+1]) || !being_oct_digit(re[ptr+2]))
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

int segment::process_range(const char *re, int ptr, bool *sb_p)
{
	bool sb = false;
	if (ptr == strlen(re))
	{					// basic regex syntax checking
		error("segment:: process_range: range expression not closed.");
	}
	if (re[ptr] == _CLOSE_MBRACKET_)
	{					// basic regex syntax checking
		error("segment:: process_range: invalid range expression.");
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
		error("segment:: process_range: range expression not closed.");
	}
	else
	{
		ptr++;
	}
	(*sb_p) = sb;
	return ptr;
}

int segment::process_quantifier(const char *re, int ptr, int *lb_p, int *ub_p)
{
	int lb = 0, ub = _INFINITY;		// default {0,}
	int res = sscanf(re+ptr, "%d", &lb);	// Actually read the number from the string starting from regex[ptr]
	if (res != 1)
	{
		error("segment:: process_quantifier: invalid quantified expression.");
	}
	while (ptr < strlen(re) && re[ptr] != _COMMA_ && re[ptr] != _CLOSE_LBRACKET_)
	{					// skip the number scanned before
		ptr++;
	}
	if (ptr == strlen(re) || (re[ptr] == _COMMA_ && ptr == (strlen(re)-1)))
	{					// avoid the situation {N and {N,
		error("segment:: process_quantifier: quantified expression not closed.");
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
				error("segment:: process_quantifier: invalid quantified expression.");
			}
		}				// default the case {N,}
	}
	else
	{
		error("segment:: process_quantifier: invalid quantified expression.");
	}
	while (re[ptr] != _CLOSE_LBRACKET_)
	{					// skip the number scanned before
		ptr++;
		if (ptr == strlen(re))
		{
			error("segment:: process_quantifier: quantified expression not closed.");
		}
	}
	ptr++;
	(*lb_p) = lb;
	(*ub_p) = ub;
	return ptr;
}
