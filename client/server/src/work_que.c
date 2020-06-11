#include "../include/factory.h"

void queInit(pQue_t pq, int capacity)
{
    bzero(pq,sizeof(Que_t));
    pq->capacity=capacity;
    pq->size=0;
    pq->queHead=NULL;
    pq->queTail=NULL;
    pthread_mutex_init(&pq->mutex,NULL);
}
void queInsert(pQue_t pq, pNode_t pnew)
{
    if(pq->size==0)
    {
        pq->queHead=pq->queTail=pnew;
    }
    pq->queTail->pNext=pnew;
    pq->queTail=pnew;
    pq->size++;
}
int queGet(pQue_t pq, pNode_t *p)
{
    if(pq->size==0)
    {
        *p=NULL;
        return -1;
    }
    *p=pq->queHead;
    pq->queHead=pq->queHead->pNext;
    if(pq->queHead==NULL)
    {
        pq->queTail=NULL;
    }
    pq->size--;
    return 0;
}
void queDestroy(pQue_t pq)
{
    pNode_t pCurDel = pq->queHead;
    pNode_t pNextDel;
    while(pCurDel != NULL){
        pNextDel = pCurDel->pNext;
        free(pCurDel);
        pCurDel = pNextDel;
    }
    pq->queHead = NULL;
    pq->queTail = NULL;
    pthread_mutex_destroy(&pq->mutex);
}