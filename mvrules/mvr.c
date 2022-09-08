/*
 * Copyright (c) 2012 Kyle Wang and Tsinghua University in Beijing.
 * All rights reserved
 */

/*
 *
 *	File:			mvr.c
 *	Description:		Modify the sensitive content of each and
 *				every signature described as regular
 *				expressions
 *
 *	Organization:		Venus Team, Network Security Laboratory
 *				Tsinghua University
 *	Author:			Kyle Wang (woncy2005@gmail.com)
 *
 *	Version:		1.0.1
 *	Date:			May 31, 2012
 *	Created:		May 31, 2012
 *	Last Modified:		May 31, 2012
 *
 *	1. Randomize the content of the rule file from NSFocus Corporation
 *	   and generate a new rule file.
 *	2. The rule starting with '#' means that it is annotated, and will
 *	   not be tackled
 *
 */


#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"
#include "math.h"
#include "time.h"

/*
 * Definition of meta characters in regular expression syntax
 */
#define _ESCAPE_		'\\'
#define _CARET_			'^'
#define _DOLLAR_		'$'
#define _OR_			'|'
#define _STAR_			'*'
#define _PLUS_			'+'
#define _QUES_			'?'
#define _OPEN_SBRACKET_		'('
#define _DOT_			'.'
#define _CLOSE_SBRACKET_	')'
#define _OPEN_MBRACKET_		'['
#define _DASH_			'-'
#define _CLOSE_MBRACKET_	']'
#define _OPEN_LBRACKET_		'{'
#define _COMMA_			','
#define _CLOSE_LBRACKET_	'}'

int DEBUG = 1;

unsigned int randomize(unsigned int k)
{
	// Don't modify the interpunctions
	if ((k>=0x20&&k<=0x2F) || (k>=0x3A&&k<=0x40) || (k>=0x5B&&k<=0x60) || (k>=0x7B&&k<=0x7E))
	{
		return k;
	}

	unsigned int m, n, c;
	if (k <= 0x1F)
	{
		m = 0x00;
		n = 0x1F;
	}
	else if (k <= 0x3F)
	{
		m = 0x30;
		n = 0x39;
	}
	else if (k <= 0x5F)
	{
		m = 0x41;
		n = 0x5A;
	}
	else if (k <= 0x7F)
	{
		m = 0x61;
		n = 0x7A;
	}
	else
	{
		m = 0x80;
		n = 0xFF;
	}

	struct timeval seed;
	gettimeofday(&seed, NULL);
	srand(seed.tv_usec);

	c = m + rand() % (n - m);
	return c;
}

char *allocate_char_array(int size)
{
	char *array = malloc(size * sizeof(char));
	if (!array)
	{   
		error("mvr::allocate_char_array: memory allocation failed");
	}
        return array;
}

int be_meta(char c)
{
	return ((c==_ESCAPE_)||(c==_CARET_)||(c==_DOLLAR_)||
		(c==_STAR_)||(c==_PLUS_)||(c==_QUES_)||
		(c==_OPEN_SBRACKET_)||(c==_CLOSE_SBRACKET_)||
		(c==_OPEN_MBRACKET_)||(c==_CLOSE_MBRACKET_)||
		(c==_OPEN_LBRACKET_)||(c==_CLOSE_LBRACKET_)||
		(c==_DASH_)||(c==_COMMA_)||
		(c==_DOT_)||(c==_OR_));
}

int be_dec_digit(char c)
{
	return ((c>='0') && (c<='9'));
}

int be_oct_digit(char c)
{
	return ((c>='0') && (c<='7'));
}

int be_hex_digit(char c)
{
	return (be_dec_digit(c) || (c>='a' && c<='f') || (c>='A' && c<='F'));
}

int be_x(char c)
{
	return ((c=='x') || (c=='X'));
}

int process_escape(const char *re, int ptr)
{
	if (ptr == strlen(re))
	{					// basic regex syntax checking
		error("mvr::process_escape: uncomplete escape sequence.");
	}

	char c = re[ptr];			// the character behind the meta character '\'

	if (be_x(c))
	{					// hex escape sequence
		if (ptr > (strlen(re)-3))
		{				// handle the illegal case \x and \xN
			error("mvr::process_escape: invalid hex escape sequence.");
		}
		else if (!be_hex_digit(re[ptr+1]) || !be_hex_digit(re[ptr+2]))
		{				// handle the illegal case \xNN with N is not hex
			error("mvr::process_escape: invalid hex escape sequence.");
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

int process_range(const char *re, int ptr)
{
	if (ptr == strlen(re))
	{					// basic regex syntax checking
		error("mvr::process_range: range expression not closed.");
	}
	if (re[ptr] == _CLOSE_MBRACKET_)
	{					// basic regex syntax checking
		error("mvr::process_range: invalid range expression.");
	}

	if (re[ptr] == _CARET_)
	{					// handle the case [^...]
		ptr++;
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
		error("mvr::process_range: range expression not closed.");
	}
	else
	{
		ptr++;
	}
	return ptr;
}

int process_quantifier(const char *re, int ptr)
{
	int lb = 0, ub = -1;			// default {0,}
	int res = sscanf(re+ptr, "%d", &lb);	// Actually read the number from the string starting from regex[ptr]
	if (res != 1)
	{
		error("mvr::process_quantifier: invalid quantified expression.");
	}
	while (ptr < strlen(re) && re[ptr] != _COMMA_ && re[ptr] != _CLOSE_LBRACKET_)
	{					// skip the number scanned before
		ptr++;
	}
	if (ptr == strlen(re) || (re[ptr] == _COMMA_ && ptr == (strlen(re)-1)))
	{					// avoid the situation {N and {N,
		error("mvr::process_quantifier: quantified expression not closed.");
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
				error("mvr::process_quantifier: invalid quantified expression.");
			}
		}				// default the case {N,}
	}
	else
	{
		error("mvr::process_quantifier: invalid quantified expression.");
	}
	while (re[ptr] != _CLOSE_LBRACKET_)
	{					// skip the number scanned before
		ptr++;
		if (ptr == strlen(re))
		{
			error("mvr::process_quantifier: quantified expression not closed.");
		}
	}
	ptr++;
	return ptr;
}

void parse_re(char *rq, const char *rp, int *ptr)
{
	char *rx = allocate_char_array(20);

	memset(rq, 0, 2000);

	/* Processing the given regex character by character via the ptr */
	while ((*ptr) < strlen(rp))
	{
		if (rp[(*ptr)] == '\\')
		{				// when meet the meta character '\', randomize
			int tmp = (*ptr);
			(*ptr) = process_escape(rp, (*ptr)+1);
			if ((*ptr)-tmp == 4)
			{
				unsigned int c = rp[(*ptr)];
				sprintf(rx, "\\x%x", randomize(c));
			}
			else
			{
				memcpy(rx, &rp[tmp], (*ptr)-tmp);
			}
			strcat(rq, rx);
		}
		else if (!be_meta(rp[(*ptr)]))
		{				// when meet the common characters, randomize
			unsigned int c = rp[(*ptr)];
			sprintf(rx, "%c", randomize(c));
			strcat(rq, rx);
			(*ptr)++;
		}
		else if (rp[(*ptr)] == _OPEN_LBRACKET_)
		{				// when meet the meta character '{'
			int tmp = (*ptr);
			(*ptr) = process_quantifier(rp, (*ptr)+1);
			memcpy(rx, &rp[tmp], (*ptr)-tmp);
			strcat(rq, rx);
		}
		else if (rp[(*ptr)] == _OPEN_MBRACKET_)
		{				// when meet the meta character '['
			int tmp = (*ptr);
			(*ptr) = process_range(rp, (*ptr)+1);
			memcpy(rx, &rp[tmp], (*ptr)-tmp);
			strcat(rq, rx);
		}
		else
		{				// when meet other meta characters
			sprintf(rx, "%c", rp[(*ptr)]);
			strcat(rq, rx);
			(*ptr)++;
		}
		memset(rx, 0, 20);
	}
	free(rx);
}

void parse(FILE *fq, const char *rp)
{
	char *rq = allocate_char_array(2000);
	int ptr = 0;
	parse_re(rq, rp, &ptr);
	fprintf(fq, "%s\n", rq);
	free(rq);
}

/*
 * Program entry point
 */
int main(int argc, char **argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "\n./mvr <old_regex_file> <new_regex_file>\n\n");
		exit(0);
	}

	int i = 0, j = 0, from = 1, to = -1;
	unsigned int c;
	FILE *fp = fopen(argv[1], "r");
	FILE *fq = fopen(argv[2], "w");
	char *rp = allocate_char_array(2000);
	do
	{
		c = fgetc(fp);
		if (c=='\n' || c=='\r' || c==EOF)	// for the regex rule of each line
		{
			if (i != 0)			// for the regex (line) that is not blank
			{
				rp[i] = '\0';
				if (rp[0] != '#')	// for the regex (line) that is not annotated (starting with '#' means annotation)
				{
					j++;
					if (j>=from && (to==-1 || j<=to))
					{
						if (DEBUG)
						{
							fprintf(stdout, "\n@%-4d  %s\n", j, rp);
						}
						parse(fq, rp);
					} // endif j
				} // endif re
				i = 0;
				free(rp);
				rp = allocate_char_array(2000);
			} // endif i
		} // endif c
		else
		{
			rp[i++] = c;
		}
	} while (c != EOF);

	if (rp != NULL)
	{
		free(rp);
	}
	fclose(fp);
	fclose(fq);

	return 0;
}
