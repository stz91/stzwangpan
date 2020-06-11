#include "../include/factory.h"

int exitFds[2];

void exitHandler(int signum)
{
    printf("%d is coming\n",signum);
    write(exitFds[1],"1",1);//给exitFds[1]发送一个信号，使其退出
}

int main(int argc,char* argv[])
{
    if(5!=argc)
    {
        printf("./server IPadrr port pthreadNum capacity\n");
        return -1;
    }
    pipe(exitFds);
    // printf("%d,%d\n",exitFds[0],exitFds[1]);
    while(fork())
    {
        signal(SIGUSR1,exitHandler);
        int status;
        wait(&status);//随机等待一个要退出的fd
        if(WIFEXITED(status))
        {
            close(exitFds[1]);
            exit(0);
        }
    }
    close(exitFds[1]);
    factory_t pf;
    int threadNum=atoi(argv[3]);
    int capacity=atoi(argv[4]);
    // printf("ready\n");
    factoryInit(&pf,threadNum,capacity);
    factoryStart(&pf);
    // printf("finish\n");
    int socketFd=tcpInit(argv[1],argv[2]);
    ERROR_CHECK(socketFd,-1,"socketFd");
    int newFd;
    int epfd=epoll_create(1);
    ERROR_CHECK(epfd,-1,"epoll_create");
    struct epoll_event evs[2];
    epollAdd(epfd,socketFd);
    epollAdd(epfd,exitFds[0]);
    int readyFdNum;
    int i,j;
    pNode_t pNew;
    pQue_t pq=&pf.que;
    while(1)
    {
        readyFdNum=epoll_wait(epfd,evs,2,-1);
        printf("newFd=%d\n",newFd);
        for(i=0;i<readyFdNum;i++)
        {
            if(evs[i].data.fd==exitFds[0])
            {
                for(j=0;j<pf.pthreadNum;j++){
                    pthread_cancel(pf.pthId[j]);
                }
                for(j=0;j<pf.pthreadNum;j++){
                    pthread_join(pf.pthId[j], NULL);
                }
                close(socketFd);
                factoryDestroy(&pf);
                exit(0);
            }
            if(evs[i].data.fd==socketFd)//有新的客户端请求
            {
                newFd=accept(socketFd,NULL,NULL);
                printf("newFd=%d\n",newFd);
                pNew=(pNode_t)calloc(1,sizeof(Node_t));
                pNew->newFd=newFd;
                pthread_mutex_lock(&pq->mutex);
                queInsert(pq,pNew);
                pthread_mutex_unlock(&pq->mutex);
                pthread_cond_signal(&pf.cond);//通知子线程去干活
            }
        }
    }
}
