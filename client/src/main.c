#include "../include/factory.h"


char pwdStr[100];
char nameStr[40];
int exitFds[2];
void exitHandler(int signum)
{
    printf("%d is coming\n", signum);
    write(exitFds[1], "1", 1);
}

int main(int argc,char* argv[])
{
    system("stty erase ^H");
    ARGS_CHECK(argc,3);
    pipe(exitFds);
    while(fork())           //防止客户端崩溃
    {
        signal(SIGUSR1, exitHandler);
        int status;
        wait(&status);
        if(WIFEXITED(status))
        {
            close(exitFds[1]);
            close(exitFds[0]);
            exit(0);
        }
    }
    bzero(nameStr,sizeof(nameStr));
    close(exitFds[1]);
    factory_t pf;
    int capacity = 10;
    int threadNum = 5;
    factoryInit(&pf, threadNum, capacity);
    factoryStart(&pf);
    //连接服务器
    int ret,socketFd;
    char ServerIP[30] = {0};
    char ServerPort[10] = {0};
    strcpy(ServerIP, argv[1]);
    strcpy(ServerPort, argv[2]);
    socketFd = tcp_client(ServerIP, ServerPort);
    ERROR_CHECK(socketFd, -1, "tcp_client");

    //先进行登录或者注册
begin:    
    if(-1 == windowForLogin(socketFd)){
        printf("goodbye\n");
        return -1;
    }
    //初始化队列成员
    pQue_t pq = &pf.que;
    pNode_t pNew;

    //监控输入端和服务器
    int epfd = epoll_create(1);
    ERROR_CHECK(epfd, -1, "epoll_create");
    struct epoll_event evs[2];
    epollAdd(epfd,socketFd);
    epollAdd(epfd, STDIN_FILENO);
    int readyNumOfFd;
    int i, commend;
    char bufTemp[MSGBUFSIZE] = {0};
    char serverInfo[MSGBUFSIZE] = {0};
    bzero(pwdStr,sizeof(pwdStr));
    while(1){
        printf("%s@wangpan:~%s$ ",nameStr,pwdStr);
        fflush(stdout);
        bzero(bufTemp, sizeof(bufTemp));
        readyNumOfFd = epoll_wait(epfd, evs, 2, -1);
        for(i=0;i<readyNumOfFd;i++){
            if(evs[i].data.fd == socketFd){
                ret = recv(socketFd, serverInfo, MSGBUFSIZE, 0);
                if(0 == ret)
                {
                    goto END;
                }
            }
            if(evs[i].data.fd == STDIN_FILENO)
            {
                commend = getCommendFromStdin(bufTemp);
                switch(commend){
                case CDCOMMEND:
                    ret = simpleCommend(socketFd, bufTemp, CDCOMMEND, printForCd);
                    ret =simpleCommend(socketFd,bufTemp,PWDCOMMEND, printForPwd2);
                    if(-1 == ret){
                        printf("error in cd\n");
                        return -1;
                    }
                    break;
                case LSCOMMEND:
                    ret = simpleCommend(socketFd, bufTemp,LSCOMMEND, printForLs);
                    if(-1 == ret){
                        return -1;
                    }
                    break;
                case PUTSCOMMEND:
                    pNew = (pNode_t)calloc(1, sizeof(Node_t));
                    pNew->flag = PUTSCOMMEND;
                    strcpy(pNew->fileName,bufTemp);
                    strcpy(pNew->ip, ServerIP);
                    strcpy(pNew->port, ServerPort);
                    pthread_mutex_lock(&pq->mutex);
                    queInsert(pq, pNew);
                    pthread_mutex_unlock(&pq->mutex);
                    pthread_cond_signal(&pf.cond);
                    break;
                case GETSCOMMEND:
                    pNew = (pNode_t)calloc(1, sizeof(Node_t));
                    pNew->flag = GETSCOMMEND;
                    strcpy(pNew->fileName,bufTemp);
                    strcpy(pNew->ip, ServerIP);
                    strcpy(pNew->port, ServerPort);
                    pthread_mutex_lock(&pq->mutex);
                    queInsert(pq, pNew);
                    pthread_mutex_unlock(&pq->mutex);
                    pthread_cond_signal(&pf.cond);
                    break;
                case RMCOMMEND:
                    ret =simpleCommend(socketFd, bufTemp, RMCOMMEND, printForRm);
                    if(-1 == ret){
                        return -1;
                    }
                    break;
                case PWDCOMMEND:
                    ret =simpleCommend(socketFd,bufTemp,PWDCOMMEND, printForPwd);
                    if(-1 == ret){
                        return -1;
                    }
                    break;
                case MKDIRCOMMEND:
                    ret=simpleCommend(socketFd,bufTemp,MKDIRCOMMEND,printForMkdir);
                    if(-1==ret){
                        return -1;
                    }
                    break;
                case HELPCOMMEND:
                    helpManual();
                    break;

                case EXITCOMMEND:
                    goto begin;
                default:
                    break;
                }

            }
        }
    }
END:
    printf("server is out\n");
    close(socketFd);
    return 0;
}