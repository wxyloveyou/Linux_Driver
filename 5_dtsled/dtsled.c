#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#define LEDOFF 0 // 关闭led
#define LEDON 1  // 打开led
#define DTSLED_CNT 1
#define DTSLED_NAME "dtsled"

/*地址映射后的虚拟地址指针*/
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

struct dtsled_dev
{

    dev_t devid;           // 主设备号
    struct cdev cdev;      // 字符设备
    struct class *class;   // 类;
    struct device *device; // 设备
    int major;             // 主设备号
    int minor;
    struct device_node *nd; // 设备节点
};
struct dtsled_dev dtsled;
/*LED打开或者关闭*/
static void led_switch(unsigned char sta)
{
    unsigned int val = 0;
    printk("sta %d\n", sta);
    val = readl(GPIO1_DR);
    if (sta == LEDON)
    {
        val &= ~(1 << 3); // bit3清零，打开led灯
    }
    else if (sta == LEDOFF)
    {
        val |= (1 << 3); // bit3置一，关闭led灯
    }
    writel(val, GPIO1_DR);
}
static int dtsled_open(struct inode *inode, struct file *file)
{
    file->private_data = &dtsled;
    return 0;
}
static int dtled_release(struct inode *inode, struct file *file)
{
    struct dtsled_dev *dev = (struct dtsled_dev *)file->private_data;
    return 0;
}
static int dtsled_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    struct dtsled_dev *dev = (struct dtsled_dev *)file->private_data;
    int retvalue = 0;
    unsigned char databuf[1];
    retvalue = copy_from_user(databuf, buf, count);
    if (retvalue < 0)
    {
        printk("kernel write fail!\r\n");
        return -EFAULT;
    }
    led_switch(databuf[0]);

    return 0;
}

static const struct file_operations dtsled_fops = {
    .owner = THIS_MODULE,
    .open = dtsled_open,
    .release = dtled_release,
    .write = dtsled_write,
};

static int __init dtsled_init(void)
{
    int ret = 0;
    const char *str;
    u32 regdata[10];
    u8 i = 0;
    unsigned int val = 0;
    /*注册字符设备*/
    /*1.申请设备号*/
    dtsled.major = 0;
    if (dtsled.major) // 表示定义了主设备号
    {
        dtsled.devid = MKDEV(dtsled.major, 0);
        ret = register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
   
    }
    else 
    {
        alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }
    if (ret < 0)
    {
        goto fail_devid;
    }
    /*2.添加字符设备*/
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);
    ret = cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);
    if (ret < 0)
    {
        goto fail_cdev;
    }
    /*自动创建设备节点*/
    dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
    if (IS_ERR(dtsled.class))
    {
        ret = PTR_ERR(dtsled.class);
        goto fail_class;
    }
    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
    if (IS_ERR(dtsled.device))
    {
        ret = PTR_ERR(dtsled.device);
        goto fail_device;
    }
    /*获取设备数属性内容*/
    dtsled.nd = of_find_node_by_path("/alphaled");
    if (dtsled.nd == NULL)
    {
        ret = -EINVAL;
        goto fail_findnd;
    }
    /*获取设备属性*/
    ret = of_property_read_string(dtsled.nd, "status", &str);
    if (ret < 0)
    {
        goto fail_rs;
    }
    else
    {
        printk("status: %s\n", str);
    }
    ret = of_property_read_string(dtsled.nd, "compatible", &str);
    if (ret < 0)
    {
        goto fail_rs;
    }
    else
    {
        printk("status: %s\n", str);
    }
#if 0
    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
    if (ret < 0)
    {
        goto fail_rs;
    }
    else
    {
        for (i = 0; i < 10; i++)
        {
            printk("reg[%d]: %#X\r\n", i, regdata[i]);
        }
    }
    /*led灯初始化*/
    /*1、初始化LED*/

    IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
    SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
    SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
    GPIO1_DR = ioremap(regdata[6], regdata[7]);
    GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
#endif
    IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0);
    SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1);
    SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2);
    GPIO1_DR = of_iomap(dtsled.nd, 3);
    GPIO1_GDIR = of_iomap(dtsled.nd, 4);

    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26); // 先清楚以前的配置bit26 bit 27
    val |= (3 << 26);  // bit 26 bit 27置一
    writel(val, IMX6U_CCM_CCGR1);

    writel(0x5, SW_MUX_GPIO1_IO03);    // 设置复用
    writel(0x10B0, SW_PAD_GPIO1_IO03); // 设置电气属性

    val = readl(GPIO1_GDIR);
    val |= (1 << 3); // bit3置一，设置为输出
    writel(val, GPIO1_GDIR);

    val = readl(GPIO1_DR);
    val |= (1 << 3); // bit3设置为1 ，关闭led灯
    writel(val, GPIO1_DR);

    return 0;
fail_rs:
fail_findnd:
    device_destroy(dtsled.class, dtsled.devid);
fail_device:
    class_destroy(dtsled.class);
fail_class:
    cdev_del(&dtsled.cdev);
fail_cdev:
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
fail_devid:
    return ret;
}
static void __exit dtsled_exit(void)
{
    unsigned int val = 0;
    val = readl(GPIO1_DR);
    val |= (1 << 3);
    writel(val, GPIO1_DR);

    /*取消地址映射*/
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    /*删除字符设备*/
    cdev_del(&dtsled.cdev);
    /*释放设备号*/
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
    /*删除设备节点*/
    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);

}

module_init(dtsled_init);
module_exit(dtsled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");
