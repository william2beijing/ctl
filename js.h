/* Json Library Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#ifndef __JS_H_
#define __JS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------- struct ----------------------------------- */

typedef struct jsHandle {
	void *err;
	void *obj;
} jsHandle;

/* -------------------------------- define ----------------------------------- */

#define JS_OK 1
#define JS_ERR 0
#define JS_INV -1

#define JS_FALSE 0
#define JS_TRUE 1
#define JS_NULL 2
#define JS_BOOLEAN 3
#define JS_NUMBER 4
#define JS_STRING 5
#define JS_ARRAY 6
#define JS_OBJECT 7

#define js_error(_j) ((_j)->err)
#define js_object(_j) ((_j)->obj)

/* -------------------------------- api -------------------------------------- */

jsHandle *js_create(void);
void js_destroy(jsHandle *js);
void js_free(jsHandle *js);
int js_parse(jsHandle *js, const char *str);

int js_replace(void *obj, void *rep);

char *js_print(void *obj, int *len);
void js_free_string(char *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __JS_H_ */
