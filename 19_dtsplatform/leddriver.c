
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

#define GPIOLED_CNT 1
#define GPIOLED_NAME "dtsplatled"
#define LEDOFF 0
#define LEDON 1

/*gpioled 设备结构体*/
struct gpioled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *node;
    int led_gpio; /* led所使用的GPIO编号		*/
};
struct gpioled_dev gpioled;
static int gpioled_open(struct inode *inode, struct file *file)
{
    file->private_data = &gpioled; /* 设置私有数据 */
    return 0;
}
static int gpioled_release(struct inode *inode, struct file *file)
{

    return 0;
}
static ssize_t gpioled_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retval = 0;
    unsigned char databuf[1];
    struct gpioled_dev *dev = filp->private_data;
    retval = copy_from_user(databuf, buf, cnt);
    if (retval < 0)
    {
        printk("error\r\n");
        return -EFAULT;
    }
    if (databuf[0] == LEDON)
    {
        gpio_set_value(dev->led_gpio, 0);
    }
    else if (databuf[0] == LEDOFF)
    {
        gpio_set_value(dev->led_gpio, 1);
    }

    return 0;
}
static const struct file_operations gpioled_fops = {
    .owner = THIS_MODULE,
    .open = gpioled_open,
    .write = gpioled_write,
    .release = gpioled_release,
};

static int led_probe(struct platform_device *dev) {
    printk("led probe is runing \r\n");
    int ret = 0;
    /*注册字符设备驱动*/
    gpioled.major = 0;
    if (gpioled.major)
    {
        gpioled.devid = MKDEV(gpioled.major, 0);
        register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
    }
    printk("magor = %d,minor = %d\r\n", gpioled.major, gpioled.minor);
    /*初始化cdev*/
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &gpioled_fops);
    cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);
    /*创建节点*/
    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if (IS_ERR(gpioled.class))
    {
        return PTR_ERR(gpioled.class);
    }

    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if (IS_ERR(gpioled.device))
    {
        return PTR_ERR(gpioled.device);
    }
    /*获取节点*/
    #if 0
    gpioled.node = of_find_node_by_path("/gpioled");
    if (gpioled.node == NULL)
    {
        ret = -EINVAL;
        goto fail_findnode;
    }
    printk("asldjf\r\n");
#endif
    gpioled.node = dev->dev.of_node;
    /*获取led所对应的gpio*/
    gpioled.led_gpio = of_get_named_gpio(gpioled.node, "led-gpios", 0);
    if (gpioled.led_gpio < 0)
    {
        printk("error not find gpio \r\n");
        ret = -EINVAL;
    }
    printk("led gpio num = %d\r\n", gpioled.led_gpio);
    /*申请gpio*/
    ret = gpio_request(gpioled.led_gpio, "led-gpio");
    if (ret)
    {
        printk("error request gpio\r\n");
        ret = -EINVAL;
    }
    /*使用gpio,设置为输出，默认是高电平*/
    ret = gpio_direction_output(gpioled.led_gpio, 1); // 低电平点亮
    if (ret)
    {
        goto fail_set_output;
    }
    /*设置低电平*/
    gpio_set_value(gpioled.led_gpio, 0);

    return 0;
fail_set_output:
    gpio_free(gpioled.led_gpio);
fail_findnode:
    return ret;

    return 0;
}

static int led_remove(struct platform_device *dev) {
    printk("led remove is runing\r\n");
    /*关灯*/
    gpio_set_value(gpioled.led_gpio, 1);

    /*注销字符设备驱动*/
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
    printk(KERN_INFO "gpio_driver: Goodbye, cruel world!\n");
    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);
    /*释放gpio */
    gpio_free(gpioled.led_gpio);
    return 0;
}

struct of_device_id led_of_match[] = {
        {.compatible = "alientek,gpioled"},
        {},
};
static struct platform_driver leddriver = {
    .driver = {
        .name = "imx6ull-led",
        .of_match_table = led_of_match,},
    .probe = led_probe,
    .remove = led_remove,
};

static int __init leddriver_init(void) {
    return platform_driver_register(&leddriver);
}

static void __exit leddriver_exit(void) {
    platform_driver_unregister(&leddriver);
}

module_init(leddriver_init);

module_exit(leddriver_exit);

MODULE_AUTHOR("WXY");
MODULE_LICENSE("GPL");