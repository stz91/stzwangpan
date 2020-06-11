#include "../include/factory.h"

void getDirNameFromDirId(MYSQL *conn, char* DirID, char* DirName)
{
    char queryInfo[100] = "select clientName from file where dirID=";
    sprintf(queryInfo,"%s%s", queryInfo, DirID);
    queryMySQL(conn, queryInfo, DirName);
}

void sendErrorMsg(int fd, linkMsg_t* plmsg)
{
    plmsg->flag = FAIL_FLAG;
    plmsg->size = MSGHEADSIZE + strlen(plmsg->buf);
    send(fd, plmsg, plmsg->size, 0);
}

void reverse(char* str, int low, int high)
{
    int mid = low + (high - low)/2;
    char temp;
    for(;low<=mid;low++, high--)
    {
        temp = str[low];
        str[low] = str[high];
        str[high] = temp;
    }
}

static void GenerateStr(char *str)
{
    int i=0, flag;
    str[0] = '$';
    str[1] = '6';
    str[2] = '$';
    srand(time(NULL));
    for(i=3;i<11;i++)
    {
        flag = rand()%3;
        switch(flag)
        {
        case 0:
            str[i] = rand()%26 + 'a';
            break;
        case 1:
            str[i] = rand()%26 + 'A';
            break;
        case 2:
            str[i] = rand()%10 + '0';
            break;
        }
    }
    //printf("%s\n", str);    
}

static int apartUserInformation(char* buf, User_t* pUser)
{
    printf("i am apart");
    char* p=buf;
    int i=0;
    while(*p != '$')
    {
        pUser->name[i++] = *p;
        p++;
    }
    p++;
    i=0;
    while(*p != '$' && *p != '\0')
    {
        pUser->passwd[i++] = *p;
        p++;
    }
    printf("name:%s\npasswd:%s\n",pUser->name,pUser->passwd);
    return 0;
}

int userEnroll(int fd, MYSQL* conn,linkMsg_t* plmsg)
{
    User_t user;
    int ret;
    bzero(&user, sizeof(User_t));
    printf("begin\n");
    apartUserInformation(plmsg->buf, &user);
    printf("ready\n");
    //清空信息，保存返回信息
    bzero(plmsg->buf,sizeof(plmsg->buf));
    
    GenerateStr(user.salt);
    //将密码暂存至一个字符数组，生成密文，防止明文密码长于密文导致出错
    char TempBuf[100] = {0};
    strcpy(TempBuf, user.passwd);
    memset(user.passwd, 0, sizeof(user.passwd));
    strcpy(user.passwd, crypt(TempBuf, user.salt));
    //send(fd,user.salt, strlen(user.salt),0);
    char insertUser[300] = {0};
    char insertHead[50]  = "INSERT INTO account(name, salt, password) values(";
    sprintf(insertUser,"%s'%s','%s','%s')",insertHead,user.name, user.salt, user.passwd);
    ret = insertMySQL(conn,insertUser,plmsg->buf);
    if(-1 == ret)
    {
        sendErrorMsg(fd,plmsg);
        return -1;
    }
    //将目录插入文件表,目录没有md5，和真实文件。
    char insertFile[300] = "INSERT INTO file(prevID,creator,type,clientName) values(";
    sprintf(insertFile, "%s%s,'%s','%s','%s')",insertFile, "-1", user.name, "d","/");
    ret = insertMySQL(conn, insertFile, plmsg->buf);
    if(-1 == ret)
    {
        sendErrorMsg(fd,plmsg);
        return -1;
    }
    //获取新增目录的id
    long dirID = mysql_insert_id(conn); 
    //更新用户主目录id
    char updateUser[300] = "UPDATE account set dirID=";
    sprintf(updateUser, "%s%ld %s%s'",updateUser, dirID, "where name='",user.name);
    ret = updateMySQL(conn, updateUser, plmsg->buf);
    if(-1 == ret)
    {
        sendErrorMsg(fd,plmsg);
        return -1;
    }

#ifdef DEBUG_SERVER
    printf("\n\n***************enroll user************\n");
    printf("insertUser=%s\n", insertUser);
    printf("salt = %s\n", user.salt);
    printf("crypt = %s\n", crypt(user.passwd, user.salt));
    printf("dirID=%ld\n", dirID);
    printf("insertFile=%s\n", insertFile);
    printf("updateUser = %s\n", updateUser);
#endif

    printf("insert success\n");

    //注册成功，发送信息给客户端
    plmsg->flag = SUCCESS;
    plmsg->size = MSGHEADSIZE + strlen(plmsg->buf);
    send(fd, plmsg, plmsg->size,0); 
    return 0;
}

int userLogin(int fd, MYSQL* conn, linkMsg_t* plmsg, userState_t* pUState)
{
    User_t uState;
    bzero(&uState,sizeof(uState));
    int ret;
    strcpy(uState.name,plmsg->buf);
    char queryInfo[200]="select * from account where name = '";
    sprintf(queryInfo,"%s%s'",queryInfo,uState.name);
    printf("select * from account where name = '%s'\n",uState.name);
    ret=queryMySQLForUser(conn, queryInfo, &uState);
    printf("ret=%d\n",ret);
    if(-1==ret)
    {
        strcpy(plmsg->buf,"用户不存在");
        sendErrorMsg(fd,plmsg);
        return -1;
    }
#ifdef DEBUG_SERVER
    printf("\n\n-----------------");
    printf("uState.name = %s\n", uState.name);
    printf("uState.salt = %s\n", uState.salt);
    printf("uState.id = %s\n", uState.id);
    printf("uState.passwd = %s\n", uState.passwd);
    printf("uState.dirID = %s\n", uState.dirID);
#endif
    
    bzero(plmsg->buf,strlen(plmsg->buf));
    strcpy(plmsg->buf,uState.salt);
    plmsg->size=strlen(plmsg->buf)+MSGHEADSIZE;
    plmsg->flag=SUCCESS;
    send(fd,plmsg,plmsg->size,0);
    
    recvCycle(fd, plmsg, MSGHEADSIZE);
    recvCycle(fd, plmsg->buf, plmsg->size-MSGHEADSIZE);
    if(!strcmp(uState.passwd,plmsg->buf))
    {
        time_t nowTime;
        nowTime=time(NULL);
        updateOPTime(conn,uState.id,nowTime);
        char dest[100]={0};
        sprintf(dest,"%s%ld",uState.name,nowTime);
        char md5_string[40];
        Compute_string_md5((unsigned char*)dest,strlen(dest),md5_string);
        plmsg->flag=SUCCESS;
        strcpy(plmsg->buf,md5_string);
        plmsg->size=strlen(plmsg->buf)+MSGHEADSIZE;
        send(fd,plmsg,plmsg->size,0);
        char InsertMd5Str[200]="update account set token=";
        sprintf(InsertMd5Str, "%s'%s' %s'%s'", InsertMd5Str, md5_string, "where id=", uState.id);
        updateMySQL(conn, InsertMd5Str, NULL);
    }
    else
    {
        plmsg->flag = FAIL_FLAG;
        strcpy(plmsg->buf,"密码错误");
        plmsg->size = strlen(plmsg->buf)+MSGHEADSIZE;
        send(fd, plmsg, plmsg->size, 0);
        return -1;
    }
    //将用户状态保存在另一个结构体中
    strcpy(pUState->name,uState.name);
    strcpy(pUState->currentDirId,uState.dirID);
    updateCurrentDir(conn, pUState->name,pUState->currentDirId);
    printf("userid = %s\nuserDir = %s\n", pUState->currentDirId, pUState->name);
    printf("success\n");
    return 0;
}

int CdCommand(int fd,MYSQL* conn, linkMsg_t* plmsg, userState_t* pUState)
{
    int ret;
    char bufTemp[200]={0};
    if(!strncmp(plmsg->buf,"..",2))
    {
        char queryInfo[200]="select prevID from file where prevID>-1 and dirID=";
        sprintf(queryInfo,"%s'%s'",queryInfo,pUState->currentDirId);
        ret=queryMySQL(conn,queryInfo,bufTemp);
        printf("%s\n",bufTemp);
        if(ret!=0)
        {
            strcpy(plmsg->buf,"已经到顶层了");
            printf("已经到顶层了\n");
            sendErrorMsg(fd,plmsg);
            return -1;
        }
        bzero(pUState->currentDirId,sizeof(pUState->currentDirId));
        strcpy(pUState->currentDirId,bufTemp);
        updateCurrentDir(conn,pUState->name,pUState->currentDirId);
        printf("当前目录为%s#\n",pUState->currentDirId);
        plmsg->flag=SUCCESS;
        bzero(plmsg->buf,sizeof(plmsg->buf));
        plmsg->size=MSGHEADSIZE;
        send(fd,plmsg,plmsg->size,0);
    }
    else
    {
        char buf[100]={0};
        strncpy(buf,plmsg->buf,strlen(plmsg->buf));
        char queryInfo[200]="select dirID from file where prevID=";
        sprintf(queryInfo, "%s%s %s'%s'", queryInfo, pUState->currentDirId,"and clientName =", buf);
        ret=queryMySQL(conn,queryInfo,bufTemp);
        if(-2==ret)
        {
            strcpy(plmsg->buf,"No such file or directory");
            sendErrorMsg(fd,plmsg);
            return -1;
        }
        bzero(pUState->currentDirId, sizeof(pUState->currentDirId));
        strcpy(pUState->currentDirId,bufTemp);
        updateCurrentDir(conn,pUState->name, pUState->currentDirId);

        //发送成功消息给客户端
        plmsg->flag = SUCCESS;
        bzero(plmsg->buf,sizeof(plmsg->buf));
        plmsg->size = MSGHEADSIZE + strlen(plmsg->buf);
        send(fd, plmsg, plmsg->size, 0);
        printf("CD success\n");
    }
    return 0;
}

int RmCommand(int fd, MYSQL* conn, linkMsg_t* plmsg, userState_t* pUState)
{
    int ret;
    char fileName[256] = {0};
    // char bufTemp[100]={0};
    strncpy(fileName, plmsg->buf, 255);
    // char queryInfo[100]="select md5 from file where prevID=";
    // sprintf(queryInfo, "%s%s %s'%s'", queryInfo, pUState->currentDirId, "and clientName=", fileName);
    // int ret=queryMySQLForRm(conn,queryInfo,bufTemp);
    // printf("%s\n",bufTemp);
    // sprintf(bufTemp,"%s",bufTemp);
    // ret=remove(bufTemp);
    // ERROR_CHECK(ret,-1,"remove");
    char deleteInfo[300]="delete from file where prevID=";
    sprintf(deleteInfo, "%s%s %s'%s'", deleteInfo, pUState->currentDirId, "and clientName=", fileName);
#ifdef DEBUG_SERVER
    printf("rm deleteInfo=%s#\n", deleteInfo);
#endif
    bzero(plmsg->buf, sizeof(plmsg->buf));
    ret =deleteMySQL(conn, deleteInfo, plmsg->buf);
    printf("delete ret = %d\n", ret);
    plmsg->flag = SUCCESS;
    plmsg->size = strlen(plmsg->buf) + MSGHEADSIZE;
    send(fd, plmsg, plmsg->size, 0);
    return 0;
}

int LsCommand(int fd, MYSQL* conn, userState_t* pUState)
{
    char queryInfo[100] = "select type, fileSize,clientName from file where prevID=";
    sprintf(queryInfo,"%s%s", queryInfo,pUState->currentDirId);
    printf("in ls dirId=%s#", pUState->currentDirId);
    linkMsg_t lmsg;
    bzero(&lmsg, sizeof(lmsg));
    lmsg.flag = SUCCESS;
    //将目录消息放入lmsg.buf
    /*这里目录长度可能会超过lmsg.buf最大值*/
    queryMySQL(conn, queryInfo, lmsg.buf);
    lmsg.size = strlen(lmsg.buf) + MSGHEADSIZE;
    send(fd, &lmsg, lmsg.size,0);
    return 0;
}

int PwdCommand(int fd,MYSQL* conn,userState_t* pUState)
{
    char reverseStr[100]={0};
    char reverseStr2[100]={0};
    dirState_t fatherDir;
    bzero(&fatherDir,sizeof(fatherDir));
    char childDirID[50]={0};
    strcpy(childDirID,pUState->currentDirId);
    while(findFatherDirID(conn,childDirID,&fatherDir)!=-1)
    {
        reverse(fatherDir.dirName,0,strlen(fatherDir.dirName)-1);
        sprintf(reverseStr,"%s/%s",reverseStr,fatherDir.dirName);
        strcpy(childDirID,fatherDir.dirID);
    }
    reverseStr[strlen(reverseStr)]='/';
    strncpy(reverseStr2,reverseStr+1,strlen(reverseStr)-1);
    printf("reverseStr=%s\n",reverseStr2);
    reverse(reverseStr2,0,strlen(reverseStr2)-1);
    printf("reverseStr=%s\n",reverseStr2);
    // int low=0,high=0;
    // int flagIn=0;
    // for(size_t i=0;i<strlen(reverseStr2);i++)
    // {
    //     if(reverseStr2[i] != '/' && 0 == flagIn)
    //     {
    //         low = i;
    //         flagIn = 1;
    //     }else if(1 == flagIn && '/' ==  reverseStr2[i]){
    //         high = i-1;
    //         reverse(reverseStr2, low, high);
    //         flagIn = 0;
    //     }
    // }
    linkMsg_t lmsg;
    bzero(&lmsg, sizeof(lmsg));
    lmsg.flag = SUCCESS;
    if(strlen(reverseStr2)==0)
    {
        sprintf(reverseStr2,"/home/%s",pUState->name);
    }
    strcpy(lmsg.buf, reverseStr2);
    lmsg.size = MSGHEADSIZE + strlen(lmsg.buf);
    send(fd, &lmsg, lmsg.size, 0);
    return 0;
}

int findFatherDirID(MYSQL* conn,char* childDirID,pdirState_t fatherDir)
{
    bzero(fatherDir->dirID,DIRIDSIZE);
    bzero(fatherDir->dirName,DIRNAMESIZE);
    char bufTemp[300]={0};
    char queryInfo[100]="select prevID,clientName from file where dirID=";
    sprintf(queryInfo,"%s'%s'",queryInfo,childDirID);
    queryMySQL(conn,queryInfo,bufTemp);
    sscanf(bufTemp,"%s %s",fatherDir->dirID,fatherDir->dirName);
    if(!strcmp(fatherDir->dirID,"-1"))
    {
        return -1;
    }
    return 0;
}

int MkdirCommand(int fd,MYSQL* conn,linkMsg_t* plmsg,userState_t* pUState)
{
    int ret;
    char fileName[100]={0};
    char bufTemp[100]={0};
    strcpy(fileName,plmsg->buf);
    char insertInfo[100]="insert into file (prevID,creator,type,md5,clientName) values";
    sprintf(insertInfo,"%s(%s,'%s','%s','%s','%s')",insertInfo,pUState->currentDirId,pUState->name,"d","/",fileName);
    ret=insertMySQL(conn,insertInfo,bufTemp);
    if(-1==ret)
    {
        strcpy(plmsg->buf,"mkdir fail\n");
        sendErrorMsg(fd,plmsg);
    }
    else
    {
        printf("mkdir success\n");
        plmsg->flag=SUCCESS;
        bzero(plmsg->buf,sizeof(plmsg->buf));
        strcpy(plmsg->buf,"mkdir success");
        plmsg->size=MSGHEADSIZE+strlen(plmsg->buf);
        send(fd,plmsg,plmsg->size,0);
    }
    return 0;
}