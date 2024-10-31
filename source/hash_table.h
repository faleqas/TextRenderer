#pragma once


//this needs to be initialized and zeroed out before using any of the hashtable_ functions.
//if you don't initialize to zero don't cry when the program crashes
typedef struct
{
	int* values;
	int values_len;
} hash_table;



void hashtable_insert(hash_table* ht, char key, int value);
unsigned int hashtable_hash(const hash_table* ht, char key);