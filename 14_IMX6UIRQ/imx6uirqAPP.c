#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "sys/ioctl.h"
/*
*argc :应用程序参数
*argv[] :具体的参数内容,字符串形似
  ./imx6uirqAPP <filename>
 ./imx6uirqAPP /dev/imx6uirq 0 表示关灯

*/
#define KEY0VALUE		0XF0	/* 按键值 		*/
#define INVAKEY			0X00	/* 无效的按键值  */

#define CLOSE_CMD           _IO(0xEF,1)        //关闭命令
#define OPEN_CMD            _IO(0xEF,2)        //打开命令
#define SETPERIOD_CMD       _IO(0xEF,3)       //设置周期

int main(int argc,char *argv[])
{

    int fd, ret;
    char *filename;
    unsigned char data;
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


    while(1){
        ret = read(fd, &data, sizeof(data));
        if (ret < 0) {

        }else {
            if (data) {
                printf("key value is %#x\r\n", data);
            }
        }
    }
    close(fd);

    return 0;
}