/*************************************************************************
*File Name: remove.c
*Author: tz_shi
*657923857@qq.com
*Created Time: 2019年07月18日 星期四 11时25分01秒
 ************************************************************************/
#include"func.h"

int main(int argc,char* argv[])
{
    int ret=remove("../../bin/file3");
    ERROR_CHECK(ret,-1,"remove");
    return 0;
}

