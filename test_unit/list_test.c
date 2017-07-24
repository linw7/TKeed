#include <stdio.h>
#include <stdlib.h>
#include "list.h"

typedef struct{
    void *arg;
    list_head list;  
}st_header;

typedef struct{
    void *arg1;
    void *arg2;
    void *arg3;
    list_head list;
}st_struct;

void add_node(void *arg, list_head *head){
    st_struct *s = (st_struct *)malloc(sizeof(st_struct));
    s->arg1 = arg;
    s->arg2 = arg;
    s->arg3 = arg;
    list_add(&(s->list), head);
}

void del_node(list_head *entry){
    list_del(entry);
    st_struct *s = list_entry(entry, st_struct, list);
    free(s);
}

void display(list_head *head){
    list_head *pos;
    st_struct *et;
    size_t i = 99;

    list_for_each(pos, head) {
        et = list_entry(pos, st_struct, list);
    }
}

int main() {
    st_header st_h;
    // init list
    INIT_LIST_HEAD(&st_h.list);

    size_t i;
    // add test
    for (i = 0; i < 100; i++)
        add_node((void *)i, &(st_h.list));   

    list_head *pos;
    st_struct *et;
    i = 99;
    list_for_each(pos, &st_h.list) {
        et = list_entry(pos, st_struct, list);
        i--;
    }

    // delete test
    for (i = 0; i < 32; i++) {
        del_node(st_h.list.next);
    }

    i = 67;
    list_for_each(pos, &st_h.list) {
        et = list_entry(pos, st_struct, list);
        printf("The num = %ld\n", (size_t)et->arg1);
        i--;
    }
    return 0;
}
