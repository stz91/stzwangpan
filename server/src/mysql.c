#include "../include/factory.h"

int connectMYSQL(MYSQL **pConn)
{
    char *server = "localhost";
    char *user = "root";
    char *password = "123";
    char *database = "wangpan";
    *pConn = mysql_init(NULL);
    if (!mysql_real_connect(*pConn, server, user, password, database, 0, NULL, 0))
    {
        printf("Error connecting to database:%s", mysql_error(*pConn));
        return -1;
    }
    else
    {
        printf("connect database success\n");
    }
    return 0;
}
int queryMySQL(MYSQL *conn, char *queryInfo, char *resultInfo)
{
    int ret;
    unsigned queryNum;
    MYSQL_ROW row;
    MYSQL_RES *res;
    int flag = -2;
    ret = mysql_query(conn, queryInfo);
    if (ret)
    {
        sprintf(resultInfo, "%s %s", "Error making query", mysql_error(conn));
        return -1;
    }
    else
    {
        res = mysql_use_result(conn);
        if (res)
        {
            while ((row = mysql_fetch_row(res)) != NULL)
            {
                for (queryNum = 0; queryNum < mysql_num_fields(res); queryNum++)
                {
                    sprintf(resultInfo, "%s %s", resultInfo, row[queryNum]);
                }
                flag = 0;
            }
        }
        else
        {
            sprintf(resultInfo, "%s", "Don't find data");
        }
        mysql_free_result(res);
    }
    return flag;
}
int queryMySQLForRm(MYSQL *conn, char *queryInfo, char *resultInfo)
{
    int ret;
    unsigned queryNum;
    MYSQL_ROW row;
    MYSQL_RES *res;
    int flag = -2;
    ret = mysql_query(conn, queryInfo);
    if (ret)
    {
        sprintf(resultInfo, "%s%s", "Error making query", mysql_error(conn));
        return -1;
    }
    else
    {
        res = mysql_use_result(conn);
        if (res)
        {
            while ((row = mysql_fetch_row(res)) != NULL)
            {
                for (queryNum = 0; queryNum < mysql_num_fields(res); queryNum++)
                {
                    sprintf(resultInfo, "%s%s", resultInfo, row[queryNum]);
                }
                flag = 0;
            }
        }
        else
        {
            sprintf(resultInfo, "%s", "Don't find data");
        }
        mysql_free_result(res);
    }
    return flag;
}
int insertMySQL(MYSQL *conn, char *insertInfo, char *resultInfo)
{
    int ret;
    ret = mysql_query(conn, insertInfo);
    if (ret)
    {
        sprintf(resultInfo, "%s %s", "Error making insert", mysql_error(conn));
    }
    else
    {
        sprintf(resultInfo, "%s", "Insert success!");
    }
    return 0;
}
int deleteMySQL(MYSQL *conn, char *deleteInfo, char *resultInfo)
{
    int ret;
    ret = mysql_query(conn, deleteInfo);
    if (ret)
    {
        sprintf(resultInfo, "%s %s", "Error making delete", mysql_error(conn));
    }
    else
    {
        sprintf(resultInfo, "%s", "delete success!");
    }
    return 0;
}
int updateMySQL(MYSQL *conn, char *updateInfo, char *resultInfo)
{
    int ret;
    ret = mysql_query(conn, updateInfo);
    if(resultInfo==NULL)
    {
        return 0;
    }
    if (ret)
    {
        sprintf(resultInfo, "%s %s", "Error making update", mysql_error(conn));
    }
    else
    {
        sprintf(resultInfo, "%s", "update success!");
    }
    return 0;
}
/* 测试用例 */
/* int main()
{
    char insertInfo[100]="insert into student values (2,'xiaogang')";
    char queryInfo[100]="select * from student";
    char deleteInfo[100]="delete from student where ID=1";
    char updateInfo[100]="update student set ID=3 where ID=2";
    char resultInfo[100]={0};
    MYSQL *conn=NULL;
    connectMYSQL(&conn);
    //insertMySQL(conn,insertInfo,resultInfo);
    //queryMySQL(conn,queryInfo,resultInfo);
    //deleteMySQL(conn,deleteInfo,resultInfo);
    updateMySQL(conn,updateInfo,resultInfo);
    printf("%s\n",resultInfo);
    return 0;
}
*/

/*成功返回0，未能匹配或者超时返回-1*/
int checkToken(MYSQL *conn, char *token, int outTime)
{
    printf("\nin checkToken\n");
    char queryToken[200] = "select id from account where token=";
    sprintf(queryToken, "%s'%s'", queryToken, token);
    int ret;
    char lastOPTimeAndId[100] = {0};
    ret = queryMySQL(conn, queryToken, lastOPTimeAndId);
    if (-1 == ret)
    {
        printf("dont find the token\n");
        return -1;
    }
    //printf("in checkToken2\n");
    printf("\nout checkToken3\n");
    return 0;
}

int getUserNameAndCurDir(MYSQL *conn, char *token, char *userName, char *currentDirId)
{
    //printf("\n%d token = %s\n", __LINE__, token);
    char queryInfo[300] = "select name,currentDirID from account where token=";
    sprintf(queryInfo, "%s'%s'", queryInfo, token);
    char result[200] = {0};
    queryMySQL(conn, queryInfo, result);
    sscanf(result, "%s %s", userName, currentDirId);
    return 0;
}

int queryMySQLForUser(MYSQL *conn, char *queryInfo, User_t *pUser)
{
    int ret;
    MYSQL_ROW row;
    MYSQL_RES *res;
    ret = mysql_query(conn,queryInfo);
    if(ret)
    {
        return 0;
    }
    else
    {
        res = mysql_store_result(conn);
        if(res)
        {   
            if((row = mysql_fetch_row(res)) != NULL)
            {
                strcpy(pUser->id, row[0]);
                strcpy(pUser->name, row[1]);
                strcpy(pUser->salt, row[2]);
                strcpy(pUser->passwd, row[3]);
                strcpy(pUser->dirID, row[4]);
            }else{
                return -1;
            }
        }else{
            mysql_free_result(res);
            return -1;
        }
        mysql_free_result(res);
    }
    return 0;
}

int updateOPTime(MYSQL* conn, char* userID, time_t t)
{
    int ret;
    char updateInfo[200]="update account set lastOPTime = ";
    sprintf(updateInfo,"%s%ld where id = '%s'",updateInfo,t,userID);
    ret = mysql_query(conn, updateInfo);

#ifdef DEBUG_SERVER
    if(ret)
    {
        printf("error updateOPTime\n");
        return -1;
    }else{
        printf("success time update\n");
    }
#endif

    return 0;
}

int updateCurrentDir(MYSQL* conn,char* userName, char* currentDirId)
{
    char updateInfo[200]="update account set currentDirID=";
    sprintf(updateInfo,"%s%s %s'%s'", updateInfo, currentDirId, "where name=", userName);
    //printf("\ncurrentDirId=%s##\n", currentDirId);
    char result[100] = {0};
    updateMySQL(conn, updateInfo, result);
    return 0;
}

int getFileInfo(MYSQL* conn, char* fileName, char* currentDirId,char* result)
{
    char queryInfo[300] = "select type,md5,fileSize from file where clientName=";
    sprintf(queryInfo, "%s'%s' %s%s", queryInfo, fileName, "and prevID=", currentDirId);
    int ret;
    ret = queryMySQL(conn, queryInfo, result);
    if(0 == strlen(result) || -1 == ret)
    {
        return -1;
    }
    return 0;
}

int getUploadInfo(MYSQL* conn, size_t* pOffset, char* md5)
{
    char queryInfo[200]="select offset from upload where md5=";
    sprintf(queryInfo,"%s'%s'",queryInfo,md5);
    int ret;
    char result[200] = {0};
    ret = queryMySQL(conn, queryInfo,result);
    if(-1 == ret)
    {
        printf("发生错误\n");
        return -1;
    }else if(-2 == ret){
        printf("没有该消息\n");
        return -1;
    }else{
        sscanf(result,"%ld", pOffset);
        printf("查询成功,断点续传, 上传\n");
        return 0;
    }
}

int updateUploadInfo(MYSQL* conn, size_t offset, char* md5)
{
    char queryupload[300]="update upload set offset=";
    sprintf(queryupload,"%s%ld %s'%s'", queryupload,offset,"where md5=",md5);
    char result[200] = {0};
    int ret = updateMySQL(conn, queryupload,result);
    if(-1 == ret)
    {
        printf("update upload fail\n");
        return -1;
    }else{
        printf("update upload success\n");
        return 0;
    }
}

int insertUploadInfo(MYSQL* conn, size_t fileSize,size_t offset,char* md5)
{
    char insertUpload[300]="insert into upload(fileSize,offset,md5) values";
    sprintf(insertUpload,"%s(%ld,%ld,'%s')", insertUpload, fileSize,offset,md5);
    char result[200] = {0};
    int ret = insertMySQL(conn, insertUpload,result);
    if(-1 == ret)
    {
        printf("insert upload fail\n");
        return -1;
    }else{
        printf("insert upload success\n");
        return 0;
    }
}

int deleteUploadInfo(MYSQL* conn, char* md5)
{
    char deleteMd5[200]="delete from upload where md5=";
    sprintf(deleteMd5, "%s'%s'", deleteMd5, md5);
    char result[100] = {0};
    deleteMySQL(conn, deleteMd5, result);

#ifdef DEBUG_SERVER
    printf("\ndeleteUploadInfo\n");
    printf("result = %s\n", result);
#endif

    return 0;
}

void updateOPLog(MYSQL* conn,char* name,char* opStr)
{
    char updateOP[300] = "insert into opLog(userName,op) values";
    sprintf(updateOP, "%s('%s','%s')", updateOP, name, opStr);
    printf("updateOP = %s#\n", updateOP);
    char result[200] = {0};
    insertMySQL(conn, updateOP, result);
}