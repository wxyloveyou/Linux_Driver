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
#include <linux/spi/spi.h>
#include "icm20608reg.h"

#define ICM20608_CNT 1
#define ICM20608_NAME "icm20608"

struct icm20608_dev {
    dev_t devid;                /* 设备号 	 */
    struct cdev cdev;            /* cdev 	*/
    struct class *class;        /* 类 		*/
    struct device *device;        /* 设备 	 */
    struct device_node *nd;    /* 设备节点 */
    int major;                    /* 主设备号 */
    void *private_data;            /* 私有数据 		*/
    signed int gyro_x_adc;        /* 陀螺仪X轴原始值 	 */
    signed int gyro_y_adc;        /* 陀螺仪Y轴原始值		*/
    signed int gyro_z_adc;        /* 陀螺仪Z轴原始值 		*/
    signed int accel_x_adc;        /* 加速度计X轴原始值 	*/
    signed int accel_y_adc;        /* 加速度计Y轴原始值	*/
    signed int accel_z_adc;        /* 加速度计Z轴原始值 	*/
    signed int temp_adc;        /* 温度原始值 			*/
};

static struct icm20608_dev icm20608Dev;





/*
 * @description	: 从icm20608读取多个寄存器数据
 * @param - dev:  icm20608设备
 * @param - reg:  要读取的寄存器首地址
 * @param - val:  读取到的数据
 * @param - len:  要读取的数据长度
 * @return 		: 操作结果
 */
static int icm20608_read_regs(struct icm20608_dev *dev,u8 reg,void *buf,int len)
{
    int ret = -1;
    unsigned char txdata[1];
    unsigned char *rxdata;
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;
    t = kzalloc(sizeof(struct spi_transfer),GFP_KERNEL);
    if (!t)
    {
        return -ENOMEM;
    }
    rxdata = kzalloc(sizeof(char) * len, GFP_KERNEL);	/* 申请内存 */
    if(!rxdata) {
        goto out1;
    }
    /* 一共发送len+1个字节的数据，第一个字节为寄存器首地址，一共要读取len个字节长度的数据，*/
    txdata[0] = reg | 0x80;                 //写数据的时候，寄存器首地址bit8要置1
    t->rx_buf = rxdata;                     //要发送的数据
    t->tx_buf = txdata;                     //要读取的数据
    t->len = len +1;                        //t->len = 发送的数据长度+读取的数据长度
    spi_message_init(&m);                   //初始化
    spi_message_add_tail(t, &m);            // 将spi_transfer添加到spi_message队列
    ret = spi_sync(spi, &m);        //同步发送
    if (ret)
    {
        goto out2;
    }
    out2:
    kfree(rxdata);
    out1:
    kfree(t);
    return ret;
}

static s32 icm20608_write_regs(struct icm20608_dev *dev, u8 reg, u8 *buf, u8 len) {
    int ret = -1;
    unsigned char txdata[1];
    unsigned char *rxdata;
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *) dev->private_data;
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);            //申请内存
    if (!t) {
        return -ENOMEM;
    }
    rxdata = kzalloc(sizeof(char) * len, GFP_KERNEL);
    if (!rxdata) {
        goto out1;
    }
    /*一共发送len+1个字节的数据，第一个字 节为寄存器首地址，一共要读取len个字节长度的数据*/
    txdata[0] = reg | 0;                //写数据时首寄存器地址bit8要置为1
    t->tx_buf = txdata;                      //要发送的数据
    t->rx_buf = rxdata;                 //要读取的数据
    t->len = len + 1;                /* t->len=发送的长度+读取的长度 */
    spi_message_init(&m);            //初始化spi_message
    spi_message_add_tail(t, &m);        /* 将spi_transfer添加到spi_message队列 */
    ret = spi_sync(spi, &m);        //同步发送
    if (ret) {
        goto out2;
    }
    memcpy(buf , rxdata+1, len);  /* 只需要读取的数据 */
    out2:
    kfree(rxdata);					/* 释放内存 */
    out1:
    kfree(t);						/* 释放内存 */

    return ret;
}

static unsigned char icm20608_read_onereg(struct icm20608_dev *dev,u8 reg)
{
    u8 data = 0;
    icm20608_read_regs(dev, reg, &data, 1);
    return data;
}

static void icm20608_write_onereg(struct icm20608_dev *dev, u8 reg, u8 value) {
    u8 buf = value;
    icm20608_write_regs(dev, reg, &buf, 1);
}
static void icm20608_readdata(struct icm20608_dev *dev)
{
    unsigned char data[14] = {0};
    icm20608_read_regs(dev, ICM20_ACCEL_XOUT_H, data, 14);
    dev->accel_x_adc = (signed short) ((data[0] << 8) | data[1]);
    dev->accel_y_adc = (signed short) ((data[2] << 8) | data[3]);
    dev->accel_z_adc = (signed short) ((data[4] << 8) | data[5]);
    dev->temp_adc = (signed short) ((data[6] << 8) | data[7]);
    dev->gyro_x_adc = (signed short) ((data[8] << 8) | data[9]);
    dev->gyro_y_adc = (signed short) ((data[10] << 8) | data[11]);
    dev->gyro_z_adc = (signed short) ((data[12] << 8) | data[13]);
}


static int icm20608_open(struct inode *inode,struct file *file)
{
    file->private_data = &icm20608Dev;
    return 0;
}
static ssize_t icm20608_read(struct file *file, char __user *buf, size_t cnt, loff_t *off){
    signed int data[7];
    long err = 0;
    struct icm20608_dev *dev = (struct icm20608_dev *)file->private_data;
    icm20608_readdata(dev);
    data[0] = dev->gyro_x_adc;
    data[1] = dev->gyro_y_adc;
    data[2] = dev->gyro_z_adc;
    data[3] = dev->accel_x_adc;
    data[4] = dev->accel_y_adc;
    data[5] = dev->accel_z_adc;
    data[6] = dev->temp_adc;
    err = copy_to_user(buf, data, sizeof(data));
    return 0;
}

static int icm20608_release(struct inode *inode, struct file *file) {
    return 0;
}

static const struct file_operations icm20608Dev_ops = {

        .owner = THIS_MODULE,
        .open = icm20608_open,
        .read = icm20608_read,
        .release = icm20608_release,
};

void icm20608_reginit(void) {
    u8 value = 0;
    icm20608_write_onereg(&icm20608Dev, ICM20_PWR_MGMT_1, 0x80);
    mdelay(50);
    icm20608_write_onereg(&icm20608Dev, ICM20_PWR_MGMT_1, 0x01);
    mdelay(50);
    value = icm20608_read_onereg(&icm20608Dev, ICM20_WHO_AM_I);
    printk("icm20608 ID = %x\r\n", value);

    icm20608_write_onereg(&icm20608Dev, ICM20_SMPLRT_DIV, 0x00); 	/* 输出速率是内部采样率					*/
    icm20608_write_onereg(&icm20608Dev, ICM20_GYRO_CONFIG, 0x18); 	/* 陀螺仪±2000dps量程 				*/
    icm20608_write_onereg(&icm20608Dev, ICM20_ACCEL_CONFIG, 0x18); 	/* 加速度计±16G量程 					*/
    icm20608_write_onereg(&icm20608Dev, ICM20_CONFIG, 0x04); 		/* 陀螺仪低通滤波BW=20Hz 				*/
    icm20608_write_onereg(&icm20608Dev, ICM20_ACCEL_CONFIG2, 0x04); /* 加速度计低通滤波BW=21.2Hz 			*/
    icm20608_write_onereg(&icm20608Dev, ICM20_PWR_MGMT_2, 0x00); 	/* 打开加速度计和陀螺仪所有轴 				*/
    icm20608_write_onereg(&icm20608Dev, ICM20_LP_MODE_CFG, 0x00); 	/* 关闭低功耗 						*/
    icm20608_write_onereg(&icm20608Dev, ICM20_FIFO_EN, 0x00);		/* 关闭FIFO						*/

}
static int icm20608_probe(struct spi_device *spi) {

    if (icm20608Dev.major) {
        icm20608Dev.devid = MKDEV(icm20608Dev.major, 0);
        register_chrdev_region(icm20608Dev.devid, ICM20608_CNT, ICM20608_NAME);
    } else {
        alloc_chrdev_region(&icm20608Dev.devid, 0, ICM20608_CNT, ICM20608_NAME);
        icm20608Dev.major = MAJOR(icm20608Dev.devid);
    }
    printk("icm20608 major is %d \r\n", icm20608Dev.major);
    cdev_init(&icm20608Dev.cdev, &icm20608Dev_ops);
    cdev_add(&icm20608Dev.cdev, icm20608Dev.devid, ICM20608_CNT);

    icm20608Dev.class = class_create(THIS_MODULE, ICM20608_NAME);
    if (IS_ERR(icm20608Dev.class)) {
        return PTR_ERR(icm20608Dev.class);
    }
    icm20608Dev.device = device_create(icm20608Dev.class, NULL, icm20608Dev.devid, NULL, ICM20608_NAME);
    if (IS_ERR(icm20608Dev.device)) {
        return PTR_ERR(icm20608Dev.device);
    }

    /*初始化spi_device */
    spi->mode = SPI_MODE_0;    /*MODE0，CPOL=0，CPHA=0*/
    spi_setup(spi);
    icm20608Dev.private_data = spi; /* 设置私有数据 */

    /* 初始化ICM20608内部寄存器 */
    icm20608_reginit();
    return 0;
}

static int icm20608_remove(struct spi_device *spi) {
    /* 删除设备 */
    cdev_del(&icm20608Dev.cdev);
    unregister_chrdev_region(icm20608Dev.devid, ICM20608_CNT);

/* 注销掉类和设备 */
    device_destroy(icm20608Dev.class, icm20608Dev.devid);
    class_destroy(icm20608Dev.class);
    return 0;
}

/* 传统匹配方式ID列表 */
static const struct spi_device_id icm20608_id[] = {
        {"alientek,icm20608", 0},
        {}
};
/*设备树匹配列表*/
static const struct of_device_id icm20608_of_match[] = {
        {.compatible = "alientek,icm20608"},
        {/*Sentienl*/},
};
static struct spi_driver icm20608_driver = {
        .probe = icm20608_probe,
        .remove = icm20608_remove,
        .driver  ={
                .owner = THIS_MODULE,
                .name = "icm20608",
                .of_match_table = icm20608_of_match,
        },
        .id_table = icm20608_id,
};

static int __init icm20608_init(void) {
    return spi_register_driver(&icm20608_driver);
}

static void __exit icm20608_exit(void) {
    spi_unregister_driver(&icm20608_driver);
}

module_init(icm20608_init);
module_exit(icm20608_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");
