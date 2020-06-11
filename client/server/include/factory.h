#pragma once
#include "work_que.h"
#include "head.h"
#include "md5.h"

#define DEBUG_SERVER
//表示使用mmap的临界值
#define BIGFILESIZE 100 * 1024 * 1024  
//一般缓冲区大小
#define BUFSIZE 4096
#define MSGHEADSIZE 56
#define DIRIDSIZE 30
#define DIRNAMESIZE 50
#define USERIDSIZE 20
#define USERNAMESIZE 50
#define PASSWDSIZE 120    //加密后长度为98
#define MD5SIZE 40

typedef struct{
    Que_t que;
    pthread_cond_t cond;
    pthread_t *pthId;
    int pthreadNum;
    int startFlag;
}factory_t,*pfactory_t;

typedef struct{

}LinkMsg_t,pLinkMsg_t;

typedef struct linkMsg{
    int size;
    int flag;//标志位 读写浏览
    char token[40];
    size_t fileSize;
    char buf[BUFSIZE];//文件名，文件大小，接收目录
}linkMsg_t;

typedef struct user{
    char id[USERIDSIZE];
    char name[USERNAMESIZE];
    char passwd[PASSWDSIZE];
    char salt[20];
    char dirID[DIRIDSIZE];
    time_t lastOPTime;
}User_t;

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
    MKDIRCOMMEND
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

int tcpInit(char* ip, char* port);
int epollAdd(int epfd,int fd);
int factoryInit(factory_t*,int,int);
int factoryStart(factory_t*);
void factoryDestroy(pfactory_t pf);
void* pthreadFunc(void* p);


int connectMYSQL(MYSQL** pConn);
int queryMySQL(MYSQL* conn, char* queryInfo, char* resultInfo);
int insertMySQL(MYSQL* conn, char* insertInfo, char* resultInfo);
int deleteMySQL(MYSQL *conn, char* deleteInfo, char* resultInfo);
int updateMySQL(MYSQL* conn, char* updateInfo, char* resultInfo);
int getUserNameAndCurDir(MYSQL* conn, char* token, char* userName, char* currentDirId);
int checkToken(MYSQL* conn, char* token,int outTime);
int updateOPTime(MYSQL* conn, char* userID, time_t t);
int queryMySQLForUser(MYSQL *conn, char *queryInfo, User_t *pUser);
int updateCurrentDir(MYSQL* conn,char* userName, char* currentDirId);
int getFileInfo(MYSQL* conn, char* fileName, char* currentDirId,char* result);
int getUploadInfo(MYSQL* conn, size_t* pOffset, char* md5);
int updateUploadInfo(MYSQL* conn, size_t offset, char* md5);
int insertUploadInfo(MYSQL* conn, size_t fileSize,size_t offset,char* md5);
int deleteUploadInfo(MYSQL* conn, char* md5);
int queryMySQLForRm(MYSQL *conn, char *queryInfo, char *resultInfo);
void updateOPLog(MYSQL* conn,char* name,char* opStr);
void alterOPNumToStr(char* to, int from);

int recvCycle(int sfd,void* buf,long filesize);
int checkMd5InMySQL(MYSQL *conn, const char *md5Str);
int InsertFileInfo(MYSQL* conn, userState_t *pUState, char* fileType, char* fileName, char* md5Str, size_t fileSize);
int recvFile(int sfd, MYSQL* conn,linkMsg_t* plmsg, userState_t* pUState);
int transmiss(int tranFd, MYSQL* conn,linkMsg_t *plmsg, userState_t* pUState);

void getDirNameFromDirId(MYSQL *conn, char* DirID, char* DirName);
void sendErrorMsg(int fd, linkMsg_t* plmsg);
void reverse(char* str, int low, int high);
int userEnroll(int fd, MYSQL* conn,linkMsg_t* plmsg);
int userLogin(int fd, MYSQL* conn, linkMsg_t* plmsg, userState_t* pUState);
int CdCommand(int fd,MYSQL* conn, linkMsg_t* plmsg, userState_t* pUState);
int RmCommand(int fd, MYSQL* conn, linkMsg_t* plmsg, userState_t* pUState);
int LsCommand(int fd, MYSQL* conn, userState_t* pUState);
int findFatherDirID(MYSQL* conn,char* childDirID,pdirState_t fatherDir);
int PwdCommand(int fd,MYSQL* conn,userState_t* pUState);
int MkdirCommand(int fd,MYSQL* conn,linkMsg_t* plmsg,userState_t* pUState);


