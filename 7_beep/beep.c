#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>

#define BEEPOFF 0 // 关闭led
#define BEEPON 1  // 打开beep
#define beep_CNT     1
#define beep_NAME    "beep"

struct beep_dev {
        dev_t devid;        //主设备号
        struct cdev cdev;   //字符设备
        struct class *class;    //类
        struct device *devicd;  //设备
        int major ;
        int minor;
        struct  device_node *node;
        int beep_gpio;   //beep所使用的GPIO
};
static struct beep_dev beep;

static int beep_open(struct inode *inode,struct file *file)
{
    file->private_data = &beep;
    return 0;
}
static int beep_release(struct inode *inode,struct file *file)
{
    return 0;
}
static int beep_write(struct file *file,const char __user *buf,size_t count,loff_t *offt)
{
    unsigned char databuf[1];
    int ret = 0;
    struct beep_dev *dev = file->private_data;
    ret = copy_from_user(databuf, buf, count);
    if (ret < 0) {
        return -EINVAL;
    }
    if (databuf[0] == BEEPON) {
        gpio_set_value(dev->beep_gpio,0);
    }else if (databuf[0] == BEEPOFF){
        gpio_set_value(dev->beep_gpio,1);
    }

    return 0;
}

static const struct file_operations beep_fops = {
        .owner = THIS_MODULE,
        .open = beep_open,
        .release = beep_release,
        .write = beep_write,
};

static int __init beep_init(void)
{
    int ret = 0;
    printk("hell beep_init\r\n");
    /*注册字符设备驱动*/
    beep.major = 0;
    if (beep.major) {
        beep.devid = MKDEV(beep.major, 0);
        ret = register_chrdev_region(beep.devid, beep_CNT, beep_NAME);
    } else{
        ret = alloc_chrdev_region(&beep.devid, 0, beep_CNT, beep_NAME);
        beep.major = MAJOR(beep.devid);
        beep.minor = MINOR(beep.devid);
    }
    if (ret < 0) {
        goto fail_devid;
    }
    printk("major = %d,minor = %d\r\n", beep.major, beep.minor);
    /*初始化cdev*/
    beep.cdev.owner = THIS_MODULE;
    cdev_init(&beep.cdev, &beep_fops);
    ret = cdev_add(&beep.cdev, beep.devid, beep_CNT);
    if (ret) {
        goto fail_cdevadd;
    }
    /*创建节点*/
    beep.class = class_create(THIS_MODULE, beep_NAME);
    if (IS_ERR(beep.class)) {
        ret = PTR_ERR(beep.class);
        goto fail_class;
    }
    beep.devicd = device_create(beep.class, NULL, beep.devid, NULL, beep_NAME);
    if (IS_ERR(beep.devicd)) {
        ret = PTR_ERR(beep.devicd);
        goto fail_devicd;
    }
    /*获取节点*/
    beep.node = of_find_node_by_path("/beep");
    if (beep.node == NULL) {
     ret = -EINVAL;
     goto fail_nd;
    }
    beep.beep_gpio = of_get_named_gpio(beep.node, "beep-gpios", 0);
    if (beep.beep_gpio < 0) {
        ret = -EINVAL;
        goto fail_nd;
    }
    ret = gpio_request(beep.beep_gpio, "beep-gpios");
    if (ret) {
        printk("can requese gpio\r\n");
        goto fail_nd;
    }
    ret = gpio_direction_output(beep.beep_gpio, 1);
    if (ret < 0) {
        goto fail_set;
    }
    return 0;

//    gpio_set_value(beep.led_gpio, 0);
    fail_set:
    gpio_free(beep.beep_gpio);
    fail_nd:
    device_destroy(beep.class, beep.devid);
    fail_devicd:
        class_destroy(beep.class);
    fail_class:
        cdev_del(&beep.cdev);
    fail_cdevadd:
        unregister_chrdev_region(beep.devid, beep_CNT);
    fail_devid:
    return ret;

}
static void __exit beep_exit(void)
{
    gpio_set_value(beep.beep_gpio,1);
    unregister_chrdev_region(beep.devid, beep_CNT);
    cdev_del(&beep.cdev);
    device_destroy(beep.class, beep.devid);
    class_destroy(beep.class);
    gpio_free(beep.beep_gpio);
}
module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");
