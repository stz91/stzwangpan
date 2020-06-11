#include "../include/factory.h"

int epollAdd(int epfd,int fd)
{
    struct epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN;
    int ret;
    ret=epoll_ctl(epfd,EPOLL_CTL_ADD, fd, &event);
    ERROR_CHECK(ret,-1,"epoll_ctl");
    return 0;
}

int factoryInit(pfactory_t pf,int pthreadNum,int capacity)
{
    pQue_t pq=&pf->que;
    queInit(pq,capacity);
    pthread_cond_init(&pf->cond,NULL);
    pf->pthreadNum=pthreadNum;
    pf->pthId=(pthread_t*)calloc(pthreadNum,sizeof(pthread_t));
    pf->startFlag=0;
    printf("factory init success\n");
    return 0;
}
int factoryStart(factory_t* pf)
{   
    int i;
    for(i=0;i<pf->pthreadNum;i++)
    {
        pthread_create(pf->pthId+i,NULL,pthreadFunc,pf);
        // printf("pthread create success\n");
    }
    pf->startFlag=1;
    return 0;
}

void threadExitFunc(void* p)
{
    factory_t* pf=(factory_t*)p;
    pthread_mutex_unlock(&pf->que.mutex);
}

void* pthreadFunc(void* p)
{
    factory_t* pf=(factory_t*)p;
    pQue_t pq=&pf->que;
    pNode_t pNew;
    int getSuccess,ret;
    linkMsg_t lmsg;
    while (1)
    {
        pthread_mutex_lock(&pq->mutex);
        pthread_cleanup_push(threadExitFunc,pf); 
        if(!pq->size)
        {
            pthread_cond_wait(&pf->cond,&pq->mutex);
        }
        getSuccess=queGet(pq,&pNew);//通过加锁，防止当判断成功后其他线程强占，导致死锁
        pthread_cleanup_pop(1);
        if(!getSuccess)
        {
            MYSQL *conn=NULL;
            connectMYSQL(&conn);
            userState_t uState;
            bzero(&uState,0);
            while(1)
            { 
                memset(&lmsg, 0, sizeof(lmsg));
                ret = recvCycle(pNew->newFd, &lmsg, MSGHEADSIZE);//接收大小，标志，文件大小
                if(-1 == ret)
                {
                    goto ERROR_DISCONNECT;
                }
                ret = recvCycle(pNew->newFd, lmsg.buf, lmsg.size - MSGHEADSIZE);
                if(-1 == ret)
                {
                    goto ERROR_DISCONNECT;
                }
                if(USERLOGIN!=lmsg.flag&&USERENROLL!=lmsg.flag)
                {
                    //查询数据库中该用户是否有该token值
                    if(checkToken(conn,lmsg.token,30) == -1){
                        goto ERROR_DISCONNECT;
                    }else{
                        //将用户名和当前目录id赋给子进程
                        getUserNameAndCurDir(conn, lmsg.token, uState.name, uState.currentDirId);
                        printf("得到对应的用户名和目录：%s %s\n", uState.name, uState.currentDirId);
                    }
                } 
#ifdef DEBUG_SERVER
                printf("\n\n-------factory.c---threadFun------------\n");
                printf("接收到客户端请求，接着进行处理\n");
                printf("line = %d\n", __LINE__);
                printf("size = %d, flag = %d fileSize = %ld\n", lmsg.size, lmsg.flag,lmsg.fileSize);
                printf("lmsg.buf = %s\n\n", lmsg.buf);
#endif    

                char opStr[200] = {0};
                char temp[180]= {0};
                alterOPNumToStr(opStr, lmsg.flag);
                strncpy(temp, lmsg.buf, 180);//防止lmsg.buf越界
                sprintf(opStr, "%s %s", opStr, temp);
                updateOPLog(conn,uState.name,opStr);

                 switch(lmsg.flag)
                {
                case USERENROLL:
                    printf("userEnRoll recved\n"); 
                    userEnroll(pNew->newFd,conn, &lmsg);
                       //用户注册
                    break;
                case USERLOGIN:
                    printf("recv login\n");
                    userLogin(pNew->newFd, conn, &lmsg, &uState);    //用户登录
                    break;
                case GETSCOMMEND://客户端请求下载文件
                    /*发消息给客户端*/
                    transmiss(pNew->newFd, conn, &lmsg, &uState);
                    // getsDealFunc(pNew->newFd, conn, &lmsg, &uState);
                    break;
                case PUTSCOMMEND:
                    //客户端请求上传文件
                    recvFile(pNew->newFd, conn, &lmsg, &uState);
                    goto ERROR_DISCONNECT;
                    break;
                case CDCOMMEND:
                    //浏览目录，删除文件，添加文件
                    CdCommand(pNew->newFd, conn, &lmsg, &uState);
                    //printf("current dir = %s\n", uState.currentDirId);
                    break;
                case RMCOMMEND:
                    RmCommand(pNew->newFd, conn, &lmsg, &uState);
                    break;
                case LSCOMMEND:
                    LsCommand(pNew->newFd, conn,&uState);
                    break;
                case PWDCOMMEND:
                    PwdCommand(pNew->newFd, conn, &uState);
                    break;
                case MKDIRCOMMEND:
                    printf("mkdir recv\n");
                    MkdirCommand(pNew->newFd,conn,&lmsg,&uState);
                    break;
                default:
                    break;
                }//switch(lmsg.flag) 
            }
ERROR_DISCONNECT:
            close(pNew->newFd);
            free(pNew);
            mysql_close(conn);
            printf("user disconnect\n");
        }
    }
    

}

void factoryDestroy(pfactory_t pf)
{
    queDestroy(&pf->que);
    pthread_cond_destroy(&pf->cond);
    free(pf->pthId);
    printf("all is clear\n");
}

void alterOPNumToStr(char* to, int from)
{
    switch(from)
    {
    case CDCOMMEND:
        strcpy(to, "cd");
        break;
    case PWDCOMMEND:
        strcpy(to, "pwd");
        break;
    case PUTSCOMMEND:
        strcpy(to, "puts");
        break;
    case GETSCOMMEND:
        strcpy(to, "gets");
        break;
    case RMCOMMEND:
        strcpy(to, "rm");
        break;
    case LSCOMMEND:
        strcpy(to, "ls");
        break;
    case USERLOGIN:
        strcpy(to, "login");
        break;
    case USERENROLL:
        strcpy(to, "enroll");
        break;
    default:
        strcpy(to, "???");
        break;
    }
}