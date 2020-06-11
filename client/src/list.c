#include "../include/factory.h"

void addList(pLNode_t *phead,int id)
{
    pLNode_t pNew=(pLNode_t)calloc(1,sizeof(lNode_t));
    pNew->id=id;
    if(*phead==NULL)
    {
        *phead=pNew;
    }
    else
    {
        pNew->nextNode=*phead;
        *phead=pNew;
    }
}

int deleteList(pLNode_t *phead,int id)
{   
    pLNode_t pPre=*phead;
    pLNode_t pCur=*phead;
    if(pCur->id==id)
    {
        *phead=(*phead)->nextNode;
    }
    else
    {   
        while(pCur)
        {
            if(pCur->id==id)
            {
                pPre->nextNode=pCur->nextNode;
                break;
            }
            pPre=pCur;
            pCur=pCur->nextNode;
        }
        if(NULL==pCur)
		{
			return -1;
		}
    }
    free(pCur);
    pCur=NULL;
    pPre=NULL;
    return 0;
}

void freeList()
{}

