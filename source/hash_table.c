#include "hash_table.h"

#include <stdio.h>
#include <stdlib.h>


void hashtable_insert(hash_table* ht, char key, int value)
{
	if (!ht) return;
}


unsigned int hashtable_hash(const hash_table* ht, char key)
{
	if (ht->values_len == 0) return 0;

	unsigned char p = key;
	unsigned h = 0x811c9dc5;
	int i;

	for (i = 0; i < 1; i++)
		h = (h ^ p) * 0x01000193;

	unsigned int index = h % ht->values_len;

	return index;
}