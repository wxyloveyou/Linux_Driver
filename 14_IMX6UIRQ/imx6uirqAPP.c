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
  ./TimerPP <filename>
 ./timerAPP /dev/tiemr 0 表示关灯

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
    unsigned int cmd;
    unsigned int arg;
    filename = argv[1];
    unsigned char str[100];
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
        printf("input CMD : ");
        ret = scanf("%d", &cmd);
        if (ret != 1) {
            gets(str); //防止卡死
        }
        if (cmd == 1) {         //关闭
            cmd = CLOSE_CMD;

        } else if (cmd == 2) {          //打开
            cmd = OPEN_CMD;
        } else if (cmd == 3) {          //设置周期
            cmd = SETPERIOD_CMD;
            printf("INPUT Timer Period : ");
            ret = scanf("%d", &arg);
            if (ret != 1) {			/* 参数输入错误 */
                gets(str);			/* 防止卡死 */
            }
        }
        ioctl(fd, cmd, arg);
    }
    close(fd);

    return 0;
}