/*
 * FILE:	 spliter.h
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 * 
 * 		This class implements a regular expression spliter.
 *		Refer to ZTE contract requirements, PCRE standard specifications, http://www.regular-expressions.info web pages.
 * 		Use the spliter function to spliter a file containing a list of regular expressions and generate the corresponding NFA.
 * 
 */

#ifndef __SPLIT_H__
#define __SPLIT_H__

#include "stdinc.h"

#include <string>
#include <list>
#include <utility>
#include <fstream>
#include <iostream>

using namespace std;
typedef list<string> str_list;
typedef list<pair<int, string> *> pair_list;
struct entry_attr
{
	int  stage;
	bool ending;
	int  next_dfatype;
};
typedef struct entry_attr entry_attr_t;
typedef list<entry_attr_t *> attr_list;

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
 
class spliter
{

	pair_list *front_end;

	pair_list *back_end;

	attr_list *removes;

	int anchor_entry;
	
public:

	/*
	 * Instantiates the spliter
	 */
	spliter();
	
	/*
	 * The spliter de-allocator
	 */
	~spliter();
	
	/*
	 * Parses all the regular expressions contained in file and returns the corresponding NFA
	 */
	void split(FILE *file, int from=1, int to=-1);
	
private:

	/*
	 * Parses a regular expressions into the given NFA
	 */
	void split_re(const char *re, int rule, int &sub_rule);

	/*
	 * Parses a substring of a regular expression and returns the corresponding NFA
	 */
	str_list *split_re(const char *re, int *ptr, bool b_status, int *b_times, bool encap_mask);

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


	void make_form(str_list **fa, string *s);


	void make_copy(str_list **fa);


	void make_link(str_list **fa, string *s);


	void make_and(str_list **fa, str_list *to_link);


	void make_or(str_list **fa, str_list *to_link);


	void encap(str_list **fa);


	void mark_point(string *res);


	void parse_re(const char *re, int rule, int sub_rule);


	void inserts(string *s, int dfatype, int next_dfatype, int rule, int sub_rule, int stage, bool ending);


	void to_file();

};

/*
 * Returns true if the given character is meta character
 */
inline bool be_meta(char c)
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
inline bool be_dec_digit(char c)
{
	return ((c>='0') && (c<='9'));
}

/*
 * Returns true if the character is a oct. digit
 */
inline bool be_oct_digit(char c)
{
	return ((c>='0') && (c<='7'));
}

/*
 * Returns true if the character is a hex. digit
 */
inline bool be_hex_digit(char c)
{
	return (be_dec_digit(c) || (c>='a' && c<='f') || (c>='A' && c<='F'));
}

/*
 * Returns true if the character represents a (bounded or unbounded) repetition
 */
inline bool be_repetition(char c)
{
	return ((c==_STAR_) || (c==_PLUS_) || (c==_QUES_) || (c==_OPEN_LBRACKET_));
};

/*
 * Returns true if the character is x or X
 */
inline bool be_x(char c)
{
	return ((c=='x') || (c=='X'));
}

#endif /*__SPLIT_H__*/
