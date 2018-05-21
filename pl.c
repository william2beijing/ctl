/* Memory Pool Library Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pl.h"

/* -------------------------------- define ----------------------------------- */

#define PL_ARG_SIZE sizeof(unsigned int) * 4
#define PL_END_SIZE sizeof(unsigned int)
#define PL_OFF_SIZE sizeof(unsigned int)

#define PL_FREE(_p) \
	do { if(_p) { free(_p); _p = NULL; } } while(0)

/* -------------------------------- private ---------------------------------- */

static void _pl_init(plHandle *pl);

static void *_pl_alloc_block(plHandle *pl, int size);
static void *_pl_alloc_large(plHandle *pl, int size);

/* -------------------------------- private implementation ------------------- */

static void _pl_init(plHandle *pl) {
	pl->max = PL_PAGE_SIZE;

	pl->data.last = (void *)(pl) + sizeof(*pl);
	pl->data.end = pl->data.last + PL_PAGE_SIZE;
	pl->data.failed = 0;
	pl->data.next = NULL;

	pl->current = pl;
	pl->large = NULL;
	pl->free = NULL;
}

static void *_pl_alloc_block(plHandle *pl, int size) {
	plHandle *p, *n;
	void *m;
	unsigned int psize = 0;

	psize = pl->data.end - (void *)(pl);
	n = malloc(psize);
	if( !n )
		return NULL;

	n->data.end = (void *)(n) + psize;
	n->data.next = NULL;
	n->data.failed = 0;

	m = (void *)(n) + sizeof(*pl);
	((unsigned int *)m)[0] = size;
	m += PL_OFF_SIZE;
	n->data.last = m + size;

	for( p = pl->current; p->data.next; p = p->data.next ) {
		if( 4 < p->data.failed++ )
			pl->current = p->data.next;
	}
	p->data.next = n;
	return m;
}

static void *_pl_alloc_large(plHandle *pl, int size) {
	plLarge *l;
	void *p;

	p = malloc(PL_OFF_SIZE + size);
	if( !p )
		return NULL;

	((unsigned int *)p)[0] = size;

	l = pl->free;
	if( l ) {
		if( l->prev )
			l->prev->next = l->next;
		else
			pl->free = l->next;
		if( l->next )
			l->next->prev = l->prev;

		l->alloc = p;
		l->prev = NULL;
		l->next = pl->large;
		if( pl->large )
			pl->large->prev = l;
		pl->large = l;
		return p + PL_OFF_SIZE;
	}

	l = pl_alloc(pl,sizeof(*l));
	if( !l ) {
		PL_FREE(p);
		return NULL;
	}
	l->alloc = p;
	l->prev = NULL;
	l->next = pl->large;
	if( pl->large )
		pl->large->prev = l;
	pl->large = l;
	return p + PL_OFF_SIZE;
}

/* -------------------------------- api implementation ----------------------- */

plHandle *pl_create(void) {
	plHandle *pl = malloc(sizeof(*pl) + PL_PAGE_SIZE);
	if( !pl )
		return NULL;
	_pl_init(pl);
	return pl;
}

void pl_destroy(plHandle *pl) {
	plHandle *p, *n;
	plLarge *l;

	for( l = pl->large; l; l = l->next )
		PL_FREE(l->alloc);
	for( p = pl; p; p = n ) {
		n = p->data.next;
		PL_FREE(p);
	}
}

void pl_reset(plHandle *pl) {
	plHandle *p, *n;
	plLarge *l;

	for( l = pl->large; l; l = l->next )
		PL_FREE(l->alloc);
	for( p = pl->data.next; p; p = n ) {
		n = p->data.next;
		PL_FREE(p);
	}
	pl->data.last = (void *)(pl) + sizeof(*pl);
	pl->data.failed = 0;
	pl->data.next = NULL;
	pl->current = pl;
	pl->large = NULL;
	pl->free = NULL;
}

void *pl_alloc(plHandle *pl, int size) {
	plHandle *p = NULL;
	void *m = NULL;

	if( pl->max >= (unsigned int)PL_OFF_SIZE + size ) {
		p = pl->current;
		do {
			m = p->data.last;
			if( p->data.end - m >= (unsigned int)PL_OFF_SIZE + size ) {
				((unsigned int *)m)[0] = size;
				m += PL_OFF_SIZE;
				p->data.last = m + size;
				return m;
			}
			p = p->data.next;
		} while( p );
		return _pl_alloc_block(pl,size);
	}
	return _pl_alloc_large(pl,size);
}

void *pl_realloc(plHandle *pl, void *p, int size) {
	void *ptr = p - PL_OFF_SIZE;
	unsigned int psize = ((unsigned int *)ptr)[0];

	ptr = pl_alloc(pl,size);
	if( !ptr )
		return NULL;

	if( psize <= (unsigned int)size )
		memcpy(ptr,p,psize);
	else
		memcpy(ptr,p,size);
	return ptr;
}

void *pl_calloc(plHandle *pl, int size) {
	void *p = pl_alloc(pl,size);
	if( !p )
		return NULL;
	memset(p,0,size);
	return p;
}

void pl_free(plHandle *pl, void *p) {
	plLarge *l;
	void *ptr = p - PL_OFF_SIZE;

	if( pl->max >= ((unsigned int *)ptr)[0] )
		return;

	for( l = pl->large; l; l = l->next ) {
		if( l->alloc == ptr ) {
			if( l->prev )
				l->prev->next = l->next;
			else
				pl->large = l->next;
			if( l->next )
				l->next->prev = l->prev;

			PL_FREE(l->alloc);
			l->prev = NULL;
			l->next = pl->free;
			if( pl->free )
				pl->free->prev = l;
			pl->free = l;
			return;
		}
	}
}

char *pl_strdup(plHandle *pl, const char *src, int len) {
	char *buf = pl_alloc(pl,len+1);
	if( !buf )
		return NULL;
	memcpy(buf,src,len);
	buf[len] = '\0';
	return buf;
}

char *pl_sprintf(plHandle *pl, const char *fmt, ...) {
	char *buf = (char *)fmt;
	char *arg;
	int len = strlen(fmt) + PL_END_SIZE;

	va_list ap;
	va_start(ap,fmt);

	while( (buf = strchr(buf,'%')) ) {
		arg = va_arg(ap,char *);
		switch( buf[1] ) {
		case 's':
			len += strlen(arg);
			len += PL_END_SIZE;
			break;
		default:
			len += PL_ARG_SIZE;
			break;
		}
		buf++;
	}

	buf = pl_alloc(pl,len);
	if( !buf )
		return NULL;
	buf[0] = '\0';

	va_start(ap,fmt);
	vsnprintf(buf,len-1,fmt,ap);
	va_end(ap);
	return buf;
}

char *pl_replace(plHandle *pl, const char *src, const char *org, const char *rep) {
	char *top = (char *)src, *end;
	char *buf, *ptr;
	int srclen = strlen(src);
	int orglen = strlen(org);
	int replen = strlen(rep);
	int len = srclen + 1;

	if( !srclen || !orglen )
		return NULL;

	if( orglen < replen )
		len = replen * (srclen / orglen + 1);

	buf = pl_calloc(pl,len);
	if( !buf )
		return NULL;

	ptr = buf;
	while( '\0' != top[0] ) {
		end = strstr(top,org);
		if( !end )
			break;
		len = end - top;
		if( 0 < len ) {
			memcpy(ptr,top,len);
			ptr += len;
			top += len;
		}
		memcpy(ptr,rep,replen);
		ptr += replen;
		top += orglen;
	}
	memcpy(ptr,top,strlen(top));
	return buf;
}
