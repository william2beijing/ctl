/* list Library Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#ifndef __LS_H_
#define __LS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------- struct ----------------------------------- */

typedef struct lsType {
	void (*free)(void *ptr);
} lsType;

typedef struct lsNode {
	struct lsNode *prev;
	struct lsNode *next;
	void *value;
} lsNode;

typedef struct lsHandle {
	lsNode *head;
	lsNode *tail;
	lsType *type;
	unsigned int len;
} lsHandle;

typedef struct lsIterator {
	lsHandle *ls;
	lsNode *next;
	int direction;
} lsIterator;

/* -------------------------------- define ----------------------------------- */

#define LS_OK 1
#define LS_ERR 0

#define LS_START_HEAD 0
#define LS_START_TAIL 1

#define ls_value(_n) ((_n)->value)
#define ls_prev_node(_n) ((_n)->prev)
#define ls_next_node(_n) ((_n)->next)

#define ls_length(_l) ((_l)->len)

/* -------------------------------- api functions ---------------------------- */

lsHandle *ls_create(lsType *type);
void ls_destroy(lsHandle *ls);
int ls_add_head(lsHandle *ls, void *val);
int ls_add_tail(lsHandle *ls, void *val);
int ls_insert(lsHandle *ls, lsNode *old_node, void *val, int after);
lsNode *ls_index(lsHandle *ls, int index);
void ls_delete(lsHandle *ls, lsNode *node);
void ls_clear(lsHandle *ls);
lsIterator *ls_create_iterator(lsHandle *ls, int direction);
void ls_destroy_iterator(lsIterator *iter);
void ls_rewind(lsIterator *iter, int direction);
lsNode *ls_next(lsIterator *iter);

#ifdef __cplusplus
}
#endif

#endif /* __LS_H_ */
