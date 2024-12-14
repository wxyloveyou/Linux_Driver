#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "sys/ioctl.h"
#include "sys/select.h"
#include "poll.h"
/*
*argc :应用程序参数
*argv[] :具体的参数内容,字符串形似
  ./imx6uirqAPP <filename>
 ./imx6uirqAPP /dev/imx6uirq 0 表示关灯

*/
#define KEY0VALUE 0XF0 /* 按键值 		*/
#define INVAKEY 0X00   /* 无效的按键值  */

#define CLOSE_CMD _IO(0xEF, 1)     // 关闭命令
#define OPEN_CMD _IO(0xEF, 2)      // 打开命令
#define SETPERIOD_CMD _IO(0xEF, 3) // 设置周期

int main(int argc, char *argv[])
{
    // fd_set readfds;
    // struct timeval timeout;
    struct pollfd fds;

    int fd, ret;
    char *filename;
    unsigned char data;

    if (argc != 2)
    {
        printf("ERROR usage!\r\n");
        return -1;
    }
    filename = argv[1];
    fd = open(filename, O_RDWR | O_NONBLOCK); // 以非阻塞方式打开文件
    if (fd < 0)
    {
        printf("file %s is open fail\r\n", filename);
        return -1;
    }
    else
    {
        printf("file %s is successful!!!\r\n", filename);
    }
#if 0

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0; // 1s timeout
        ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
        switch (ret)
        {
        case 0: // 超时
           printf("select timeout\r\n");
            break;

        case -1: // 错误
            break;
        default: // 可以读取数据
            if (FD_ISSET(fd, &readfds))
            {
                ret = read(fd, &data, sizeof(data));
                if (ret < 0)
                {

                }
                else
                {
                    if (data)
                    {
                        printf("key value is %#x\r\n", data);
                    }
                }
            }
            break;
        }
    }
#endif

    while (1)
    {
        fds.fd = fd;
        fds.events = POLLIN;
        ret = poll(&fds, 1, 500);
        if (ret == 0) // 超时
        {
            printf("poll timeout\r\n");
        }
        else if (ret < 0) // 错误
        {
        }
        else        // 可以读取
        { 
            if (fds.revents & POLLIN)
            {
                ret = read(fd, &data, sizeof(data));
                if (ret < 0)
                {
                }
                else
                {
                    if (data)
                    {
                        printf("key value is %#x\r\n", data);
                    }
                }
            }
        }
    }
    close(fd);

    return 0;
}