/* Protocol Buffer Implementation.
 *
 * This library is free software; you can redistribute it and/or modify
 */

#ifndef __PB_H_
#define __PB_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------- api functions ---------------------------- */

char *pb_new(int size);
char *pb_new_len(char *p, int size);
char *pb_renew(char *p, int size);
void pb_free(char *p);
int pb_size(char *p);
int pb_len(char *p);
void pb_set_len(char *p, int len);
void pb_incr_len(char *p, int len);
char *pb_cat(char *p, const char *buf, int len);
int pb_cmp(char *p, const char *buf, int len);
char *pb_cpy(char *p, const char *buf, int len);
char *pb_sub(char *p, int begin, int end);
void pb_mov(char *p, int begin, int end);
char *pb_sprintf(char *p, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __PB_H_ */
