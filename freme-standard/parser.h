/*
 * FILE:	 parser.h
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 * 
 * 		This class implements a regular expression parser.
 *		Refer to ZTE contract requirements, PCRE standard specifications, http://www.regular-expressions.info web pages.
 * 		Use the parse function to parse a file containing a list of regular expressions and generate the corresponding NFA.
 * 
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#include "stdinc.h"
#include "int_set.h"
#include "nfa.h"
#include "dfa.h"

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
 
class parser
{
	
	bool m_modifier;	// <=>(?-m)(?m) '^' and '$' match the start and end of the string(=0), or match after and before '\n'(=1)

	bool s_modifier;	// <=>(?-s)(?s) '.' matches [^\n](=0), or [\x00-\xFF](=1)

	bool i_modifier;	// <=>(?-i)(?i) case sensitive(=0) and insensitive(=1)

public:

	/*
	 * Instantiates the parser
	 */
	parser(bool m_mod, bool s_mod, bool i_mod);
	
	/*
	 * The parser de-allocator
	 */
	~parser();
	
	/*
	 * Parses all the regular expressions contained in file and returns the corresponding NFA
	 */
	NFA *parse(FILE *file, int from=1, int to=-1);
	
private:

	/*
	 * Parses a regular expressions into the given NFA
	 */
	void *parse_re(NFA* nfa, const char *re);

	/*
	 * Parses a substring of a regular expression and returns the corresponding NFA
	 */
	NFA *parse_re(const char *re, int *ptr, bool bracket, bool negate_re);
	
	/*
	 * Process an escape sequence
	 */
	int process_escape(const char *re, int ptr, int_set *chars);

	/*
	 * Process a range of characters [-]
	 */
	int process_range(const char *re, int ptr, NFA **fa, NFA **to_link, bool negate_re);
	
	/*
	 * Process a bounded repetition {,}
	 */
	int process_quantifier(const char *re, int ptr, int *lb, int *ub);

};

/*
 * Returns true if the given character is meta character
 */
inline bool is_meta(char c)
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
inline bool is_dec_digit(char c)
{
	return ((c>='0') && (c<='9'));
}

/*
 * Returns true if the character is a oct. digit
 */
inline bool is_oct_digit(char c)
{
	return ((c>='0') && (c<='7'));
}

/*
 * Returns true if the character is a hex. digit
 */
inline bool is_hex_digit(char c)
{
	return (is_dec_digit(c) || (c>='a' && c<='f') || (c>='A' && c<='F'));
}

/*
 * Returns true if the character represents a (bounded or unbounded) repetition
 */
inline bool is_repetition(char c)
{
	return ((c==_STAR_) || (c==_PLUS_) || (c==_QUES_) || (c==_OPEN_LBRACKET_));
};

/*
 * Returns true if the character is x or X
 */
inline bool is_x(char c)
{
	return ((c=='x') || (c=='X'));
}

/*
 * Returns the escaped form of the character
 */
inline char escaped(char c)
{
	switch (c)
	{
		case 'a': return '\a';
		case 'b': return '\b';
		case 't': return '\t';
		case 'n': return '\n';
		case 'v': return '\v';
		case 'f': return '\f';
		case 'r': return '\r';
		default : return c;
	}
}

#endif /*__PARSER_H__*/
