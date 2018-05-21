/* Protocol Buffer Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pb.h"

/* -------------------------------- struct ----------------------------------- */

typedef struct pbHandle {
	int len;
	int free;
	char buf[];
} pbHandle;

/* -------------------------------- define ----------------------------------- */

#define PB_FREE(_p) \
	do { if(_p) { free(_p); _p = NULL; } } while(0)

#define PB_MAX(_a,_b) \
	(_a) > (_b) ? (_a) : (_b)

/* -------------------------------- static ----------------------------------- */

static void _pb_cat(char *p, const char *buf, int len);
static void _pb_cpy(char *p, const char *buf, int len);
static char *_pb_vsprintf(char *p, const char *fmt, va_list va);

/* -------------------------------- static implementation -------------------- */

static void _pb_cat(char *p, const char *buf, int len) {
	pbHandle *pb = (pbHandle *)(p - sizeof(*pb));
	memcpy(pb->buf + pb->len,buf,len);
	pb->free -= len;
	pb->len += len;
	pb->buf[pb->len] = '\0';
}

static void _pb_cpy(char *p, const char *buf, int len) {
	pbHandle *pb = (pbHandle *)(p - sizeof(*pb));
	memcpy(pb->buf,buf,len);
	pb->free = pb->free + pb->len - len;
	pb->len = len;
	pb->buf[pb->len] = '\0';
}

static char *_pb_vsprintf(char *p, const char *fmt, va_list va) {
	char *out, *t;
	int len;
	if( -1 == (len = vasprintf(&t,fmt,va)) )
		return NULL;
	out = pb_cat(p,t,len);
	PB_FREE(t);
	return out;
}

/* -------------------------------- api implementation ----------------------- */

char *pb_new(int size) {
	pbHandle *pb = calloc(1,sizeof(*pb) + size + 1);
	if( !pb )
		return NULL;
	pb->free = size;
	return pb->buf;
}

char *pb_new_len(char *p, int size) {
	pbHandle *pb = calloc(1,sizeof(*pb) + size + 1);
	if( !pb )
		return NULL;
	memcpy(pb->buf,p,size);
	pb->len = size;
	pb->buf[pb->len] = '\0';
	return pb->buf;
}

char *pb_renew(char *p, int size) {
	pbHandle *pb = (pbHandle *)(p - sizeof(*pb));
	if( pb->free < size ) {
		int buf_size = pb->free + pb->len;
		int add_size = pb->len + size;
		int new_size = PB_MAX(buf_size * 2,add_size + 8);
		pb = realloc(pb,sizeof(*pb) + new_size + 1);
		if( !pb )
			return NULL;
		pb->free = new_size - pb->len;
	}
	return pb->buf;
}

void pb_free(char *p) {
	pbHandle *pb = (pbHandle *)(p - sizeof(*pb));
	PB_FREE(pb);
}

int pb_size(char *p) {
	pbHandle *pb = (pbHandle *)(p - sizeof(*pb));
	return pb->free + pb->len;
}

int pb_len(char *p) {
	pbHandle *pb = (pbHandle *)(p - sizeof(*pb));
	return pb->len;
}

void pb_set_len(char *p, int len) {
	pbHandle *pb = (pbHandle *)(p - sizeof(*pb));
	int size = pb->free + pb->len;
	pb->free = size - len;
	pb->len = len;
	pb->buf[pb->len] = '\0';
}

void pb_incr_len(char *p, int len) {
	pbHandle *pb = (pbHandle *)(p - sizeof(*pb));
	pb->free -= len;
	pb->len += len;
	pb->buf[pb->len] = '\0';
}

char *pb_cat(char *p, const char *buf, int len) {
	if( 0 == len )
		return p;
	p = pb_renew(p,len);
	if( p ) {
		_pb_cat(p,buf,len);
	}
	return p;
}

int pb_cmp(char *p, const char *buf, int len) {
	if( 0 == len )
		return 0;
	return memcmp(p,buf,len);
}

char *pb_cpy(char *p, const char *buf, int len) {
	int diff, size;
	if( 0 == len )
		return p;
	diff = pb_size(p) - len;
	if( 0 > diff ) {
		size = pb_size(p) - pb_len(p) - diff;
		p = pb_renew(p,size);
	}
	if( p ) {
		_pb_cpy(p,buf,len);
	}
	return p;
}

char *pb_sub(char *p, int begin, int end) {
	return NULL;
}

void pb_mov(char *p, int begin, int end) {
	int new_len, len = pb_len(p);
	if( 0 == len )
		return;
	if( 0 > begin ) {
		begin += len;
		if( 0 > begin )
			begin = 0;
	}
	if( 0 > end ) {
		end += len;
		if( 0 > end )
			end = 0;
	}
	new_len = (begin > end) ? 0 : (end - begin) + 1;
	if( 0 != new_len ) {
		if( begin >= len ) {
			new_len = 0;
		} else if( end >= len ) {
			end = len - 1;
			new_len = (begin > end) ? 0 : (end - begin) + 1;
		}
	} else {
		begin = 0;
	}
	if( begin && new_len )
		memmove(p,p+begin,new_len);
	pb_set_len(p,new_len);
}

char *pb_sprintf(char *p, const char *fmt, ...) {
	char *out;
	va_list va;
	va_start(va,fmt);
	if( !(out = _pb_vsprintf(p,fmt,va)) ) {
		va_end(va);
		return NULL;
	}
	va_end(va);
	return out;
}
