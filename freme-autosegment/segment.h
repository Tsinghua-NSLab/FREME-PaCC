/*
 * FILE:	segment.h
 * ORGANIZATION: Network Security Laboratory
 *
 * DESCRIPTION:
 *
 *		This class implements a regular expression segmentation algorithm.
 *		Refer to ZTE contract requirements, PCRE standard specifications, http://www.regular-expressions.info web pages.
 *		Use the spliter function to spliter a file containing a list of regular expressions and generate the corresponding NFA.
 *
 */

#ifndef __SEGMENT_H__
#define __SEGMENT_H__

#include "stdinc.h"

#include <string>
#include <list>
#include <utility>
#include <algorithm>

using namespace std;
typedef list<string> str_list;
typedef struct seg_s
{
	int id;
	string cf;
	str_list *head;
	int len;
	int offset;
	string tail;
} seg_t;

/*
 * Definition of meta characters in regular expression syntax
 */
#define _ESCAPE_		'\\'		// specialize, normalize, back-reference, or octalize the characters
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
 
class segment
{

public:

	/*
	 * Instantiates the segment
	 */
	segment();
	
	/*
	 * The segment de-allocator
	 */
	~segment();
	
	void scan(FILE *file, int from=1, int to=-1);
	
private:

	int parse_re(const char *re, int rule, seg_t *segs[]);

	int enum_segs(seg_t *segs[], int rulenum);

	int swap_segs(seg_t *segs[], int rulenum);

	int mark_segs(seg_t *segs[], int rulenum);

	int puts_file(seg_t *segs[], int rulenum, const char *newfile);

	/*
	 * Process an escape sequence
	 */
	int process_escape(const char *re, int ptr);

	/*
	 * Process a range of characters [-]
	 */
	int process_range(const char *re, int ptr, bool *sb_p);
	
	/*
	 * Process a bounded repetition {,}
	 */
	int process_quantifier(const char *re, int ptr, int *lb_p, int *ub_p);

};

/*
 * Returns true if the given character is meta character
 */
inline bool being_meta(char c)
{
	return ((c==_ESCAPE_)||(c==_STAR_)||(c==_PLUS_)||(c==_QUES_)||
		(c==_OPEN_SBRACKET_)||(c==_CLOSE_SBRACKET_)||
		(c==_OPEN_MBRACKET_)||(c==_CLOSE_MBRACKET_)||
		(c==_OPEN_LBRACKET_)||(c==_CLOSE_LBRACKET_)||
		(c==_DOT_)||(c==_OR_));
}

/*
 * Returns true if the character is a dec. digit
 */
inline bool being_dec_digit(char c)
{
	return ((c>='0') && (c<='9'));
}

/*
 * Returns true if the character is a oct. digit
 */
inline bool being_oct_digit(char c)
{
	return ((c>='0') && (c<='7'));
}

/*
 * Returns true if the character is a hex. digit
 */
inline bool being_hex_digit(char c)
{
	return (being_dec_digit(c) || (c>='a' && c<='f') || (c>='A' && c<='F'));
}

/*
 * Returns true if the character represents a (bounded or unbounded) repetition
 */
inline bool being_repetition(char c)
{
	return ((c==_STAR_) || (c==_PLUS_) || (c==_QUES_) || (c==_OPEN_LBRACKET_));
};

/*
 * Returns true if the character is x or X
 */
inline bool being_x(char c)
{
	return ((c=='x') || (c=='X'));
}

#endif /* __SEGMENT_H__ */
