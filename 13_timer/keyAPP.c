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
 ./KEYAPP <filename>
 ./KEYAPP /dev/key 0 表示关灯

*/
#define KEY0VALUE		0XF0	/* 按键值 		*/
#define INVAKEY			0X00	/* 无效的按键值  */

int main(int argc,char *argv[])
{
    int value = 0;
    int fd;
    char *filename;
    unsigned char datebuf[1];
    int ret = 0;
    filename = argv[1];
    if(argc != 2){
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

    /*循环读取按键值*/
    while(1){
        read(fd, &value, sizeof(value));
        if (value == KEY0VALUE) {
            printf("key0 press value is %x\r\n", value);
        }
       // printf("key0 value is %d\r\n", value);
       // sleep(5);
    }
    close(fd);

    return 0;
}