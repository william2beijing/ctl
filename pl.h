/* Memory Pool Library Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#ifndef __PL_H_
#define __PL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------- struct ----------------------------------- */

typedef struct plLarge {
	void *alloc;
	struct plLarge *prev;
	struct plLarge *next;
} plLarge;

typedef struct plHandle plHandle;

typedef struct plData {
	void *last;
	void *end;
	unsigned int failed;
	plHandle *next;
} plData;

typedef struct plHandle {
	unsigned int max;
	plData data;
	plHandle *current;
	plLarge *large;
	plLarge *free;
} plHandle;

/* -------------------------------- define ----------------------------------- */

#define PL_OK 1
#define PL_ERR 0

#define PL_PAGE_SIZE (1024 * 4)

/* -------------------------------- api functions ---------------------------- */

plHandle *pl_create(void);
void pl_destroy(plHandle *pl);
void pl_reset(plHandle *pl);

void *pl_alloc(plHandle *pl, int size);
void *pl_realloc(plHandle *pl, void *p, int size);
void *pl_calloc(plHandle *pl, int size);
void pl_free(plHandle *pl, void *p);

char *pl_strdup(plHandle *pl, const char *src, int len);
char *pl_sprintf(plHandle *pl, const char *fmt, ...);
char *pl_replace(plHandle *pl, const char *src, const char *org, const char *rep);

#ifdef __cplusplus
}
#endif

#endif /* __PL_H_ */
