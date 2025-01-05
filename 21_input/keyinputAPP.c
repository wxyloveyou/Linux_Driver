#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "sys/ioctl.h"
#include "linux/input.h"
/*
*argc :应用程序参数
*argv[] :具体的参数内容,字符串形似
  ./keyinputAPP <filename>
 ./keyinputAPP /dev/keyinput/event1 0 表示关灯

*/
#define KEY0VALUE		0XF0	/* 按键值 		*/
#define INVAKEY			0X00	/* 无效的按键值  */

#define CLOSE_CMD           _IO(0xEF,1)        //关闭命令
#define OPEN_CMD            _IO(0xEF,2)        //打开命令
#define SETPERIOD_CMD       _IO(0xEF,3)       //设置周期

struct input_event inputevent;


int main(int argc,char *argv[])
{

    int fd, err;
    char *filename;
    unsigned char data;

    if(argc != 2){
        printf("ERROR usage!\r\n");
        return -1;
    }
    filename = argv[1];
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
    while (1) {

        err = read(fd, &inputevent, sizeof(inputevent));
        if (err > 0) {      //数据读取成功
            switch (inputevent.type) {
                case EV_KEY:
                    if (inputevent.code < BTN_MISC) {  //key
                        printf("key %d %s\r\n", inputevent.code,inputevent.value ? "press":"release");
                    }else {
                        printf("button %d %s\r\n", inputevent.code,inputevent.value ? "press":"release");
                    }
                    //printf("EV_EVK 事件\r\n");
                    break;
                case EV_SYN:
                   // printf("EV_SYS 事件\r\n");
                    break;
                case EV_REL:
                    break;
                case EV_ABS:
                    break;
            }
        }else {
            printf("读取数据失败\r\n");
        }
    }
    close(fd);

    return 0;
}