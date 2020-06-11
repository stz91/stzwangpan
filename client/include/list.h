#include "../include/head.h"

typedef struct lNode{
    int id;
    struct lNode *nextNode;
}lNode_t,*pLNode_t;

typedef struct timer{
    pLNode_t phead;
}mytimer_t;

void addList(pLNode_t *phead,int id);
int deleteList(pLNode_t *phead,int id);