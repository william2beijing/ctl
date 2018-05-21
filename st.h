/* Stack Library Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#ifndef __ST_H_
#define __ST_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------- struct ----------------------------------- */

typedef struct stType {
	void (*free)(void *ptr);
} stType;

typedef struct stNode {
	struct stNode *prev;
	struct stNode *next;
	void *value;
} stNode;

typedef struct stHandle {
	stNode *top;
	stType *type;
	unsigned int len;
} stHandle;

typedef struct stIterator {
	stHandle *st;
	stNode *next;
} stIterator;

/* -------------------------------- define ----------------------------------- */

#define ST_OK 1
#define ST_ERR 0

#define st_value(_n) ((_n)->value)
#define st_prev_node(_n) ((_n)->prev)
#define st_next_node(_n) ((_n)->next)

#define st_free_node(_s, _n) do { \
	if((_s)->type->free) \
		(_s)->type->free((_n)->value); \
	free(_n); \
} while(0)

#define st_length(_s) ((_s)->len)

/* -------------------------------- api functions ---------------------------- */

stHandle *st_create(stType *type);
void st_destroy(stHandle *st);
int st_push(stHandle *st, void *val);
stNode *st_pop(stHandle *st);
void st_delete(stHandle *st, stNode *node);
void st_clear(stHandle *st);
stIterator *st_create_iterator(stHandle *st);
void st_destroy_iterator(stIterator *iter);
stNode *st_next(stIterator *iter);

#ifdef __cplusplus
}
#endif

#endif /* __ST_H_ */
