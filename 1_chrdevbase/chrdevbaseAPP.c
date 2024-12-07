#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
/*
*argc :应用程序参数
*argv[] :具体的参数内容,字符串形似
 ./chrdevbaseAPP <filename> <1:2> 1表示读,2表示写
 ./chrdevbaseAPP /dev/chrdevbase 1 表示从驱动里面都数据
 ./chrdevbaseAPP /dev/chrdevbae 2 表示向驱动里面写数据
*/
int main(int argc,char *argv[]){
    int ret = 0;
    int fd = 0;
    char *filename;
    char readbuf[100];
    char writebuf[100];
    static char usrbuf[] = {"This is usrdata\r\n"};

    filename = argv[1];
    if (argc != 3)
    {
        printf("Error usage\r\n");
        return -1;
    }

    fd = open(filename, O_RDWR);
    if (fd < 0)
    {
        printf("can not open file %s\r\n",filename);
        return -1;
    }
  
    if (atoi(argv[2]) == 1)     //读操作
    {
        /*read*/
        ret = read(fd, readbuf, 50);
        if (ret < 0)
        {
            printf("read filw %s fail\r\n", filename);
            return -1;
        }else{
            printf("APP read date is %s\r\n",readbuf);
            printf("read data : %s\r\n", readbuf);
        }
    }

    if (atoi(argv[2]) == 2) // 写操作
    {
        /*write*/
        memcpy(writebuf,usrbuf,sizeof(usrbuf));
        ret = write(fd, writebuf, 50);
        if (ret < 0)
        {
            printf("write %s fial\r\n", filename);
        }
        else
        {

        }
    }
   
    /*close*/
    ret = close(fd);
    if (ret < 0)
    {
        printf("close %s fail\r\n",filename);
        return -1;
    }

    return 0;
}