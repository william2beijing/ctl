/* Stack Library Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "st.h"

/* -------------------------------- define ----------------------------------- */

#define ST_FREE(_p) \
	do { if(_p) { free(_p); _p = NULL; } } while(0)

/* -------------------------------- private ---------------------------------- */

static void _st_init(stHandle *st, stType *type);
static void _st_reset(stHandle *st);
static void _st_clear(stHandle *st);

/* -------------------------------- private implementation ------------------- */

static void _st_init(stHandle *st, stType *type) {
	_st_reset(st);
	st->type = type;
}

static void _st_reset(stHandle *st) {
	st->top = NULL;
	st->len = 0;
}

static void _st_clear(stHandle *st) {
	stNode *node, *next;

	node = st->top;
	while( st->len-- ) {
		next = node->next;
		if( st->type->free )
			st->type->free(node->value);
		ST_FREE(node);
		node = next;
	}
	_st_reset(st);
}

/* -------------------------------- api implementation ----------------------- */

stHandle *st_create(stType *type) {
	stHandle *st = malloc(sizeof(*st));
	if( !st )
		return NULL;
	_st_init(st,type);
	return st;
}

void st_destroy(stHandle *st) {
	_st_clear(st);
	ST_FREE(st);
}

int st_push(stHandle *st, void *val) {
	stNode *node = malloc(sizeof(*node));
	if( !node )
		return ST_ERR;
	node->value = val;
	if( 0 == st->len ) {
		st->top = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = NULL;
		node->next = st->top;
		st->top->prev = node;
		st->top = node;
	}
	st->len++;
	return ST_OK;
}

stNode *st_pop(stHandle *st) {
	stNode *node = st->top;
	if( !node )
		return NULL;
	if( node->prev )
		node->prev->next = node->next;
	else
		st->top = node->next;
	if( node->next )
		node->next->prev = node->prev;
	st->len--;
	return node;
}

void st_delete(stHandle *st, stNode *node) {
	if( node->prev )
		node->prev->next = node->next;
	else
		st->top = node->next;
	if( node->next )
		node->next->prev = node->prev;
	if( st->type->free )
		st->type->free(node->value);
	st->len--;
	ST_FREE(node);
}

void st_clear(stHandle *st) {
	_st_clear(st);
}

stIterator *st_create_iterator(stHandle *st) {
	stIterator *iter = malloc(sizeof(*iter));
	if( !iter )
		return NULL;
	iter->st = st;
	iter->next = st->top;
	return iter;
}

void st_destroy_iterator(stIterator *iter) {
	ST_FREE(iter);
}

stNode *st_next(stIterator *iter) {
	stNode *node = iter->next;
	if( !node )
		return NULL;
	iter->next = node->next;
	return node;
}
