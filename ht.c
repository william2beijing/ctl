/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 */

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ht.h"

/* -------------------------------- define ----------------------------------- */

#define HT_FREE(_p) \
	do { if(_p) { free(_p); _p = NULL; } } while(0)

/* -------------------------------- private ---------------------------------- */

static unsigned int ht_hash_function_seed = 5381;

static void _ht_init(htHandle *ht, htType *type);
static void _ht_reset(htHandle *ht);
static int _ht_key_index(htHandle *ht, const void *key);
static int _ht_expand_if_needed(htHandle *ht);
static int _ht_expand(htHandle *ht, unsigned int size);
static unsigned int _ht_next_power(unsigned int size);
static int _ht_clear(htHandle *ht);
static void _ht_rehash(htHandle *ht, htHandle *_ht);

/* -------------------------------- hash functions --------------------------- */

unsigned int ht_int_hash_function(unsigned int key) {
	key += ~(key << 15);
	key ^=  (key >> 10);
	key +=  (key << 3);
	key ^=  (key >> 6);
	key += ~(key << 11);
	key ^=  (key >> 16);
	return key;
}

unsigned int ht_gen_hash_function(const void *key, int len) {
	unsigned int seed = ht_hash_function_seed;
	unsigned int h = seed ^ len;
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	const unsigned char *data = (const unsigned char *)key;

	while( 4 <= len ) {
		unsigned int k = *(unsigned int *)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	switch( len ) {
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0]; h *= m;
	};

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return (unsigned int)h;
}

/* -------------------------------- private implementation ------------------- */

static void _ht_init(htHandle *ht, htType *type) {
	_ht_reset(ht);
	ht->type = type;
}

static void _ht_reset(htHandle *ht) {
	ht->type = NULL;
	ht->table = NULL;
	ht->size = 0;
	ht->mask = 0;
	ht->used = 0;
}

static int _ht_key_index(htHandle *ht, const void *key) {
	unsigned int hash, index;
	htEntry *he;

	if( !_ht_expand_if_needed(ht) )
		return HT_INV;

	hash = ht_hash_key(ht,key);
	index = hash & ht->mask;
	he = ht->table[index];
	while( he ) {
		if( ht_compare_keys(ht,key,he->key) )
			return HT_INV;
		he = he->next;
	}
	return index;
}

static int _ht_expand_if_needed(htHandle *ht) {
	if( 0 == ht->size )
		return _ht_expand(ht,HT_INITIAL_SIZE);
	if( ht->size <= ht->used )
	    return _ht_expand(ht,ht->used * 2);
	return HT_OK;
}

static int _ht_expand(htHandle *ht, unsigned int size) {
	htHandle _ht;
	unsigned int realsize = _ht_next_power(size);

	if( size < ht->used )
		return HT_ERR;

	if( realsize == ht->size )
		return HT_ERR;

	_ht_init(&_ht,ht->type);
	_ht.table = calloc(1,sizeof(htEntry *) * realsize);
	if( !_ht.table )
		return HT_ERR;
	_ht.size = realsize;
	_ht.mask = realsize - 1;

	if( !ht->table )
		ht[0] = _ht;
	else
		_ht_rehash(ht,&_ht);
	return HT_OK;
}

static unsigned int _ht_next_power(unsigned int size) {
	unsigned int i = HT_INITIAL_SIZE;
	if( INT_MAX <= size )
		return INT_MAX;
	while( 1 ) {
		if( size <= i )
			return i;
		i *= 2;
	}
}

static int _ht_clear(htHandle *ht) {
	unsigned int index = 0;

	while( 0 != ht->used ) {
		htEntry *next;
		htEntry *he = ht->table[index];
		while( he ) {
			next = he->next;

			ht_free_key(ht,he);
			ht_free_val(ht,he);
			HT_FREE(he);

			ht->used--;
			he = next;
		}
		index++;
	}
	HT_FREE(ht->table);
	_ht_reset(ht);
	return HT_OK;
}

static void _ht_rehash(htHandle *ht, htHandle *_ht) {
	unsigned int index = 0;

	while( 0 != ht->used ) {
		htEntry *next;
		htEntry *he = ht->table[index];
		while( he ) {
			unsigned int _index;

			next = he->next;

			/* rehash */
			_index = ht_hash_key(_ht,he->key) & _ht->mask;
			if( !_ht->table[_index] ) {
				_ht->table[_index] = he;
				he->prev = he->next = NULL;
			} else {
				he->prev = NULL;
				he->next = _ht->table[_index];
				_ht->table[_index]->prev = he;
				_ht->table[_index] = he;
			}
			_ht->used++;

			ht->used--;
			he = next;
		}
		index++;
	}

	if( 0 == ht->used ) {
		HT_FREE(ht->table);
		ht[0] = _ht[0];
	}
}

/* -------------------------------- api implementation ----------------------- */

htHandle *ht_create(htType *type) {
	htHandle *ht = malloc(sizeof(*ht));
	if( !ht )
		return NULL;
	_ht_init(ht,type);
	return ht;
}

void ht_destroy(htHandle *ht) {
	_ht_clear(ht);
	HT_FREE(ht);
}

int ht_add(htHandle *ht, void *key, void *val) {
	htEntry *he = ht_add_raw(ht,key);
	if( !he )
		return HT_ERR;
	ht_set_val(ht,he,val);
	return HT_OK;
}

htEntry *ht_add_raw(htHandle *ht, void *key) {
	int index;
	htEntry *he;

	if( HT_INV == (index = _ht_key_index(ht,key)) )
		return NULL;

	he = calloc(1,sizeof(*he));
	if( !he )
		return NULL;
	if( !ht->table[index] ) {
		ht->table[index] = he;
		he->prev = he->next = NULL;
	} else {
		he->prev = NULL;
		he->next = ht->table[index];
		ht->table[index]->prev = he;
		ht->table[index] = he;
	}
	ht->used++;

	ht_set_key(ht,he,key);
	return he;
}

htEntry *ht_put_raw(htHandle *ht, void *key) {
	htEntry *he = ht_find(ht,key);
	return he ? he : ht_add_raw(ht,key);
}

void ht_delete(htHandle *ht, htEntry *he) {
	unsigned int hash, index;

	if( 0 == ht->size )
		return;

	hash = ht_hash_key(ht,he->key);
	index = hash & ht->mask;

	if( he->prev )
		he->prev->next = he->next;
	else
		ht->table[index] = he->next;
	if( he->next )
		he->next->prev = he->prev;

	ht_free_key(ht,he);
	ht_free_val(ht,he);
	HT_FREE(he);
	ht->used--;
}

void ht_clear(htHandle *ht) {
	_ht_clear(ht);
}

htEntry *ht_find(htHandle *ht, const void *key) {
	unsigned int hash, index;
	htEntry *he;

	if( 0 == ht->size )
		return NULL;

	hash = ht_hash_key(ht,key);
	index = hash & ht->mask;
	he = ht->table[index];
	while( he ) {
		if( ht_compare_keys(ht,key,he->key) )
			return he;
		he = he->next;
	}
	return NULL;
}

htEntry *ht_random(htHandle *ht) {
	htEntry *he, *backup;
	unsigned int index, len;

	if( 0 == ht->used )
		return NULL;

	do {
		index = random() & ht->mask;
		he = ht->table[index];
	} while( !he );

	len = 0;
	backup = he;
	while( he ) {
		he = he->next;
		len++;
	}

	len = random() % len;
	he = backup;
	while( len-- )
		he = he->next;
	return he;
}

int ht_resize(htHandle *ht) {
	int minimal = ht->used;
	if( HT_INITIAL_SIZE > minimal )
		minimal = HT_INITIAL_SIZE;
	return _ht_expand(ht,minimal);
}

htIterator *ht_create_iterator(htHandle *ht) {
	htIterator *iter = malloc(sizeof(*iter));
	if( !iter )
		return NULL;
	iter->ht = ht;
	if( 0 < ht->size )
		iter->next = ht->table[0];
	else
		iter->next = NULL;
	iter->index = 0;
	return iter;
}

void ht_destroy_iterator(htIterator *iter) {
	HT_FREE(iter);
}

htEntry *ht_next(htIterator *iter) {
	htEntry *he = iter->next;

	while( 1 ) {
		if( !he ) {
			htHandle *ht = iter->ht;
			iter->index++;
			if( iter->index >= ht->size )
				break;
			he = ht->table[iter->index];
		}
		if( he ) {
			iter->next = he->next;
			return he;
		}
	}
	return NULL;
}

/* -------------------------------- debugging -------------------------------- */

#define HT_STATS_VECTLEN 50

void ht_status(htHandle *ht) {
	unsigned int i, slots = 0, chainlen, maxchainlen = 0;
	unsigned int totchainlen = 0;
	unsigned int clvector[HT_STATS_VECTLEN];

	if( 0 == ht->used ) {
		printf("No stats available for empty dictionaries\n");
		return;
	}

	for( i = 0; HT_STATS_VECTLEN > i; ++i )
		clvector[i] = 0;

	for( i = 0; ht->size > i; ++i ) {
		htEntry *he;

		if( !ht->table[i] ) {
			clvector[0]++;
			continue;
		}
		slots++;

		chainlen = 0;
		he = ht->table[i];
		while( he ) {
			chainlen++;
			he = he->next;
		}
		clvector[(HT_STATS_VECTLEN > chainlen) ? chainlen : (HT_STATS_VECTLEN - 1)]++;
		if( maxchainlen < chainlen )
			maxchainlen = chainlen;
		totchainlen += chainlen;
	}
	printf("Hash table stats:\n");
	printf(" table size: %d\n",ht->size);
	printf(" number of elements: %d\n",ht->used);
	printf(" different slots: %d\n",slots);
	printf(" max chain length: %d\n",maxchainlen);
	printf(" avg chain length (counted): %.02f\n",(float)totchainlen / slots);
	printf(" avg chain length (computed): %.02f\n",(float)ht->used / slots);
	printf(" Chain length distribution:\n");
	for( i = 0; HT_STATS_VECTLEN - 1 > i; ++i ) {
		if( 0 == clvector[i] )
			continue;
		printf("   %s%d: %d (%.02f%%)\n",(HT_STATS_VECTLEN - 1 == i) ? ">= " : "",i,clvector[i],((float)clvector[i] / ht->size) * 100);
	}
}
