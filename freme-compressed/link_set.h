/*
 * FILE:	 link_set.h
 * ORGANIZATION: Network Security Laboratory
 * 
 * DESCRIPTION: 
 * 
 *		Implemented a SORTED linked set of unsigned integers 
 *
 */

#ifndef __LINK_SET_H__
#define __LINK_SET_H__

#include "stdinc.h"

#define UNDEF 0xFFFFFFFF

class link_set
{

	unsigned int data;	// value

	link_set *next;		// pointer to the next element in the set

public:

	/*
	 * Instantiate a linked set with the element item
	 * If item is not specified, an empty set is instantiated
	 */
	link_set(unsigned int=UNDEF);

	/*
	 * (Recursive) de-allocator
	 */
	~link_set();

	/*
	 * Return ture if the set is empty
	 */
	bool empty();

	/*
	 * Return ture if the set is equal to the given set (contain the same elements)
	 */
	bool equal(link_set *);

	/*
	 * Insert into the set an given item
	 */
	void insert(unsigned int);

	/*
	 * Insert into the set all the items from the given set
	 */
	void add(link_set *);

	/*
	 * Return the value associated with the current element of the set
	 */
	unsigned int value();

	/*
	 * Return the next element in the set
	 */
	link_set *succ();

	/*
	 * Remove all elements from the set
	 */
	void clear();

	/*
	 * Return ture if the item is in the set
	 */
	bool member(unsigned int);

	/*
	 * Return the size of the set
	 */
	unsigned size();

};

inline unsigned int link_set::value() { return data; }    

inline link_set *link_set::succ() { return next; }

#endif /*__LINK_SET_H__*/
