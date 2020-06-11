#include "../include/factory.h"

int recvCycle(int sfd, void *buf, long filesize)
{
    long total = 0;
    int ret;
    char *p = (char *)buf;
    while (total < filesize)
    {
        ret = recv(sfd, p + total, filesize - total, 0);
        if (0 == ret)
        {
            return -1;
        }
        total += ret;
    }
    return 0;
}

int checkMd5InMySQL(MYSQL *conn, const char *md5Str)
{
    char temp[300] = {0};
    char queryInfo[200] = "select * from file where md5 = '";
    sprintf(queryInfo, "%s%s'", queryInfo, md5Str);
    queryMySQL(conn, queryInfo, temp);
    if (strlen(temp) == 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int InsertFileInfo(MYSQL* conn, userState_t *pUState, char* fileType, char* fileName, char* md5Str, size_t fileSize)
{
    char insertInfo[200]="insert into file (prevID,creator,type,md5,clientName,filesize) values ";
    sprintf(insertInfo,"%s(%s,'%s','%s','%s','%s',%ld)",insertInfo,pUState->currentDirId,pUState->name,fileType,md5Str,fileName,fileSize);
    char bufTemp[100]={0};
    int ret=insertMySQL(conn,insertInfo,bufTemp);
    return ret;
}

int recvFile(int sfd, MYSQL* conn,linkMsg_t* plmsg, userState_t* pUState)
{
    int ret;
    char fileName[255] = {0};
    char md5Str[40] = {0};
    char fileType[4] = {0};
    size_t fileSize = plmsg->fileSize;
    sscanf(plmsg->buf, "%s %s %s", fileType, fileName, md5Str);
    char pathName[200]="../bin/";
    sprintf(pathName,"%s%s",pathName,md5Str);
    printf("%s\n",pathName);
    if(!checkMd5InMySQL(conn,md5Str))
    {
        InsertFileInfo(conn,pUState,fileType,fileName,md5Str,fileSize);
        printf("已经存在\n");
        plmsg->flag=EXIST_FLAG;
        plmsg->size=MSGHEADSIZE;
        send(sfd,plmsg,plmsg->size,0);
    }
    else
    {
        size_t offset=0;
        if(getUploadInfo(conn,&offset,md5Str)==0)
        {
        }
        else
        {
            insertUploadInfo(conn,fileSize,offset,md5Str);
        }
        printf("\noffset = %ld\n",offset);
        plmsg->flag=SUCCESS;
        bzero(plmsg->buf,sizeof(plmsg->buf));
        sprintf(plmsg->buf,"%ld",offset);
        plmsg->size=strlen(plmsg->buf)+MSGHEADSIZE;
        send(sfd,plmsg,plmsg->size,0);

        
        int fd=open(pathName,O_CREAT|O_RDWR,0666);
        ERROR_CHECK(fd,-1,"open");
        lseek(fd,offset,SEEK_SET);
        printf("\noffset = %ld\n",offset);
        //用splice接受文件
        int fds[2];
        pipe(fds);
        while(1)
        {
            ret=splice(sfd,NULL,fds[1],NULL,20000,SPLICE_F_MORE|SPLICE_F_MOVE);
            if(ret==0)
            {
                printf("ret=0传输完毕\n");
                break;
            }
            splice(fds[0],NULL,fd,NULL,ret,SPLICE_F_MORE|SPLICE_F_MOVE);
        }
#ifdef DEBUG_SERVER
        printf("\n\n--------------上传---------\n");
        printf("userid = %s\n userDir = %s\n", pUState->currentDirId, pUState->name);
        printf("type = %s#\nfileName = %s#\nmd5=%s#\nfileSize=%ld#\n",fileType,fileName,md5Str,fileSize);
#endif 
        struct stat filestat;
        fstat(fd,&filestat);
        if((size_t)filestat.st_size<fileSize)
        {
            updateUploadInfo(conn,(size_t)filestat.st_size,md5Str);
        }
        else
        {
            InsertFileInfo(conn, pUState, fileType, fileName, md5Str,fileSize);
            deleteUploadInfo(conn,md5Str);
        }
        close(fds[0]);
        close(fds[1]);
    }
    return 0;
}

int transmiss(int tranFd, MYSQL* conn,linkMsg_t *plmsg, userState_t* pUState)
{
    int ret;
    char TypeMd5SizeStr[300]={0};
    char fileName[200]={0};
    size_t offset=0;
    char md5Str[MD5SIZE]={0};
    char fileType[4]="-";
    size_t fileSize=0;
    sscanf(plmsg->buf,"%s %ld",fileName,&offset);
    printf("fileName=%s offset=%ld\n",fileName,offset);
    ret = getFileInfo(conn, fileName, pUState->currentDirId, TypeMd5SizeStr);
    if(-1 == ret)
    {
        printf("error in getFileInfo\n");
        sendErrorMsg(tranFd,plmsg);
        return -1;
    }
    //将数据分离方便映射
    sscanf(TypeMd5SizeStr, "%s %s %ld", fileType,md5Str,&fileSize);

    //将数据发送回去
    bzero(plmsg->buf, sizeof(plmsg->buf));
    strcpy(plmsg->buf,TypeMd5SizeStr);
    plmsg->flag = SUCCESS;
    plmsg->size = strlen(plmsg->buf) + MSGHEADSIZE;
    send(tranFd, plmsg, plmsg->size,0);

#ifdef DEBUG_SERVER
    printf("-----------transport 1 ------------\n");
    printf("size = %d, flag = %d fileSize = %ld\n", plmsg->size, plmsg->flag,plmsg->fileSize);
    printf("filename = %s\n", plmsg->buf);
#endif

    //send fileSize
    /*
    struct stat fileStat;
    ret = fstat(fd, &fileStat);
    ERROR_CHECK(ret, -1, "fstat");
    plmsg->fileSize = fileStat.st_size;
    */

#ifdef DEBUG_SERVER
    printf("\n\n------download--------\n");
    printf("fileType = %s##\n", fileType);
    printf("md5Str = %s##\n", md5Str);
    printf("fileSize=%ld\n", fileSize);
    printf("offset=%ld\n\n", offset);
#endif

    //打开文件,服务器上文件用md5码当文件名
    int fd = open(md5Str, O_RDONLY);//只读就可以了，不需要修改
    ERROR_CHECK(fd, -1, "open");
    //mmap文件映射
    char *pMap = (char*)mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fd, 0);
    ERROR_CHECK(pMap, (char*)-1, "mmap");
    //计时
    struct timeval start, end;
    gettimeofday(&start, NULL);
    //发送文件
    send(tranFd, pMap+offset, fileSize-offset, 0);
    ret = munmap(pMap, fileSize);
    ERROR_CHECK(ret, -1, "munmap");
    gettimeofday(&end, NULL);
    printf("use time is %ld\n", end.tv_usec-start.tv_usec+(end.tv_sec-start.tv_sec)*1000000);
    close(fd);
    //tranFd 是上一层的，交给上层去关
    return 0;
}