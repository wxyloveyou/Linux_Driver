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
 ./LEDAPP <filename> <0:1> 0表示关灯,1表示开灯
 ./LEDAPP /dev/platled 0 表示关灯
 ./LEDAPP /dev/platled 1 表示开灯
*/
int main(int argc,char *argv[])
{
    int fd;
    char *filename;
    unsigned char datebuf[1];
    int ret = 0;
    filename = argv[1];
    if(argc != 3){
        printf("ERROR usage!\r\n");
        return -1;
    }
    fd =open(filename,O_RDWR);
    if (fd < 0)
    {
        printf("file %s is open fail\r\n", filename);
        return -1;
    }
    else
    {
        printf("file %s is successful!!!\r\n", filename);
    }
    datebuf[0] = atoi(argv[2]);         //将字符转化为数字
    ret = write(fd, datebuf, sizeof(datebuf));
    if (ret < 0)
    {
        printf("LED Control is fail \r\n");
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}