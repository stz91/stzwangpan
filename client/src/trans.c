#include "../include/factory.h"

int simpleCommend(int socketFd, char *dirName, int command, void (*print)(char *Info))
{
    linkMsg_t lmsg;
    memset(&lmsg, 0, sizeof(lmsg));
    lmsg.flag = command;
    strcpy(lmsg.buf, dirName);
    strcpy(lmsg.token, TOKEN);
    lmsg.size = MSGHEADSIZE + strlen(lmsg.buf);
    send(socketFd, &lmsg, lmsg.size, 0);
#ifdef ERROR_DEBUG
    printf("\n\nsend information\n");
    printf("lmsg.buf = %s##\n", lmsg.buf);
    printf("send lsmg.size = %d\n", lmsg.size);
#endif

    //接收返回信息
    int ret;
    bzero(lmsg.buf, strlen(lmsg.buf));
    ret = recvCycle(socketFd, &lmsg, MSGHEADSIZE);
    if (-1 == ret)
    {
        return -1;
    }
    ret = recvCycle(socketFd, lmsg.buf, lmsg.size - MSGHEADSIZE);
    if (-1 == ret)
    {
        return -1;
    }
    if (SUCCESS == lmsg.flag)
    {
        print(lmsg.buf);
    }
    else
    {
        printf("error:%s\n", lmsg.buf);
    }
    return 0;
}

int upload(int socketFd, char *filename)
{
    int ret;
    int fd = open(filename, O_RDONLY);
    ERROR_CHECK(fd, -1, "open");
    struct stat fileStat;
    fstat(fd, &fileStat);
    char md5Str[40] = {0};
    char fileType[4] = {0};
    if (S_ISDIR(fileStat.st_mode))
    {
        fileType[0] = 'd';
    }
    else
    {
        fileType[0] = '-';
    }
    Compute_file_md5(filename, md5Str);
    linkMsg_t lmsg;
    bzero(&lmsg, sizeof(lmsg));
    sprintf(lmsg.buf, "%s %s %s", fileType, filename, md5Str);
    lmsg.flag = PUTSCOMMEND;
    lmsg.size = strlen(lmsg.buf) + MSGHEADSIZE;
    lmsg.fileSize = fileStat.st_size;
    strcpy(lmsg.token, TOKEN);
    send(socketFd,&lmsg, lmsg.size, 0);

#ifdef TEST_DOWNLOAD
    printf("\n\n---------------upload.c line = %d------------\n", __LINE__);
    printf("size = %d, flag = %d, fileSize = %ld\n", lmsg.size, lmsg.flag, lmsg.fileSize);
    printf("token = %s\n", lmsg.token);
    printf("filename = %s\n", lmsg.buf);
#endif

    ret = recvCycle(socketFd, &lmsg, MSGHEADSIZE);
    recvCycle(socketFd, lmsg.buf, lmsg.size - MSGHEADSIZE);
    size_t offset;
    sscanf(lmsg.buf, "%ld", &offset);
    if (SUCCESS == lmsg.flag)
    {
        char *pMap = (char *)mmap(NULL, lmsg.fileSize, PROT_READ, MAP_SHARED, fd, 0);
        send(socketFd, pMap + offset, lmsg.fileSize - offset, 0);
        ret = munmap(pMap, lmsg.fileSize);
        ERROR_CHECK(ret, -1, "munmap");
        close(fd);
        return 0;
    }
    else if (EXIST_FLAG == lmsg.flag)
    {
        printf("upload success!\n");
        close(fd);
        return 0;
    }
    else
    {
        recvCycle(socketFd, lmsg.buf, lmsg.size - MSGHEADSIZE); //接收错误信息
        printf("upload refuse\n");
        printf("%s\n", lmsg.buf);
        close(fd);
        return lmsg.flag;
    }
}

int download(int socketFd, char* filename)
{
    //打开文件
    char pathName[100]={0};
    sprintf(pathName,"../bin/%s",filename);
    int newFileFd = open(pathName, O_CREAT| O_RDWR, 0666);
    ERROR_CHECK(newFileFd, -1, "open");

    /*获取当前文件大小，用做偏移量*/
    struct stat fileStat;
    fstat(newFileFd, &fileStat);

    /*传送文件名,当前文件大小*/
    linkMsg_t lmsg;
    memset(&lmsg, 0, sizeof(lmsg));
    sprintf(lmsg.buf, "%s %ld", filename, fileStat.st_size);
    strcpy(lmsg.token, TOKEN);
    lmsg.flag = GETSCOMMEND;
    lmsg.size = MSGHEADSIZE + strlen(lmsg.buf);
    send(socketFd, &lmsg, lmsg.size, 0);

#ifdef TEST_DOWNLOAD
    printf("\n\n------------------------download.c 1------------\n");
    printf("size = %d, flag = %d, fileSize = %ld\n", lmsg.size, lmsg.flag, lmsg.fileSize);
    printf("filename = %s\n", lmsg.buf);
#endif

    int ret;
    //接收文件大小,和服务器确认信息
    recvCycle(socketFd, &lmsg, MSGHEADSIZE);

#ifdef TEST_DOWNLOAD
    printf("\n\n----------------download.c 1-----------------\n");
    printf("size = %d, flag = %d, fileSize = %ld\n", lmsg.size, lmsg.flag, lmsg.fileSize);
    printf("%s\n", lmsg.buf);
#endif

    if(SUCCESS == lmsg.flag){
        //设置偏移量
        size_t offset = fileStat.st_size;
        lseek(newFileFd, offset, SEEK_CUR);

        //接收文件类型，Md5, 文件大小
        char fileType[4] = {0};
        char Md5Str[MD5SIZE] = {0};
        size_t fileSize = 0;
        recvCycle(socketFd, &lmsg.buf, lmsg.size - MSGHEADSIZE);
        sscanf(lmsg.buf,"%s %s %ld", fileType, Md5Str, &fileSize);
        /*
           size_t totalNow = offset;
           size_t prev = totalNow;
           size_t limitSize = fileSize/10000;
           */
        int fds[2];
        pipe(fds);
        while(1)
        {
            ret = splice(socketFd, NULL, fds[1], NULL, 20000, SPLICE_F_MORE | SPLICE_F_MOVE);
            //printf("ret = %d\n", ret);
            if(0 == ret)
            {  
                printf("传输完毕\n");
                break;
            }
            splice(fds[0], NULL, newFileFd, NULL, ret, SPLICE_F_MORE | SPLICE_F_MOVE);
            /*
               totalNow += ret;
               if(totalNow - prev > limitSize)
               {
               printf("%50.2lf%%\r", (double)totalNow/fileSize*100);
               fflush(stdout);
               prev = totalNow;
               }
               */
        }
        ret = close(fds[1]);
        ret = close(fds[0]);
        close(newFileFd); 
        printf("download success\n");
        return 0;
    }else if(FILE_EXIST_FLAG == lmsg.flag){
        printf("文件已存在\n");
        return 0;
    }else{
        printf("download refuse\n");
        printf("%s\n", lmsg.buf);       //服务器拒绝，并返回拒绝信息   
        close(newFileFd);
        return -1;
    }
}