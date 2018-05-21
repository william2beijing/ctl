/* list Library Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ls.h"

/* -------------------------------- define ----------------------------------- */

#define LS_FREE(_p) \
	do { if(_p) { free(_p); _p = NULL; } } while(0)

/* -------------------------------- private ---------------------------------- */

static void _ls_init(lsHandle *ls, lsType *type);
static void _ls_reset(lsHandle *ls);
static void _ls_clear(lsHandle *ls);

/* -------------------------------- private implementation ------------------- */

static void _ls_init(lsHandle *ls, lsType *type) {
	_ls_reset(ls);
	ls->type = type;
}

static void _ls_reset(lsHandle *ls) {
	ls->head = ls->tail = NULL;
	ls->len = 0;
}

static void _ls_clear(lsHandle *ls) {
	lsNode *node, *next;

	node = ls->head;
	while( ls->len-- ) {
		next = node->next;
		if( ls->type->free )
			ls->type->free(node->value);
		LS_FREE(node);
		node = next;
	}
	_ls_reset(ls);
}

/* -------------------------------- api implementation ----------------------- */

lsHandle *ls_create(lsType *type) {
	lsHandle *ls = malloc(sizeof(*ls));
	if( !ls )
		return NULL;
	_ls_init(ls,type);
	return ls;
}

void ls_destroy(lsHandle *ls) {
	_ls_clear(ls);
	LS_FREE(ls);
}

int ls_add_head(lsHandle *ls, void *val) {
	lsNode *node = malloc(sizeof(*node));
	if( !node )
		return LS_ERR;
	node->value = val;
	if( 0 == ls->len ) {
		ls->head = ls->tail = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = NULL;
		node->next = ls->head;
		ls->head->prev = node;
		ls->head = node;
	}
	ls->len++;
	return LS_OK;
}

int ls_add_tail(lsHandle *ls, void *val) {
	lsNode *node = malloc(sizeof(*node));
	if( !node )
		return LS_ERR;
	node->value = val;
	if( 0 == ls->len ) {
		ls->head = ls->tail = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = ls->tail;
		node->next = NULL;
		ls->tail->next = node;
		ls->tail = node;
	}
	ls->len++;
	return LS_OK;
}

int ls_insert(lsHandle *ls, lsNode *old_node, void *val, int after) {
	lsNode *node = malloc(sizeof(*node));
	if( !node )
		return LS_ERR;
	node->value = val;
	if( after ) {
		node->prev = old_node;
		node->next = old_node->next;
		if( ls->tail == old_node )
			ls->tail = node;
	} else {
		node->next = old_node;
		node->prev = old_node->prev;
		if( ls->head == old_node )
			ls->head = node;
	}
	if( NULL != node->prev )
		node->prev->next = node;
	if( NULL != node->next )
		node->next->prev = node;
	ls->len++;
	return LS_OK;
}

lsNode *ls_index(lsHandle *ls, int index) {
	lsNode *node;

	if( 0 > index ) {
		index = (-index)-1;
		node = ls->tail;
		while( index-- && node )
			node = node->prev;
	} else {
		node = ls->head;
		while( index-- && node )
			node = node->next;
	}
	return node;
}

void ls_delete(lsHandle *ls, lsNode *node) {
	if( node->prev )
		node->prev->next = node->next;
	else
		ls->head = node->next;
	if( node->next )
		node->next->prev = node->prev;
	else
		ls->tail = node->prev;
	if( ls->type->free )
		ls->type->free(node->value);
	ls->len--;
	LS_FREE(node);
}

void ls_clear(lsHandle *ls) {
	_ls_clear(ls);
}

lsIterator *ls_create_iterator(lsHandle *ls, int direction) {
	lsIterator *iter = malloc(sizeof(*iter));
	if( !iter )
		return NULL;
	iter->ls = ls;
	if( LS_START_HEAD == direction )
		iter->next = ls->head;
	else
		iter->next = ls->tail;
	iter->direction = direction;
	return iter;
}

void ls_destroy_iterator(lsIterator *iter) {
	LS_FREE(iter);
}

void ls_rewind(lsIterator *iter, int direction) {
	if( LS_START_HEAD == direction )
		iter->next = iter->ls->head;
	else
		iter->next = iter->ls->tail;
	iter->direction = direction;
}

lsNode *ls_next(lsIterator *iter) {
	lsNode *node = iter->next;
	if( !node )
		return NULL;
	if( LS_START_HEAD == iter->direction )
		iter->next = node->next;
	else
		iter->next = node->prev;
	return node;
}
