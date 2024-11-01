#include "hash_table.h"

#include <stdio.h>
#include <stdlib.h>


void hash_table_init(hash_table* ht, unsigned int initial_length, size_t entry_size)
{
	if (!ht) return;
	
	ht->_entries_len = initial_length;
	ht->_entry_size = entry_size;
	ht->_entries = calloc(initial_length, entry_size);
	
	ht->insert = &hash_table_insert;
	ht->lookup = &hash_table_lookup;
}

void hash_table_insert(hash_table* ht, char key, char* data)
{
	if (!ht) return;

	int index = hash_table_hash(ht, key);

	index *= ht->_entry_size;

	for (int i = 0; i < ht->_entry_size; i++)
	{
		ht->_entries[index] = data[i];
		index++;
	}
}


char* hash_table_lookup(const hash_table* ht, char key)
{
	if (!ht) return;

	const int index = hash_table_hash(ht, key) * ht->_entry_size;

	return ht->_entries + index;
}


unsigned int hash_table_hash(const hash_table* ht, char key)
{
	if (!ht) return;
	if (ht->_entries_len == 0) return 0;

	unsigned char p = key;
	unsigned h = 0x811c9dc5;
	int i;

	for (i = 0; i < 1; i++)
		h = (h ^ p) * 0x01000193 % ht->_entries_len;

	unsigned int index = h;

	return index;
}