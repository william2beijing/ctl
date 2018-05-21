/* Json Library Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ht.h"
#include "ls.h"

#include "js.h"

/* -------------------------------- struct ----------------------------------- */

typedef struct jsObject {
	int type;
	int idouble;
	union {
		void *val;
		long l64;
		double d64;
	} v;
	char *name;
	int (*print)(struct jsObject *obj, char **buf);
} jsObject;

typedef struct jsBuffer {
	int len;
	int free;
	char buf[];
} jsBuffer;

/* -------------------------------- private ---------------------------------- */

#define JS_BUFFER_SIZE_INIT 4

#define JS_OBJECT_FREE(_p) \
	do { if(_p) { _js_object_free(_p); _p = NULL; } } while(0)
#define JS_BUFER_FREE(_p) \
	do { if(_p) { _js_buffer_free(_p); _p = NULL; } } while(0)
#define JS_TABLE_FREE(_p) \
	do { if(_p) { ht_destroy(_p); _p = NULL; } } while(0)
#define JS_LIST_FREE(_p) \
	do { if(_p) { ls_destroy(_p); _p = NULL; } } while(0)
#define JS_FREE(_p) \
	do { if(_p) { free(_p); _p = NULL; } } while(0)

#define JS_MAX(_a, _b) \
	(_a) > (_b) ? (_a) : (_b)

/* -------------------------------- private ---------------------------------- */

// hash functions
static unsigned int _js_hash_function(const void *key);
static int _js_hash_key_compare(const void *key1, const void *key2);
static void _js_hash_key_free(void *key);
static void _js_hash_val_free(void *val);

static htType htTypeJson = {
	_js_hash_function,
	NULL,
	NULL,
	_js_hash_key_compare,
	_js_hash_key_free,
	_js_hash_val_free
};

// list functions
static void _js_list_free(void *ptr);

static lsType lsTypeJson = {
	_js_list_free
};

// json buffer
static char *_js_buffer_new(int size);
static char *_js_buffer_newlen(int size);
static char *_js_buffer_renew(char *p, int size);
static void _js_buffer_free(char *p);
static int _js_buffer_len(char *p);
static char *_js_buffer_cat(char *p, const char *buf, int len);
static char *_js_buffer_vsprintf(char *p, const char *fmt, va_list va);
static int _js_buffer_sprintf(char **p, const char *fmt, ...);

// json object
static jsObject *_js_object_new();
static void _js_object_free(jsObject *obj);

static int _js_object_add_object(jsObject *obj, jsObject *sub);

static void _js_object_set_type(jsObject *obj, int type);
static void _js_object_set_object(jsObject *obj, void *value);
static void _js_object_set_array(jsObject *obj, void *value);
static void _js_object_set_string(jsObject *obj, char *value);
static void _js_object_set_number(jsObject *obj, double value, int idouble);
static void _js_object_set_bool(jsObject *obj, int value);
static void _js_object_set_null(jsObject *obj);

static int _js_object_string_escape(char **in);
static int _js_object_object_print(jsObject *obj, char **buf);
static int _js_object_array_print(jsObject *obj, char **buf);
static int _js_object_string_print(jsObject *obj, char **buf);
static int _js_object_number_print(jsObject *obj, char **buf);
static int _js_object_boolean_print(jsObject *obj, char **buf);
static int _js_object_null_print(jsObject *obj, char **buf);

// json parse
static const char *_js_skip(const char *in);
static const char *_js_string_escape(const char *in, int *pos);
static const char *_js_parse_value(jsObject *obj, const char *value, const char **pos);
static const char *_js_parse_name(jsObject *obj, const char *value, const char **pos);
static const char *_js_parse_string(jsObject *obj, const char *value);
static const char *_js_parse_number(jsObject *obj, const char *value);
static const char *_js_parse_array(jsObject *obj, const char *value, const char **pos);
static const char *_js_parse_object(jsObject *obj, const char *value, const char **pos);

// json functions
static void _js_init(jsHandle *js);
static void _js_reset(jsHandle *js);
static void _js_free(jsHandle *js);

static int _js_parse(jsHandle *js, const char *str);
static int _js_replace(jsObject *obj, jsObject *rep);
static char *_js_print(jsObject *obj, int *len);

/* -------------------------------- private implementation ------------------- */

// hash functions
static unsigned int _js_hash_function(const void *key) {
	return ht_gen_hash_function(key,strlen((const char *)key));
}

static int _js_hash_key_compare(const void *key1, const void *key2) {
	int len1 = _js_buffer_len((char *)key1);
	int len2 = _js_buffer_len((char *)key2);
	if( len1 != len2 )
		return HT_ERR;
	return 0 == memcmp((const char *)key1,(const char *)key2,len1);
}

static void _js_hash_key_free(void *key) {
	JS_BUFER_FREE(key);
}

static void _js_hash_val_free(void *val) {
	JS_OBJECT_FREE(val);
}

// list functions
static void _js_list_free(void *ptr) {
	JS_OBJECT_FREE(ptr);
}

// json buffer
static char *_js_buffer_new(int size) {
	jsBuffer *jbuf = calloc(1,sizeof(*jbuf) + size + 1);
	if( !jbuf )
		return NULL;
	jbuf->free = size;
	jbuf->len = 0;
	return jbuf->buf;
}

static char *_js_buffer_newlen(int size) {
	jsBuffer *jbuf = calloc(1,sizeof(*jbuf) + size + 1);
	if( !jbuf )
		return NULL;
	jbuf->free = 0;
	jbuf->len = size;
	return jbuf->buf;
}

static char *_js_buffer_renew(char *p, int size) {
	jsBuffer *jbuf = (jsBuffer *)(p - sizeof(*jbuf));
	if( jbuf->free < size ) {
		int buf_size = jbuf->free + jbuf->len;
		int add_size = jbuf->len + size;
		int new_size = JS_MAX(buf_size * 2,add_size + 8);
		jbuf = realloc(jbuf,sizeof(*jbuf) + new_size + 1);
		if( !jbuf )
			return NULL;
		jbuf->free = new_size - jbuf->len;
	}
	return jbuf->buf;
}

static void _js_buffer_free(char *p) {
	jsBuffer *jbuf = (jsBuffer *)(p - sizeof(*jbuf));
	JS_FREE(jbuf);
}

static int _js_buffer_len(char *p) {
	jsBuffer *jbuf = (jsBuffer *)(p - sizeof(*jbuf));
	return jbuf->len;
}

static char *_js_buffer_cat(char *p, const char *buf, int len) {
	p = _js_buffer_renew(p,len);
	if( p ) {
		jsBuffer *jbuf = (jsBuffer *)(p - sizeof(*jbuf));
		memcpy(p + jbuf->len,buf,len);
		jbuf->free -= len;
		jbuf->len += len;
		p[jbuf->len] = '\0';
	}
	return p;
}

static char *_js_buffer_vsprintf(char *p, const char *fmt, va_list va) {
	char *buf;
	char *t;
	int len;
	if( JS_INV == (len = vasprintf(&buf,fmt,va)) )
		return NULL;
	t = _js_buffer_cat(p,buf,len);
	JS_FREE(buf);
	return t;
}

static int _js_buffer_sprintf(char **p, const char *fmt, ...) {
	char *t;
	va_list va;
	va_start(va,fmt);
	if( !(t = _js_buffer_vsprintf(p[0],fmt,va)) ) {
		va_end(va);
		return JS_ERR;
	}
	va_end(va);
	p[0] = t;
	return JS_OK;
}

// json object
static jsObject *_js_object_new() {
	jsObject *obj = calloc(1,sizeof(*obj));
	if( !obj )
		return NULL;
	return obj;
}

static void _js_object_free(jsObject *obj) {
	switch( obj->type ) {
	case JS_OBJECT:
		JS_TABLE_FREE(obj->v.val);
		JS_BUFER_FREE(obj->name);
		break;
	case JS_ARRAY:
		JS_LIST_FREE(obj->v.val);
		break;
	case JS_STRING:
		JS_BUFER_FREE(obj->v.val);
	}
	JS_FREE(obj);
}

static int _js_object_add_object(jsObject *obj, jsObject *sub) {
	switch( obj->type ) {
	case JS_OBJECT:
		if( !ht_add(obj->v.val,obj->name,sub) )
			return JS_ERR;
		obj->name = NULL;
		break;
	case JS_ARRAY:
		if( !ls_add_tail(obj->v.val,sub) )
			return JS_ERR;
	}
	return JS_OK;
}

static void _js_object_set_type(jsObject *obj, int type) {
	switch( type ) {
	case JS_OBJECT:
		obj->print = _js_object_object_print;
		break;
	case JS_ARRAY:
		obj->print = _js_object_array_print;
		break;
	case JS_STRING:
		obj->print = _js_object_string_print;
		break;
	case JS_NUMBER:
		obj->print = _js_object_number_print;
		break;
	case JS_BOOLEAN:
		obj->print = _js_object_boolean_print;
		break;
	case JS_NULL:
		obj->print = _js_object_null_print;
	}
	obj->type = type;
}

static void _js_object_set_object(jsObject *obj, void *value) {
	_js_object_set_type(obj,JS_OBJECT);
	obj->v.val = value;
}

static void _js_object_set_array(jsObject *obj, void *value) {
	_js_object_set_type(obj,JS_ARRAY);
	obj->v.val = value;
}

static void _js_object_set_string(jsObject *obj, char *value) {
	_js_object_set_type(obj,JS_STRING);
	obj->v.val = value;
}

static void _js_object_set_number(jsObject *obj, double value, int idouble) {
	_js_object_set_type(obj,JS_NUMBER);
	if( idouble )
		obj->v.d64 = value;
	else
		obj->v.l64 = (long)value;
	obj->idouble = idouble;
}

static void _js_object_set_bool(jsObject *obj, int value) {
	_js_object_set_type(obj,JS_BOOLEAN);
	obj->v.l64 = (long)value;
}

static void _js_object_set_null(jsObject *obj) {
	_js_object_set_type(obj,JS_NULL);
}

static int _js_object_string_escape(char **in) {
	const char *val = in[0];
	char *str, *out;
	char c;
	int len = 0;

	while( (c = (val++)[0]) && ++len )
		if( strchr("\"\\\b\f\n\r\t",c) )
			len++;

	str = _js_buffer_newlen(len);
	if( !str )
		return JS_ERR;

	val = in[0];
	out = str;

	while( val[0] ) {
		if( 31 < (unsigned char)val[0] && '\"' != val[0] && '\\' != val[0] )
			(out++)[0] = (val++)[0];
		else {
			(out++)[0] = '\\';
			switch( (c = (val++)[0]) ) {
			case '\"':
				(out++)[0] = '\"';
				break;
			case '\\':
				(out++)[0] = '\\';
				break;
			case '\b':
				(out++)[0] = 'b';
				break;
			case '\f':
				(out++)[0] = 'f';
				break;
			case '\n':
				(out++)[0] = 'n';
				break;
			case '\r':
				(out++)[0] = 'r';
				break;
			case '\t':
				(out++)[0] = 't';
				break;
			default:
				(--out)[0] = '?';
				out++;
			}
		}
	}
	out[0] = '\0';

	JS_BUFER_FREE(in[0]);
	in[0] = str;
	return JS_OK;
}

static int _js_object_object_print(jsObject *obj, char **buf) {
	int n = 0;
	htEntry *he;
	htIterator *hi;

	if( !buf || !buf[0] || !obj )
	return JS_ERR;

	if( !_js_buffer_sprintf(buf,"{") )
		return JS_ERR;
	hi = ht_create_iterator(obj->v.val);
	if( !hi )
		return JS_ERR;
	while( (he = ht_next(hi)) ) {
		if( n++ )
			if( !_js_buffer_sprintf(buf,",") )
				return JS_ERR;

		if( !_js_buffer_sprintf(buf,"\"") )
			return JS_ERR;
		if( !_js_object_string_escape((char **)&ht_get_key(he)) )
			return JS_ERR;
		if( !_js_buffer_sprintf(buf,ht_get_key(he)) )
			return JS_ERR;
		if( !_js_buffer_sprintf(buf,"\":") )
			return JS_ERR;

		if( ht_get_val(he) ) {
			jsObject *obj = ht_get_val(he);
			if( !obj->print(obj,buf) )
				return JS_ERR;
		}
	}
	ht_destroy_iterator(hi);
	if( !_js_buffer_sprintf(buf,"}") )
		return JS_ERR;
	return JS_OK;
}

static int _js_object_array_print(jsObject *obj, char **buf) {
	int n = 0;
	lsNode *ln;
	lsIterator *li;

	if( !buf || !buf[0] || !obj )
		return JS_ERR;

	if( !_js_buffer_sprintf(buf,"[") )
		return JS_ERR;
	li = ls_create_iterator(obj->v.val,LS_START_HEAD);
	if( !li )
		return JS_ERR;
	while( (ln = ls_next(li)) ) {
		if( n++ )
			if( !_js_buffer_sprintf(buf,",") )
				return JS_ERR;

		if( ls_value(ln) ) {
			jsObject *obj = ls_value(ln);
			if( !obj->print(obj,buf) )
				return JS_ERR;
		}
	}
	ls_destroy_iterator(li);
	if( !_js_buffer_sprintf(buf,"]") )
		return JS_ERR;
	return JS_OK;
}

static int _js_object_string_print(jsObject *obj, char **buf) {
	if( !buf || !buf[0] || !obj )
		return JS_ERR;

	if( !_js_buffer_sprintf(buf,"\"") )
		return JS_ERR;
	if( !_js_object_string_escape((char **)&obj->v.val) )
		return JS_ERR;
	if( !_js_buffer_sprintf(buf,obj->v.val) )
		return JS_ERR;
	if( !_js_buffer_sprintf(buf,"\"") )
		return JS_ERR;
	return JS_OK;
}

static int _js_object_number_print(jsObject *obj, char **buf) {
	if( !buf || !buf[0] || !obj )
		return JS_ERR;

	if( obj->idouble ) {
		if( 1.0e-6 > fabs(obj->v.d64) || 1.0e9 < fabs(obj->v.d64) ) {
			if( !_js_buffer_sprintf(buf,"%e",obj->v.d64) )
				return JS_ERR;
		} else {
			char *dec, *end;
			char num[128]; num[0] = '\0';
			snprintf(num,127,"%lf",obj->v.d64);

			dec = strchr(num,'.');
			if( dec ) {
				end = num + strlen(num);
				while( dec < --end ) {
					if( '0' != end[0] || '.' == end[0] )
						break;
				}
				if( '.' == (end++)[0] )
					(end++)[0] = '0';
				(end)[0] = '\0';
			}
			if( !_js_buffer_sprintf(buf,"%s",num) )
				return JS_ERR;
		}
	} else {
		if( !_js_buffer_sprintf(buf,"%ld",obj->v.l64) )
			return JS_ERR;
	}
	return JS_OK;
}

static int _js_object_boolean_print(jsObject *obj, char **buf) {
	if( !buf || !buf[0] || !obj )
		return JS_ERR;

	if( obj->v.l64 ) {
		if( !_js_buffer_sprintf(buf,"true") )
			return JS_ERR;
	} else {
		if( !_js_buffer_sprintf(buf,"false") )
			return JS_ERR;
	}
	return JS_OK;
}

static int _js_object_null_print(jsObject *obj, char **buf) {
	if( !buf || !buf[0] || !obj )
		return JS_ERR;

	if( !_js_buffer_sprintf(buf,"null") )
		return JS_ERR;
	return JS_OK;
}

// json parse
static const char *_js_skip(const char *in) {
	while( in && in[0] && 32 >= ((unsigned char *)in)[0] )
		in++;
	return in;
}

static const char *_js_string_escape(const char *in, int *pos) {
	const char *val = in;
	char *out;
	char *str;
	int len = 0;

	while( '\"' != val[0] && val[0] && ++len && ++pos[0] )
		if( '\\' == (val++)[0] )
			val++, pos[0]++;

	str = _js_buffer_newlen(len);
	if( !str  )
		return NULL;

	val = in;
	out = str;

	while( '\"' != val[0] && val[0] ) {
		if( '\\' != val[0] )
			(out++)[0] = (val++)[0];
		else {
			val++;
			switch(val[0]) {
			case 'b':
				(out++)[0] = '\b';
				break;
			case 'f':
				(out++)[0] = '\f';
				break;
			case 'n':
				(out++)[0] = '\n';
				break;
			case 'r':
				(out++)[0] = '\r';
				break;
			case 't':
				(out++)[0] = '\t';
				break;
			default:
				(out++)[0] = val[0];
			}
			val++;
		}
	}
	out[0] = '\0';
	return str;
}

static const char *_js_parse_value(jsObject *obj, const char *value, const char **pos) {
	if( !value )
		return NULL;
	if( !strncmp(value,"null",4) ) {
		_js_object_set_null(obj);
		return value + 4;
	}
	if( !strncmp(value,"false",5) ) {
		_js_object_set_bool(obj,JS_FALSE);
		return value + 5;
	}
	if( !strncmp(value,"true",4) ) {
		_js_object_set_bool(obj,JS_TRUE);
		return value + 4;
	}
	if( '\"' == value[0] )
		return _js_parse_string(obj,value);
	if( '-' == value[0] || ('0' <= value[0] && '9' >= value[0]) )
		return _js_parse_number(obj,value);
	if( '[' == value[0] )
		return _js_parse_array(obj,value,pos);
	if( '{' == value[0] )
		return _js_parse_object(obj,value,pos);
	pos[0] = value;
	return NULL;
}

static const char *_js_parse_name(jsObject *obj, const char *value, const char **pos) {
	const char *val = value + 1;
	int len = 0;

	if( '\"' != value[0] ) {
		pos[0] = value;
		return NULL;
	}
	obj->name = (char *)_js_string_escape(val,&len);
	if( !obj->name )
		return NULL;
	val += len;
	if( '\"' == val[0] )
		val++;
	return val;
}

static const char *_js_parse_string(jsObject *obj, const char *value) {
	const char *val = value + 1;
	char *str;
	int len = 0;

	str = (char *)_js_string_escape(val,&len);
	if( !str )
		return NULL;
	val += len;
	if( '\"' == val[0] )
		val++;
	_js_object_set_string(obj,str);
	return val;
}

static const char *_js_parse_number(jsObject *obj, const char *value) {
	double num = 0, sign = 1, scale = 0;
	int subscale = 0, signsubscale = 1;

	if( '-' == value[0] )
		sign = -1, value++;
	if( '0' == value[0] )
		value++;
	if( '1' <= value[0] && '9' >= value[0] )
		do {
			num = (num * 10.0) + ((value++)[0] - '0');
		} while( '0' <= value[0] && '9' >= value[0] );
	if( '.' == value[0] && '0' <= value[1] && '9' >= value[1] ) {
		value++;
		do {
			num = (num * 10.0) + ((value++)[0] -'0'), scale--;
		} while( '0' <= value[0] && '9' >= value[0] );
	}
	if( 'e' == value[0] || 'E' == value[0] ) {
		value++;
		if( '+' == value[0] )
			value++;
		else if( '-' == value[0] )
			signsubscale = -1, value++;
		while( '0' <= value[0] && '9' >= value[0] )
			subscale = (subscale * 10) + ((value++)[0] -'0');
	}
	num = sign * num * pow(10.0,(scale + subscale * signsubscale));

	if( !scale )
		_js_object_set_number(obj,num,0);
	else
		_js_object_set_number(obj,num,1);
	return value;
}

static const char *_js_parse_array(jsObject *obj, const char *value, const char **pos) {
	lsHandle *ls;
	jsObject *sub;

	ls = ls_create(&lsTypeJson);
	if( !ls )
		return NULL;
	_js_object_set_array(obj,ls);

	value = _js_skip(value + 1);
	if( ']' == value[0] )
		return value + 1;

	sub = _js_object_new();
	if( !sub )
		return NULL;

	if( !_js_object_add_object(obj,sub) ) {
		JS_OBJECT_FREE(sub);
		return NULL;
	}

	value = _js_skip(_js_parse_value(sub,_js_skip(value),pos));
	if( !value )
		return NULL;

	while( ',' == value[0] ) {
		sub = _js_object_new();
		if( !sub )
			return NULL;

		if( !_js_object_add_object(obj,sub) ) {
			JS_OBJECT_FREE(sub);
			return NULL;
		}

		value = _js_skip(_js_parse_value(sub,_js_skip(value + 1),pos));
		if( !value )
			return NULL;
	}
	if( ']' == value[0] )
		return value + 1;
	pos[0] = value;
	return NULL;
}

static const char *_js_parse_object(jsObject *obj, const char *value, const char **pos) {
	htHandle *ht;
	jsObject *sub;

	ht = ht_create(&htTypeJson);
	if( !ht )
		return NULL;
	_js_object_set_object(obj,ht);

	value = _js_skip(value + 1);
	if( '}' == value[0] )
		return value + 1;

	sub = _js_object_new();
	if( !sub )
		return NULL;

	value = _js_skip(_js_parse_name(obj,_js_skip(value),pos));
	if( !value ) {
		JS_OBJECT_FREE(sub);
		return NULL;
	}

	if( !_js_object_add_object(obj,sub) ) {
		JS_OBJECT_FREE(sub);
		return NULL;
	}

	if( ':' != value[0] ) {
		pos[0] = value;
		return NULL;
	}

	value = _js_skip(_js_parse_value(sub,_js_skip(value + 1),pos));
	if( !value )
		return NULL;

	while( ',' == value[0] ) {
		sub = _js_object_new();
		if( !sub )
			return NULL;

		value = _js_skip(_js_parse_name(obj,_js_skip(value + 1),pos));
		if( !value ) {
			JS_OBJECT_FREE(sub);
			return NULL;
		}

		if( !_js_object_add_object(obj,sub) ) {
			JS_OBJECT_FREE(sub);
			return NULL;
		}

		if( ':' != value[0] ) {
			pos[0] = value;
			return NULL;
		}

		value = _js_skip(_js_parse_value(sub,_js_skip(value + 1),pos));
		if( !value )
			return NULL;
	}
	if( '}' == value[0] )
		return value + 1;
	pos[0] = value;
	return NULL;
}

// json functions
static void _js_init(jsHandle *js) {
	_js_reset(js);
}

static void _js_reset(jsHandle *js) {
	js->err = NULL;
	js->obj = NULL;
}

static void _js_free(jsHandle *js) {
	JS_OBJECT_FREE(js->obj);
	_js_reset(js);
}

static int _js_parse(jsHandle *js, const char *str) {
	jsObject *obj = _js_object_new();
	if( obj ) {
		const char *end = _js_parse_value(obj,_js_skip(str),(const char **)&js->err);
		if( end && JS_OBJECT == obj->type ) {
			js->obj = obj;
			return JS_TRUE;
		}
		JS_OBJECT_FREE(obj);
	}
	return JS_ERR;
}

static int _js_replace(jsObject *obj, jsObject *rep) {
	htEntry *rhe, *he;
	htIterator *rhi;

	if( !obj || !rep )
		return JS_ERR;

	rhi = ht_create_iterator(rep->v.val);
	if( !rhi )
		return JS_ERR;
	while( (rhe = ht_next(rhi)) ) {
		he = ht_find(obj->v.val,ht_get_key(rhe));
		if( he )
			ht_delete(obj->v.val,he);
		ht_add(obj->v.val,ht_get_key(rhe),ht_get_val(rhe));
		ht_get_key(rhe) = NULL;
		ht_get_val(rhe) = NULL;
	}
	ht_destroy_iterator(rhi);
	return JS_OK;
}

static char *_js_print(jsObject *obj, int *len) {
	char *buf;

	if( !obj )
		return NULL;

	buf = _js_buffer_new(JS_BUFFER_SIZE_INIT);
	if( !buf )
		return NULL;
	if( !obj->print(obj,&buf) ) {
		JS_BUFER_FREE(buf);
		return NULL;
	}
	len[0] = _js_buffer_len(buf);
	return buf;
}

/* -------------------------------- api implementation ----------------------- */

jsHandle *js_create(void) {
	jsHandle *js = malloc(sizeof(*js));
	if( !js )
		return NULL;
	_js_init(js);
	return js;
}

void js_destroy(jsHandle *js) {
	_js_free(js);
	JS_FREE(js);
}

void js_free(jsHandle *js) {
	_js_free(js);
}

int js_parse(jsHandle *js, const char *str) {
	return _js_parse(js,str);
}

int js_replace(void *obj, void *rep) {
	return _js_replace(obj,rep);
}

char *js_print(void *obj, int *len) {
	return _js_print(obj,len);
}

void js_free_string(char *ptr) {
	JS_BUFER_FREE(ptr);
}
