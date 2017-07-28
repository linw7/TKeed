//
// Latest edit by TeeKee on 2017/7/23.
//

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdlib.h>

#define TK_PQ_DEFAULT_SIZE 10

typedef int (*tk_pq_comparator_pt)(void *pi, void *pj);

typedef struct priority_queue{
    void **pq;
    size_t nalloc;
    size_t size;
    tk_pq_comparator_pt comp;
}tk_pq_t;

int tk_pq_init(tk_pq_t *tk_pq, tk_pq_comparator_pt comp, size_t size);
int tk_pq_is_empty(tk_pq_t *tk_pq);
size_t tk_pq_size(tk_pq_t *tk_pq);
void *tk_pq_min(tk_pq_t *tk_pq);
int tk_pq_delmin(tk_pq_t *tk_pq);
int tk_pq_insert(tk_pq_t *tk_pq, void *item);
int tk_pq_sink(tk_pq_t *tk_pq, size_t i);

#endif 
