#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include "linux/ioctl.h"
#include "signal.h"
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
static int fd = 0;  //文件描述符
static void sigio_signal_func(int num)
{
    printf("sigio_signal_func\r\n");
    int err;
    unsigned char keyvalue = 0;
    err = read(fd,&keyvalue,sizeof(keyvalue) );
    if (err < 0)
    {      
    }else {
        printf("sigio signal ! keyvalue is %d\r\n",keyvalue);
    }
}
int main(int argc,char *argv[])
{

    int  ret;
    char *filename;
    unsigned char data;
    int flags = 0;
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

    printf("test\r\n");
    
   /* 设置信号处理函数 */
    signal(SIGIO, sigio_signal_func);
    fcntl(fd, F_SETOWN, getpid());                  /* 设置当前进程接收SIGIO信号 	*/
    flags = fcntl(fd,F_GETFL);      //获取当前的进程状态
    printf("flags = %d\r\n", flags);
    // fcntl(fd,F_SETFL,flags | FASYNC);     //设置进程启用异步通知
    fcntl(fd, F_SETFL, flags | FASYNC); /* 设置进程启用异步通知功能 	*/
    while (1)

        ;
    close(fd);

    return 0;
}