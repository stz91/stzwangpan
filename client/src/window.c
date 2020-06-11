#include "../include/factory.h"

char TOKEN[40];
char password1[100];
char password2[100];
extern char pwdStr[100];
extern char nameStr[40];
static int userEnroll(int fd)
{
    linkMsg_t lmsg;
    bzero(&lmsg, sizeof(lmsg));
    printf("username:\n");
    fgets(lmsg.buf, 20, stdin);
    lmsg.buf[strlen(lmsg.buf) - 1] = '$';

    printf("password:\n");
    fgets(password1, 20, stdin);
    printf("password again\n");
    fgets(password2, 20, stdin);
    if(strcmp(password1,password2))
    {
        printf("两次密码不一致,任意键返回\n");
        getchar();
        return -1;
    }
    sprintf(lmsg.buf,"%s%s",lmsg.buf,password1);
    lmsg.buf[strlen(lmsg.buf) - 1] = '\0';

    //printf("test buf = %s##\n", lmsg.buf);

    lmsg.size = MSGHEADSIZE + strlen(lmsg.buf);
    lmsg.flag = USERENROLL;
    send(fd, &lmsg, lmsg.size, 0);
    // printf("buf=%s\n", lmsg.buf);
    //为什么没有卡住?
    //收到信息，但是马上被刷新了，没看到。
    recvCycle(fd, &lmsg, MSGHEADSIZE);
    recvCycle(fd, lmsg.buf, lmsg.size - MSGHEADSIZE);
    // printf("recv success\n");
    if (FAIL_FLAG == lmsg.flag)
    {
        // printf("%s\n", lmsg.buf);
        return -1;
    }
    else
    {
        // printf("%d\n", lmsg.flag);
        printf("注册成功！\n");
    }
    printf("任意键返回\n");
    getchar();
    return 0;
}

static int userLogin(int fd)
{
    linkMsg_t lmsg;
    bzero(&lmsg, sizeof(lmsg));
    printf("username:");
    fflush(stdout);
    fgets(lmsg.buf, 20, stdin);
    lmsg.buf[strlen(lmsg.buf) - 1] = '\0';
    strcpy(nameStr,lmsg.buf);
    lmsg.flag = USERLOGIN;
    lmsg.size = strlen(lmsg.buf) + MSGHEADSIZE;
    send(fd, &lmsg, lmsg.size, 0);

    //接收返回信息，salt值或者错误值
    recvCycle(fd, &lmsg, MSGHEADSIZE);
    recvCycle(fd, lmsg.buf, lmsg.size - MSGHEADSIZE);
    if (SUCCESS == lmsg.flag)
    {
        //将密码生成秘钥
        char *passwd;
        passwd = getpass("password:");
        char *tempStr = crypt(passwd, lmsg.buf);
        bzero(lmsg.buf, sizeof(lmsg.buf));
        strcpy(lmsg.buf, tempStr);

        //将密文发回去给服务器
        lmsg.size = strlen(lmsg.buf) + MSGHEADSIZE;
        lmsg.flag = USERLOGIN;
        send(fd, &lmsg, lmsg.size, 0);

        //接收返回来的消息
        recvCycle(fd, &lmsg, MSGHEADSIZE);
        memset(lmsg.buf, 0, sizeof(lmsg.buf));
        recvCycle(fd, lmsg.buf, lmsg.size - MSGHEADSIZE);
        //printf("lmsg.buf=%s\n", lmsg.buf);
        //将token值保存到全局变量区
        strcpy(TOKEN, lmsg.buf);
        if (SUCCESS == lmsg.flag)
        {
            return 0;
        }
        else
        {
            printf("error:%s,按任意键跳转\n", lmsg.buf);
            getchar();
            return -1;
        }
    }
    else
    {
        printf("error: %s,按任意键跳转\n", lmsg.buf);
        getchar();
        return -1;
    }
}

void helpManual()
{
    system("clear");
    int i;
    for (i = 0; i < 60; i++)
    {
        printf("-");
    }
    printf("\n");
    printf("1.输入ls, 查看当前目录下文件\n");
    printf("2.输入pwd, 查看当前目录路径\n");
    printf("3.输入gets + 文件名, 从网盘下载文件\n");
    printf("4.输入puts + 文件名, 上传文件至网盘\n");
    printf("5.输入cd + 目录名, 进入目录\n");
    printf("6.输入rm + 文件名(或remove + 文件名), 删除指定文件\n");
    printf("7.输入quit,返回到登陆界面\n");
    printf("8.输入mkdir + 目录名, 创建目录\n");
}

int windowForLogin(int fd)
{
    system("stty erase ^H");
    int i, ret;
    char ch;
    while (1)
    {
        system("clear");
        for (i = 0; i < 80; i++)
        {
            printf("*");
        }
        printf("\n1.login\n2.enroll\n3.quit\n");
        ch = getchar();
        getchar();
        if ('1' == ch)
        {
            ret = userLogin(fd);
            if (0 == ret)
            {
                return 0;
            }
        }
        else if ('2' == ch)
        {
            userEnroll(fd);
        }
        else if ('3' == ch)
        {
            return -1;
        }
        else
        {
            printf("输入错误，按任意键重新输入\n");
            getchar();
        }
    }
    return 0;
}

void printForCd(char* buf)
{
}

void printForPwd(char* buf)
{
    printf("%s\n", buf);
}

void printForPwd2(char* buf)
{
    strcpy(pwdStr,buf);
}

void printForLs(char* buf)
{   
    char fileSize[30];
    char fileType[4] = {0};
    char fileName[255] = {0};
    int offset = 0;
    while(sscanf(buf + offset, "%s %s %s", fileType, fileSize, fileName) != EOF)
    {
        if(!strcmp(fileType,"d"))
        {
            printf("\e[1;34m%s\033[0m\t",fileName);
            offset += 3 + strlen(fileType) + strlen(fileSize) + strlen(fileName);
        }
        else
        {
            printf("%s\t",fileName);
            offset += 3 + strlen(fileType) + strlen(fileSize) + strlen(fileName);
        }
    }
    printf("\n");
}

void printForRm(char* buf)
{
    printf("%s\n", buf);
}

void printForMkdir(char* buf)
{
    printf("%s\n",buf);
}
