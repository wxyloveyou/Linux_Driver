
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
#include <linux/miscdevice.h>

#define MISCBEEP_NAME "miscbeep"
#define MISCBEEP_MINOR 144
#define BEEP_ON     1
#define BEEP_OFF    0
struct miscbeep_dev {
    struct device_node *nd;
    int beep_gpio;
};
static struct miscbeep_dev miscbeepdev;

static int miscbeep_open(struct inode *inode, struct file *file) {
    file->private_data = &miscbeepdev;
    return 0;
}

static ssize_t miscbeep_write(struct file *file, const char __user *buf, size_t cnt, loff_t *ops) {
    int ret = 0;
    unsigned char databuf[1];
    struct miscbeep_dev *dev = file->private_data;
    ret = copy_from_user(databuf, buf, cnt);
    if (ret < 0) {
        return -EINVAL;
    }
    if (databuf[0] == BEEP_ON) {
        gpio_set_value(dev->beep_gpio, 0); //拉高，打開蜂鳴器BEEP
    } else if (databuf[0] == BEEP_OFF) {
        gpio_set_value(dev->beep_gpio, 1);           //拉低，关闭蜂鸣器
    }
    return 0;
}

static int miscbeep_release(struct inode *inode, struct file *file) {
    return 0;
}

/*字符设备操作集*/
static const struct file_operations miscbeep_fops = {
        .owner = THIS_MODULE,
        .open = miscbeep_open,
        .write = miscbeep_write,
        .release = miscbeep_release,
};
struct miscdevice beep_miscdev = {
        .minor = MISCBEEP_MINOR,
        .name = MISCBEEP_NAME,
        .fops =  &miscbeep_fops,
};


static int miscbeep_probe(struct platform_device *dev) {
    int ret = 0;
    /* 1、初始化蜂鸣器IO */
    miscbeepdev.nd = dev->dev.of_node;
    miscbeepdev.beep_gpio = of_get_named_gpio(miscbeepdev.nd, "beep-gpios", 0);
    if (miscbeepdev.beep_gpio < 0) {
        ret = -ENAVAIL;
        goto fail_findgpio;
    }
    ret = gpio_request(miscbeepdev.beep_gpio, "beep-gpios");
    if (ret) {
        printk("can not %d gpio\r\n ", miscbeepdev.beep_gpio);
        goto fail_findgpio;
    }

    ret = gpio_direction_output(miscbeepdev.beep_gpio, 1);       //默认输出高电平，
    if (ret < 0) {
        goto fail_setgpio;
    }
    /* 2、misc驱动注册 */;
    ret = misc_register(&beep_miscdev);
    if (ret < 0) {
        goto fail_setgpio;
    }
    return 0;
    fail_setgpio:
    gpio_free(miscbeepdev.beep_gpio);
    fail_findgpio:
    return ret;
}

static int miscbeep_remove(struct platform_device *dev) {
    gpio_set_value(miscbeepdev.beep_gpio, 1);       //拉高，关闭BEEP
    gpio_free(miscbeepdev.beep_gpio);
    misc_deregister(&beep_miscdev);
    return 0;
}

static struct of_device_id beep_of_match[] = {
        {.compatible = "alientek,beep"},
        {},
};

static struct platform_driver miscbeep_driver = {
        .driver = {
                .name = "alientek-beep",
                .of_match_table = beep_of_match,
        },
        .probe = miscbeep_probe,
        .remove = miscbeep_remove,
};

/*驱动入口函数*/
static int __init miscbeep_init(void) {
    return platform_driver_register(&miscbeep_driver);
}

static void __exit miscbeep_exit(void) {
    platform_driver_unregister(&miscbeep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);

MODULE_AUTHOR("WXY");
MODULE_LICENSE("GPL");
