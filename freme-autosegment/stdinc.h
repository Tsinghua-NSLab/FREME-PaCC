/*
 * FILE:	 stdinc.h
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 *
 *		Contains constant definitions, warning and message functions, 
 * 		memory allocation and de-allocation functions used all over the code.
 * 
 */

#ifndef __STDINC_H__
#define __STDINC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* 
 * Configuration 
 */
#define REGEX_BUFFER	1000		// regex parseing, the max memory to buffer a regex
#define COMMON_TARGET_REDUCTION		// NFA reduction, defines whether transitions to common target must be handled separately
#define MAX_DFA_SIZE	5000000		// DFA construction, define the max size of a DFA
#define MAX_DFAS_NUM	3

#define CARET_RULE	"ruleset/caret_rule.re"
#define ANY_RULE	"ruleset/any_rule.re"
#define STAR_RULE	"ruleset/star_rule.re"
#define TIME_RULE	"ruleset/time_rule.re"
#define INDEX_TABLE	"ruleset/index.table"

/*
 * Type definitions: state identifiers and symbols
 */
typedef unsigned int	state_t;
typedef unsigned	symbol_t;
typedef unsigned short	label_t;

/*
 * Constant definition
 */
#define CSIZE		256		// size of the alphabet
#define NO_STATE	0xFFFFFFFF	// invalid state representation
#define ANY		0xFFFFFFFF	// no time limitation
#define _INFINITY	-1

/*
 * Verbosity levels
 */
extern int VERBOSE;
extern int DEBUG; 

const int BIGINT = 0x7FFFFFFF;
const int EOS = '\0';

/*
 * Functions for warning and error processing
 */
inline void warning(char *s) { fprintf(stderr, "WARNING: %s\n", s); }
inline void error(char *s) { fprintf(stderr, "ERROR: %s\n", s); exit(1); }

/*
 * Functions for max and min comparison
 */
inline unsigned int max(unsigned int x, unsigned int y) { return x > y ? x : y; }
inline int max(int x, int y) { return x > y ? x : y; }
inline double max(double x, double y) { return x > y ? x : y; }
inline unsigned int min(unsigned int x, unsigned int y) { return x < y ? x : y; }
inline int min(int x, int y) { return x < y ? x : y; }
inline double min(double x, double y) { return x < y ? x : y; }

/*
 * Memory allocation operations
 */
void *allocate_array(int size, size_t element_size);
void *reallocate_array(void *array, int size, size_t element_size);

#define allocate_char_array(size)		(char *) allocate_array( size, sizeof( char ) )
#define allocate_string_array(size)		(char **) allocate_array( size, sizeof( char * ) )
#define allocate_int_array(size)		(int *) allocate_array( size, sizeof( int ) )
#define allocate_uint_array(size)		(unsigned int *) allocate_array( size, sizeof( unsigned int ) )
#define allocate_bool_array(size)		(bool *) allocate_array( size, sizeof( bool ) )
#define allocate_state_array(size)		(state_t *) allocate_array( size, sizeof( state_t ) )
#define allocate_state_matrix(size)		(state_t **) allocate_array( size, sizeof( state_t * ) )

#define reallocate_char_array(size)		(char *) reallocate_array( array, size, sizeof( char ) )
#define reallocate_string_array(array,size)	(char **) reallocate_array( array, size, sizeof( char * ) )
#define reallocate_int_array(array,size)	(int *) reallocate_array( array, size, sizeof( int ) )
#define reallocate_uint_array(array,size)	(unsigned int *) reallocate_array( array, size, sizeof( unsigned int ) )
#define reallocate_bool_array(array,size)	(bool *) reallocate_array( array,size, sizeof( bool ) )
#define reallocate_state_array(array,size)	(state_t *) reallocate_array( array, size, sizeof( state_t ) )
#define reallocate_state_matrix(array,size)	(state_t **) reallocate_array( array, size, sizeof( state_t * ) )

#endif /*__STDINC_H__*/
