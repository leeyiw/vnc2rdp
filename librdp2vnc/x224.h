#ifndef _X224_H_
#define _X224_H_

typedef struct _r2v_x224_t {
} r2v_x224_t;

extern r2v_x224_t *r2v_x224_init(int client_fd);
extern void r2v_x224_destory(r2v_x224_t *x);

#endif
