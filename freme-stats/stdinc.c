#include "stdinc.h"

void *allocate_array(int size, size_t item_size)
{
	void *array = malloc(size*item_size);

	if (!array)
	{
		error("allocate_array():: memory allocation failed");
	}

	return array;
}

void *reallocate_array(void *array, int size, size_t item_size)
{
	void *new_array = realloc(array, size*item_size);

	if (!new_array)
	{
		error("reallocate_array():: memory reallocation failed");
	}

	return new_array;
}
