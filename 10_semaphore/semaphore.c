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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/atomic.h>

#define GPIOLED_CNT     1
#define GPIOLED_NAME    "gpioled"
#define LEDOFF          0
#define LEDON           1
/*gpioled 设备结构体*/
struct gpioled_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *node;
    int led_gpio; /* led所使用的GPIO编号		*/
    struct mutex lock;      //互斥体
};
struct gpioled_dev gpioled;

static int gpioled_open(struct inode *inode, struct file *file) {

    file->private_data = &gpioled; /* 设置私有数据 */
    mutex_lock(&gpioled.lock); /*获取互斥体*/


    return 0;
}

static int gpioled_release(struct inode *inode, struct file *file) {

    struct gpioled_dev *dev = file->private_data;
    mutex_unlock(&gpioled.lock);               /*释放互斥体*/
    return 0;
}

static ssize_t gpioled_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt) {
    int retval = 0;
    unsigned char databuf[1];
    struct gpioled_dev *dev = filp->private_data;
    retval = copy_from_user(databuf, buf, cnt);
    if (retval < 0) {
        printk("error\r\n");
        return -EFAULT;
    }
    if (databuf[0] == LEDON) {
        gpio_set_value(dev->led_gpio, 0);
    } else if (databuf[0] == LEDOFF) {
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


static int __init gpio_init(void) {
    int ret = 0;
    /*初始化互斥体*/
    mutex_init(&gpioled.lock);
    /*注册字符设备驱动*/
    gpioled.major = 0;
    if (gpioled.major) {
        gpioled.devid = MKDEV(gpioled.major, 0);
        register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    } else {
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
    if (IS_ERR(gpioled.class)) {
        return PTR_ERR(gpioled.class);
    }

    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if (IS_ERR(gpioled.device)) {
        return PTR_ERR(gpioled.device);
    }
    /*获取节点*/
    gpioled.node = of_find_node_by_path("/gpioled");
    if (gpioled.node == NULL) {
        ret = -EINVAL;
        goto fail_findnode;
    }
    /*获取led所对应的gpio*/
    gpioled.led_gpio = of_get_named_gpio(gpioled.node, "led-gpios", 0);
    if (gpioled.led_gpio < 0) {
        printk("error not find gpio \r\n");
        ret = -EINVAL;

    }
    printk("led gpio num = %d\r\n", gpioled.led_gpio);
    /*申请gpio*/
    ret = gpio_request(gpioled.led_gpio, "led-gpios");
    if (ret) {
        printk("error request gpio\r\n");
        ret = -EINVAL;
    }
    /*使用gpio,设置为输出，默认是高电平*/
    ret = gpio_direction_output(gpioled.led_gpio, 1); // 低电平点亮
    if (ret) {
        goto fail_set_output;
    }
    /*设置低电平*/
    gpio_set_value(gpioled.led_gpio, 0);

    return 0;
    fail_set_output:
    gpio_free(gpioled.led_gpio);
    fail_findnode:
    return ret;
}

static void __exit gpio_exit(void) {
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
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");