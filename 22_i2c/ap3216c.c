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
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "ap3216creg.h"

#define AP3216C_CNT 1
#define AP3216C_NAME "ap3216c"

struct ap3216c_dev {
    struct cdev cdev;
    dev_t devid;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    void *private_data;         // 私有数据
    unsigned short ir, als, ps; // 三个传感器数据
};
static struct ap3216c_dev ap3216cdev;

/**
 * 向ap3216c多个寄存器写入数据
 * dev:
 */
static s32 ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg, u8 *buf, u8 len) {
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *) dev->private_data;
    b[0] = reg;         //寄存器首地址
    memcpy(&b[1], buf, len); // 将要写入的数据考入数组b中，
    msg.addr = client->addr;    //ap3216c地址
    msg.flags = 0;      //标记为写数据
    msg.buf = b;
    msg.len = len + 1;  //写入的长度
    return i2c_transfer(client->adapter, &msg, 1);

}

/**
 * 向指定寄存器写入值，写单个寄存器
 * dev: i2c设备
 * reg:指定寄存器值
 * data:写入的值
 */
static void ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data) {
    u8 buf = 0;
    buf = data;
    ap3216c_write_regs(dev, reg, &buf, 1);
}

static int ap3216c_open(struct inode *inode, struct file *file) {
    file->private_data = &ap3216cdev;
    /* 初始化ap3216c */
    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x04);     //复位AP3216
    mdelay(50);                                                 /* AP3216C复位最少10ms 	*/
    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x03);   //   /* 开启ALS、PS+IR 		*/
    return 0;
}
/**
 * 从ap3216c中读取多个寄存器数据
 * @param dev：ap3216c设备
 * @param reg：要读取的寄存器首地址
 * @param val：读取到的数据值
 * @param len：要读取的数据长度
 * @return：操作结果
 */
static int ap3216c_read_regs(struct ap3216c_dev *dev,u8 reg ,void *val,int len)
{
    int ret = 0;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    /*msg[0]为发送要读取的首地址*/
    msg[0].addr = client->addr;     //ap3216c地址
    msg[0].flags = 0;               //标记为发送数据
    msg[0].buf = &reg;              //读取的首地址
    msg[0].len = 1;                 //reg长度
    /*msg[1]为读取数据*/
    msg[1].addr = client->addr;     //ap3216c地址
    msg[1].flags = I2C_M_RD;        //标记为读取数据
    msg[1].buf = val;               //读取数据缓冲区
    msg[1].len = len;               //要读取的长度
    ret = i2c_transfer(client->adapter, msg, 2);
    if(ret == 2) {
        ret = 0;
    } else {
        printk("i2c read is faild\r\n");
        printk("i2c read failed=%d reg=%06x len=%d\n",ret, reg, len);
        ret = -EREMOTEIO;
    }
    return ret;
}
/**
 * 读取ap3216指定寄存器数据，读取一个寄存器
 * @param dev  ap3216设备
 * @param reg   要读取的寄存器
 * @return  读取到的数据
 */
static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev,u8 reg)
{
    u8 data = 0;
    ap3216c_read_regs(dev, reg, &data, 1);
    return data;
}
/*
 * @description	: 读取AP3216C的数据，读取原始数据，包括ALS,PS和IR, 注意！
 *				: 如果同时打开ALS,IR+PS的话两次数据读取的时间间隔要大于112.5ms
 * @param - ir	: ir数据
 * @param - ps 	: ps数据
 * @param - ps 	: als数据
 * @return 		: 无。
 */
static void ap3216c_readdata(struct ap3216c_dev *dev) {
    unsigned char i = 0;
    unsigned char buf[6];
    for ( i = 0; i < 6; ++i) {
        buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);
    }

    if(buf[0] & 0X80) 	/* IR_OF位为1,则数据无效 */
        dev->ir = 0;
    else 				/* 读取IR传感器的数据   		*/
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03);

    dev->als = ((unsigned short)buf[3] << 8) | buf[2];	/* 读取ALS传感器的数据 			 */

    if(buf[4] & 0x40)	/* IR_OF位为1,则数据无效 			*/
        dev->ps = 0;
    else 				/* 读取PS传感器的数据    */
        dev->ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F);

}

/*
 * @description		: 从设备读取数据
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t ap3216c_read(struct file *file, char __user *buf, size_t cnt, loff_t *ops) {
    short data[3];
    long err = 0;
    struct ap3216c_dev *dev = (struct ap3216c_dev *) file->private_data;
    ap3216c_readdata(dev);
    data[0] = dev->ir;
    data[1] = dev->als;
    data[2] = dev->ps;
    err = copy_to_user(buf, data, sizeof(data));
    return 0;
};

static int ap3216c_release(struct inode *inode, struct file *file) {
    return 0;
}

static const struct file_operations ap3216c_ops =
        {
                .owner = THIS_MODULE,
                .read = ap3216c_read,
                .open = ap3216c_open,
                .release = ap3216c_release,
        };

/*
 * @description     : i2c驱动的probe函数，当驱动与
 *                    设备匹配以后此函数就会执行
 * @param - client  : i2c设备
 * @param - id      : i2c设备ID
 * @return          : 0，成功;其他负值,失败
 */
static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    int ret = 0;
    /* 1、构建设备号 */
    if (ap3216cdev.major) {
        ap3216cdev.devid = MKDEV(ap3216cdev.major, 0);
        ret = register_chrdev_region(ap3216cdev.devid, AP3216C_CNT, AP3216C_NAME);
    } else {
        ret = alloc_chrdev_region(&ap3216cdev.devid, 0, AP3216C_CNT, AP3216C_NAME);
        ap3216cdev.major = MAJOR(ap3216cdev.major);
    }
    if (ret < 0) {
        goto fail_reg;
    }
    printk("This Major is = %d\r\n", ap3216cdev.major);
    /* 2、注册设备 */
    cdev_init(&ap3216cdev.cdev, &ap3216c_ops);
    cdev_add(&ap3216cdev.cdev, ap3216cdev.devid, AP3216C_CNT);
    ap3216cdev.class = class_create(THIS_MODULE, AP3216C_NAME);
    if (IS_ERR(ap3216cdev.class)) {
        return PTR_ERR(ap3216cdev.class);
    }
    ap3216cdev.device = device_create(ap3216cdev.class, NULL, ap3216cdev.devid, NULL, AP3216C_NAME);
    if (IS_ERR(ap3216cdev.device)) {
        return PTR_ERR(ap3216cdev.device);
    }
    ap3216cdev.private_data = client;

    return 0;
    fail_reg:
    return ret;
}

static int ap3216c_remove(struct i2c_client *client) {
    cdev_del(&ap3216cdev.cdev);
    unregister_chrdev_region(ap3216cdev.devid, AP3216C_CNT);
    device_destroy(ap3216cdev.class, ap3216cdev.devid);
    class_destroy(ap3216cdev.class);
    return 0;
}

/* 传统匹配方式ID列表 */
static const struct i2c_device_id ap3216c_id[] = {
        {"alientek,ap3216c", 0},
        {},
};
/* 设备树匹配列表 */
static const struct of_device_id ap3216c_of_match[] = {
        {.compatible = "alientek,ap3216c"},
        {/* Sentinel */},
};
static struct i2c_driver ap3216c_driver = {
        .probe = ap3216c_probe,
        .remove = ap3216c_remove,
        .driver = {
                .owner = THIS_MODULE,
                .name = "ap3216",
                .of_match_table = ap3216c_of_match,
        },
        .id_table = ap3216c_id,
};

static int __init ap3216c_init(void) {
    return i2c_add_driver(&ap3216c_driver);
}

static void __exit ap3216c_exit(void) {
    i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_AUTHOR("WXY");
MODULE_LICENSE("GPL");
