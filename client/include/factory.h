#pragma once
#include "work_que.h"
#include "head.h"
#include "md5.h"
#include "list.h"

#define DEBUG_SERVER
//表示使用mmap的临界值
#define BIGFILESIZE 100 * 1024 * 1024  
//一般缓冲区大小
#define BUFSIZE 4096
#define MSGBUFSIZE 2048
#define MSGHEADSIZE 56
#define DIRIDSIZE 30
#define DIRNAMESIZE 50
#define USERIDSIZE 20
#define USERNAMESIZE 50
#define PASSWDSIZE 120    //加密后长度为98
#define MD5SIZE 40
#define STDIN_MAX 255

typedef struct{
    Que_t que;
    pthread_cond_t cond;
    pthread_t *pthId;
    int pthreadNum;
    int startFlag;
}factory_t,*pfactory_t;

typedef struct linkMsg{
    int size;
    int flag;//标志位 读写浏览
    char token[40];
    size_t fileSize;
    char buf[BUFSIZE];//文件名，文件大小，接收目录
}linkMsg_t,*plinkMsg_t;

typedef struct{
    char name[USERIDSIZE];
    char currentDirId[DIRIDSIZE];
}userState_t;

typedef struct{
    char dirID[DIRIDSIZE];
    char dirName[DIRNAMESIZE];
}dirState_t,*pdirState_t;

/*客户端发给服务器的操作码*/
enum commend_num
{
    USERENROLL = 0,
    USERLOGIN,
    CDCOMMEND,
    LSCOMMEND,
    PUTSCOMMEND,
    GETSCOMMEND,
    RMCOMMEND,
    PWDCOMMEND,
    MKDIRCOMMEND,
    HELPCOMMEND,
    EXITCOMMEND,
};

/*服务器发给客户端的确认码*/
enum flag_return
{
    FAIL_FLAG = 100,
    SUCCESS,
    TOKEN_OVERTIME,
    EXIST_FLAG,
    FILE_EXIST_FLAG,
    FRAG_FLAG
};

extern char TOKEN[40];

int epollAdd(int epfd,int fd);
int factoryInit(factory_t*,int,int);
int factoryStart(factory_t*);
void factoryDestroy(pfactory_t pf);
void* pthreadFunc(void* p);

int factoryInit(pfactory_t pf,int threadNum,int capacity);
void threadExitFunc(void *p);
void sendMSG(int socketFd, int command, linkMsg_t* plmsg);
void* threadFun(void *p);
int factoryStart(pfactory_t pf);
void factoryDestroy(pfactory_t pf);

int recvCycleFile(int sfd, void* buf, int fileSize);
int recvCycle(int sfd, void* buf, int fileSize);
int tcp_client(char* ip, char* port);
int simpleCommend(int socketFd, char* dirName, int command, void (*print)(char* Info));
int upload(int socketFd, char *filename);
int download(int socketFd, char* filename);

int windowForLogin(int fd);
void helpManual();
void printForCd(char* buf);
void printForLs(char* buf);
void printForPwd(char* buf);
void printForRm(char* buf);
void printForMkdir(char* buf);
void printForPwd2(char* buf);

int getCommendFromStdin(char* dataStr);
