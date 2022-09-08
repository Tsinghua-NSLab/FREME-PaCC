/*
 * FILE:	 int_set.h
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 * 
 * 		Implements a  set of positive integer
 *
 */

#ifndef __INT_SET_H__
#define __INT_SET_H__

#include "stdinc.h"

#define UNDEF 0xFFFFFFFF

class int_set
{

	unsigned int N;			// max number of the elements in the set

	bool *item;			// item[i] is true if element i is in the set

	unsigned int num;		// actual number of the elements in the set

	unsigned int first;		// the first element in the set
	
public:

	/*
	 * Constructor
	 */
	int_set(unsigned int=100);

	/*
	 * De-allocator
	 */
	~int_set();

	/*
	 * Set assignment
	 */
	void operator=(int_set &);

	/*
	 * Insert the given item in the set
	 */
	void insert(unsigned int);

	/*
	 * Remove the given item from the set
	 */
	void remove(unsigned int);

	/*
	 * Return ture if the given item is a element of the set
	 */
	bool member(unsigned int);

	/*
	 * Return ture if the set is empty
	 */
	bool empty();

	/*
	 * Return the size of the set
	 */
	unsigned int size();

	/*
	 * Return the first item in the set
	 */
	unsigned int head();

	/*
	 * Return the successor item in the set
	 */
	unsigned int succ(unsigned int);

	/*
	 * Remove all the elements from the set.
	 */
	void clear();

	/*
	 * Change num of the set with discarding the old elements.
	 */
	void reset(unsigned int);

	/*
	 * Print the items in the set
	 */
	void print();

	/*
	 * Add all the elements in the given set
	 */
	void add(int_set *);

	/*
	 * Complement the set
	 */
	void negate();

};

inline unsigned int int_set::size() { return num; }

inline unsigned int int_set::head() { return first; }

inline bool int_set::empty() { return (num == 0); }

#endif /*__INT_SET_H__*/
