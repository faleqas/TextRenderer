#pragma once



typedef struct
{
	//Each key has 1 data entry
	//how many entries are in the hash table and the size of each entry
	//is decided upon creating the hash table with hash_table_init() and cannot be changed (yet)

	char* _entries;
	int _entries_len; //TODO(omar): dynamic resizing of hash table as needed
	size_t _entry_size; //in bytes.

	void (*insert)(void* ht, char key, char* data);
	char* (*lookup)(const void* ht, char key);
} hash_table;


void hash_table_init(hash_table* ht, unsigned int initial_length, size_t entry_size);

void hash_table_insert(hash_table* ht, char key, char* data);

char* hash_table_lookup(const hash_table* ht, char key);

unsigned int hash_table_hash(const hash_table* ht, char key);