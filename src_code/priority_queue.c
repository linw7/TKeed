//
// Latest edit by TeeKee on 2017/7/23.
//

#include <stdlib.h>
#include <string.h>
#include "priority_queue.h"

void exch(tk_pq_t *tk_pq, size_t i, size_t j){
    void *tmp = tk_pq->pq[i];
    tk_pq->pq[i] = tk_pq->pq[j];
    tk_pq->pq[j] = tmp;
}

void swim(tk_pq_t *tk_pq, size_t k){
    while (k > 1 && tk_pq->comp(tk_pq->pq[k], tk_pq->pq[k/2])){
        exch(tk_pq, k, k/2);
        k /= 2;
    }
}

int sink(tk_pq_t *tk_pq, size_t k){
    size_t j;
    size_t nalloc = tk_pq->nalloc;
    while((k << 1) <= nalloc){
        j = k << 1;
        if((j < nalloc) && (tk_pq->comp(tk_pq->pq[j+1], tk_pq->pq[j])))
            j++;

        if(!tk_pq->comp(tk_pq->pq[j], tk_pq->pq[k]))
            break;

        exch(tk_pq, j, k);
        k = j;
    }
    return k;
}

int tk_pq_sink(tk_pq_t *tk_pq, size_t i){
    return sink(tk_pq, i);
}

int tk_pq_init(tk_pq_t *tk_pq, tk_pq_comparator_pt comp, size_t size){
    // 为tk_pq_t节点的pq分配(void *)指针
    tk_pq->pq = (void **)malloc(sizeof(void *) * (size + 1));
    if (!tk_pq->pq)
        return -1;

    tk_pq->nalloc = 0;
    tk_pq->size = size + 1;
    tk_pq->comp = comp;
    return 0;
}

int tk_pq_is_empty(tk_pq_t *tk_pq){
    // 通过nalloc值款快速判断是否为空
    return (tk_pq->nalloc == 0) ? 1 : 0;
}

size_t tk_pq_size(tk_pq_t *tk_pq){
    // 获取优先队列大小
    return tk_pq->nalloc;
}

void *tk_pq_min(tk_pq_t *tk_pq){
    // 优先队列最小值直接返回第一个元素（指针）
    if (tk_pq_is_empty(tk_pq))
        return (void *)(-1);

    return tk_pq->pq[1];
}


int resize(tk_pq_t *tk_pq, size_t new_size){
    if(new_size <= tk_pq->nalloc)
        return -1;

    void **new_ptr = (void **)malloc(sizeof(void *) * new_size);
    if(!new_ptr)
        return -1;
    // 将原本nalloc + 1个元素值拷贝到new_ptr指向的位置
    memcpy(new_ptr, tk_pq->pq, sizeof(void *) * (tk_pq->nalloc + 1));
    // 释放旧元素
    free(tk_pq->pq);
    // 重新改写优先队列元素pq指针为new_ptr
    tk_pq->pq = new_ptr;
    tk_pq->size = new_size;
    return 0;
}

int tk_pq_delmin(tk_pq_t *tk_pq){
    if(tk_pq_is_empty(tk_pq))
        return 0;

    exch(tk_pq, 1, tk_pq->nalloc);
    --tk_pq->nalloc;
    sink(tk_pq, 1);
    if((tk_pq->nalloc > 0) && (tk_pq->nalloc <= (tk_pq->size - 1)/4)){
        if(resize(tk_pq, tk_pq->size / 2) < 0)
            return -1;
    }
    return 0;
}

int tk_pq_insert(tk_pq_t *tk_pq, void *item){
    if(tk_pq->nalloc + 1 == tk_pq->size){
        if(resize(tk_pq, tk_pq->size * 2) < 0){
            return -1;
        }
    }
    tk_pq->pq[++tk_pq->nalloc] = item;
    swim(tk_pq, tk_pq->nalloc);
    return 0;
}



