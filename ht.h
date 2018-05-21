/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 */

#ifndef __HT_H_
#define __HT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------- struct ----------------------------------- */

typedef struct htType {
	unsigned int (*hash_function)(const void *key);
	void *(*key_dup)(const void *key);
	void *(*val_dup)(const void *obj);
	int (*key_compare)(const void *key1, const void *key2);
	void (*key_free)(void *key);
	void (*val_free)(void *obj);
} htType;

typedef struct htEntry {
	void *key;
	union {
		void *val;
		unsigned long u64;
		long s64;
		double d64;
	} v;
	struct htEntry *prev;
	struct htEntry *next;
} htEntry;

typedef struct htHandle {
	htType *type;
	htEntry **table;
	unsigned int size;
	unsigned int mask;
	unsigned int used;
} htHandle;

typedef struct htIterator {
	htHandle *ht;
	htEntry *next;
	unsigned int index;
} htIterator;

/* -------------------------------- define ----------------------------------- */

#define HT_OK 1
#define HT_ERR 0
#define HT_INV -1

#define HT_INITIAL_SIZE 4

#define ht_set_signed_int_val(_e, _v) \
	do { _e->v.s64 = _v; } while(0)

#define ht_set_unsigned_int_val(_e, _v) \
	do { _e->v.u64 = _v; } while(0)

#define ht_set_double_val(_e, _v) \
	do { _e->v.d64 = _v; } while(0)

#define ht_free_key(_h, _e) \
	if ((_h)->type->key_free) \
		(_h)->type->key_free((_e)->key)

#define ht_set_key(_h, _e, _k) do { \
	if ((_h)->type->key_dup) \
		_e->key = (_h)->type->key_dup(_k); \
	else \
		_e->key = (_k); \
} while(0)

#define ht_compare_keys(_h, _k1, _k2) \
	(((_h)->type->key_compare) ? \
		(_h)->type->key_compare(_k1, _k2) : \
		(_k1) == (_k2))

#define ht_free_val(_h, _e) \
	if ((_h)->type->val_free) \
		(_h)->type->val_free((_e)->v.val)

#define ht_set_val(_h, _e, _v) do { \
	if ((_h)->type->val_dup) \
		_e->v.val = (_h)->type->val_dup(_v); \
	else \
		_e->v.val = (_v); \
} while(0)

#define ht_hash_key(_h, _k) (_h)->type->hash_function(_k)
#define ht_get_key(_e) ((_e)->key)
#define ht_get_val(_e) ((_e)->v.val)
#define ht_get_signed_int_val(_e) ((_e)->v.s64)
#define ht_get_unsigned_int_val(_e) ((_e)->v.u64)
#define ht_get_double_val(_e) ((_e)->v.d64)
#define ht_slots(_h) ((_h)->size)
#define ht_size(_h) ((_h)->used)

/* -------------------------------- hash functions --------------------------- */

unsigned int ht_int_hash_function(unsigned int key);
unsigned int ht_gen_hash_function(const void *key, int len);

/* -------------------------------- api functions ---------------------------- */

htHandle *ht_create(htType *type);
void ht_destroy(htHandle *ht);
int ht_add(htHandle *ht, void *key, void *val);
htEntry *ht_add_raw(htHandle *ht, void *key);
htEntry *ht_put_raw(htHandle *ht, void *key);
void ht_delete(htHandle *ht, htEntry *he);
void ht_clear(htHandle *ht);
htEntry *ht_find(htHandle *ht, const void *key);
htEntry *ht_random(htHandle *ht);
int ht_resize(htHandle *ht);
htIterator *ht_create_iterator(htHandle *ht);
void ht_destroy_iterator(htIterator *iter);
htEntry *ht_next(htIterator *iter);
void ht_status(htHandle *ht);

#ifdef __cplusplus
}
#endif

#endif /* __HT_H_ */
