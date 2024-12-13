#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>


#define IMX6UIRQ_CNT            1
#define IMX6UIRQ_NAME           "imx6uirq"

/*imx6uirq设备结构体*/
struct imx6uirq_dev {
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
};
struct imx6uirq_dev imx6uirqdev;

static int imx6uirq_open(struct inode *inode, struct file *file){
    file->private_data = &imx6uirqdev;
    return 0;
}
static ssize_t imx6uirq_read(struct file *file,char __user *buf,size_t cnt,loff_t *offt){
    return 0;
}

static int imx6uirq_release(struct inode *inode, struct file *file){
    return 0;
}
/*操作类*/
static const struct file_operations imx6uirq_fops = {
        .owner   = THIS_MODULE,
        .read    = imx6uirq_read,
        .release = imx6uirq_release
};

static int __init imx6uirq_init(void){
    int ret = 0;
    imx6uirqdev.major = 0;
    if (imx6uirqdev.major) {
        imx6uirqdev.devid = MKDEV(imx6uirqdev.major, 0);
        ret = register_chrdev_region(imx6uirqdev.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
    }else{
        ret = alloc_chrdev_region(&imx6uirqdev.devid, 0, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
        imx6uirqdev.major = MAJOR(imx6uirqdev.devid);
        imx6uirqdev.minor = MINOR(imx6uirqdev.devid);
    }
    if (ret < 0) {
        goto fail_devid;
    }
    printk("major is %d , minor is %d\r\n", imx6uirqdev.major, imx6uirqdev.minor);

    imx6uirqdev.cdev.owner = THIS_MODULE;
    cdev_init(&imx6uirqdev.cdev, &imx6uirq_fops);
    ret = cdev_add(&imx6uirqdev.cdev, imx6uirqdev.devid, IMX6UIRQ_CNT);
    if (ret) {
        goto fail_cdevadd;
    }
    imx6uirqdev.class = class_create(THIS_MODULE, IMX6UIRQ_NAME);
    if (IS_ERR(imx6uirqdev.class)) {
        ret = PTR_ERR(imx6uirqdev.class);
        goto fail_class;
    }
    imx6uirqdev.device = device_create(imx6uirqdev.class, NULL, imx6uirqdev.devid, NULL, IMX6UIRQ_NAME);
    if (IS_ERR(imx6uirqdev.device)) {
        ret = PTR_ERR(imx6uirqdev.device);
        goto fail_device;
    }
    return 0;
    fail_device:
    class_destroy(imx6uirqdev.class);
    fail_class:
    cdev_del(&imx6uirqdev.cdev);
    fail_cdevadd:
    unregister_chrdev_region(imx6uirqdev.devid, IMX6UIRQ_CNT);
    fail_devid:
    return ret;
}
static void __exit imx6uirq_exit(void){
    /* 注销字符设备驱动 */
    cdev_del(&imx6uirqdev.cdev);
    unregister_chrdev_region(imx6uirqdev.devid, IMX6UIRQ_CNT);

    device_destroy(imx6uirqdev.class, imx6uirqdev.devid);
    class_destroy(imx6uirqdev.class);

}
module_init(imx6uirq_init);
module_exit(imx6uirq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");