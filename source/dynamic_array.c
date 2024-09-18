#include <stdio.h>
#include <memory.h>
#include <malloc.h>

#include "dynamic_array.h"


void array_push(char** arr, int* arr_size, size_t element_size, char* element)
{
	if (!(*arr))
	{
		*arr = malloc(element_size);
		*arr_size = element_size;
	}
	else
	{
		(*arr_size) += element_size;
		*arr = realloc(*arr, element_size * (*arr_size));
	}

	int j = 0;
	for (int i = (*arr_size - element_size); i < (*arr_size); i++)
	{
		(*arr)[i] = element[j];
		j++;
	}
}


void array_pop(char** arr, int* arr_size, size_t element_size)
{
	if (!(*arr)) return;
	if ((*arr_size) <= 0) return;

	(*arr_size) -= element_size;
	*arr = realloc(*arr, element_size * (*arr_size));
}