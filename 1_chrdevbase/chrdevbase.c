#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>


#define CHRDEVBASE_MAJOR 200         // 主设备号
#define CHRDEVBASE_NAME "chrdevbase" // 名字

static char readbuf[100];
static char writebuf[100];
static char kerneldata[] = {"kernel data"};

static int chrdevbase_open(struct inode *inode, struct file *file)
{
    // printk("chrdevbase_open\r\n");

    return 0;
}


static int chrdevbase_release(struct inode *inode, struct file *file)
{
    // printk("chrdevbase_release\r\n");
    return 0;
}
/*
*   返回的数据存储在buf里面，count要读取的字节
*/
static ssize_t chrdevbase_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)     
{
    // printk("chrdevbase_read\r\n");
    int ret = 0;
    /* 向用户空间发送数据 */
    memcpy(readbuf, kerneldata, sizeof(kerneldata));
    ret = copy_to_user(buf, readbuf, count);
    if (ret == 0)
    {
        printk("kernel senddata ok!\r\n");
    }else{
        printk("kernel senddata fail!\r\n");
    }
    
    return 0;
}

static ssize_t chrdevbase_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
//     printk("chrdevbase_write\r\n");
    int ret = 0;
    ret = copy_from_user(writebuf, buf, count);
    if (ret == 0)
    {
         printk("kernel recevdate:%s\r\n",writebuf);
    }

    return 0;
}

/*
 * 设备操作函数结构体
 */
static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
    .release = chrdevbase_release,
};

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 0 成功;其他 失败
 */
static int __init chrdevbase_init(void)
{
    int ret = 0;
    printk("hello chrdevbase_init\r\n");
    /*注册字符设备*/
    ret = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
    if (ret < 0)
    {
        printk("chrdevbase init failed\r\n");
    }
    printk("chrdevbase init!\r\n");
    return 0;
}
 /*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit chrdevbase_exit(void)
{
    /* 注销字符设备驱动 */
    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
    printk("chrdevbase exit!\r\n");
}

/*
 * 将上面两个函数指定为驱动的入口和出口函数
 */
module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");
