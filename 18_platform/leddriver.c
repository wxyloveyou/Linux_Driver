
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define PLATFORM_CNT			1		  	/* 设备号个数 */
#define PLATFORM_NAME			"platled"	/* 名字 */
#define LEDOFF 					0			/* 关灯 */
#define LEDON 					1			/* 开灯 */

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;


/* newchrled设备结构体 */
struct newchrled_dev{
    dev_t devid;			/* 设备号 	 */
    struct cdev cdev;		/* cdev 	*/
    struct class *class;		/* 类 		*/
    struct device *device;	/* 设备 	 */
    int major;				/* 主设备号	  */
    int minor;				/* 次设备号   */
};
struct newchrled_dev newchrled;	/* led设备 */
void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON) {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_DR);
    }else if(sta == LEDOFF) {
        val = readl(GPIO1_DR);
        val|= (1 << 3);
        writel(val, GPIO1_DR);
    }
}

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &newchrled; /* 设置私有数据 */
    return 0;
}

/*
 * @description		: 从设备读取数据
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

/*
 * @description		: 向设备写数据
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;

    retvalue = copy_from_user(databuf, buf, cnt);
    if(retvalue < 0) {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];		/* 获取状态值 */

    if(ledstat == LEDON) {
        led_switch(LEDON);		/* 打开LED灯 */
    } else if(ledstat == LEDOFF) {
        led_switch(LEDOFF);	/* 关闭LED灯 */
    }
    return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* 设备操作函数 */
static struct file_operations newchrled_fops = {
        .owner = THIS_MODULE,
        .open = led_open,
        .read = led_read,
        .write = led_write,
        .release = 	led_release,
};

static int led_probe(struct platform_device *dev)
{
    struct resource *ledresource[5];
    int i = 0;
    int ret = 0;
    unsigned int val = 0;
     printk("led_probe is runing\r\n");
    /*初始化led,字符设备驱动，*/
    // 1、从设备中获取资源
    for (i = 0; i < 5; i++)
    {
        ledresource[i] = platform_get_resource(dev, IORESOURCE_MEM, i);
        if (ledresource[i] == NULL) 
        {
           return -EINVAL;
        }
//        resource_size()
        
    }

    /* 1、寄存器地址映射 */
    IMX6U_CCM_CCGR1 = ioremap(ledresource[0]->start, resource_size(ledresource[0]));
    SW_MUX_GPIO1_IO03 = ioremap(ledresource[1]->start, resource_size(ledresource[1]));
    SW_PAD_GPIO1_IO03 = ioremap(ledresource[2]->start, resource_size(ledresource[2]));
    GPIO1_DR = ioremap(ledresource[3]->start, resource_size(ledresource[3]));
    GPIO1_GDIR = ioremap(ledresource[4]->start, resource_size(ledresource[4]));

    /* 2、使能GPIO1时钟 */
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);	/* 清楚以前的设置 */
    val |= (3 << 26);	/* 设置新值 */
    writel(val, IMX6U_CCM_CCGR1);

    /* 3、设置GPIO1_IO03的复用功能，将其复用为
     *    GPIO1_IO03，最后设置IO属性。
     */
    writel(5, SW_MUX_GPIO1_IO03);

    /*寄存器SW_PAD_GPIO1_IO03设置IO属性
     *bit 16:0 HYS关闭
     *bit [15:14]: 00 默认下拉
     *bit [13]: 0 kepper功能
     *bit [12]: 1 pull/keeper使能
     *bit [11]: 0 关闭开路输出
     *bit [7:6]: 10 速度100Mhz
     *bit [5:3]: 110 R0/6驱动能力
     *bit [0]: 0 低转换率
     */
    writel(0x10B0, SW_PAD_GPIO1_IO03);

    /* 4、设置GPIO1_IO03为输出功能 */
    val = readl(GPIO1_GDIR);
    val &= ~(1 << 3);	/* 清除以前的设置 */
    val |= (1 << 3);	/* 设置为输出 */
    writel(val, GPIO1_GDIR);

    /* 5、默认关闭LED */
    val = readl(GPIO1_DR);
    val |= (1 << 3);
    writel(val, GPIO1_DR);

    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    if (newchrled.major) {		/*  定义了设备号 */
        newchrled.devid = MKDEV(newchrled.major, 0);
        ret = register_chrdev_region(newchrled.devid, PLATFORM_CNT, PLATFORM_NAME);
    } else {						/* 没有定义设备号 */
        ret = alloc_chrdev_region(&newchrled.devid, 0, PLATFORM_CNT, PLATFORM_NAME);	/* 申请设备号 */
        newchrled.major = MAJOR(newchrled.devid);	/* 获取分配号的主设备号 */
        newchrled.minor = MINOR(newchrled.devid);	/* 获取分配号的次设备号 */
    }
    printk("newcheled major=%d,minor=%d\r\n",newchrled.major, newchrled.minor);
    if (ret < 0) {
        ret = -ENAVAIL;
        goto fail_error;
    }
    /* 2、初始化cdev */
    newchrled.cdev.owner = THIS_MODULE;
    cdev_init(&newchrled.cdev, &newchrled_fops);

    /* 3、添加一个cdev */
    ret = cdev_add(&newchrled.cdev, newchrled.devid, PLATFORM_CNT);
    if (ret < 0) {
        ret = -ENAVAIL;
        goto fail_error;
    }
    /* 4、创建类 */
    newchrled.class = class_create(THIS_MODULE, PLATFORM_NAME);
    if (IS_ERR(newchrled.class)) {
        return PTR_ERR(newchrled.class);
    }

    /* 5、创建设备 */
    newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, PLATFORM_NAME);
    if (IS_ERR(newchrled.device)) {
        return PTR_ERR(newchrled.device);
    }

    return 0;
    fail_error:
    return ret;

}
static int led_remove(struct platform_device *dev)
{
    unsigned int val = 0;
    printk("led remove is runing\r\n");

    val = readl(GPIO1_DR);
    val |=(1 << 3);
    writel(val,GPIO1_DR);

    /*1.删除字符设备*/
    cdev_del(&newchrled.cdev);
    /*2.注销设备号*/
    unregister_chrdev_region(newchrled.devid,PLATFORM_CNT);
    /*3.摧毁设备*/
    device_destroy(newchrled.class,newchrled.devid);
    /*4.摧毁类*/
    class_destroy(newchrled.class);
    return 0;
}
static struct platform_driver leddriver = {
    .driver = {
        .name = "imx6ull-led", // 驱动名字用于和设备名字相同
    },
    .probe = led_probe,
    .remove = led_remove,

};

/*设备加载  */
static int __init leddriver_init(void)
{
    int ret = 0;
    ret = platform_driver_register(&leddriver);

    return ret;
}
static void __exit leddriver_exit(void)
{
    platform_driver_unregister(&leddriver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);

MODULE_AUTHOR("wxy");
MODULE_LICENSE("GPL");