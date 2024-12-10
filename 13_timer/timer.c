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



#define TIMER_CNT       1
#define TIMER_NAME      "timer"

#define CLOSE_CMD           _IO(0xEF,1)        //关闭命令
#define OPEN_CMD            _IO(0xEF,2)        //打开命令
#define SETPERIOD_CMD       _IO(0xEF,3)       //设置周期


struct timer_dev {
    dev_t devid;                //设备号
    struct cdev cdev;           //cdev
    struct class *class;        //类
    struct device *device;      //设备
    int major;
    int minor;
    struct device_node *node;   //设备节点
    struct timer_list timer;     //定时器
    int timerperiod;              //定时器周期ms
    int led_gpio;
};
struct timer_dev timerdev;          //tiemrdev

/*定时器处理函数*/
static void timer_func(unsigned long arg ){
    struct timer_dev *dev = (struct timer_dev *)arg;
    static int sta = 1;
    sta = !sta;
    gpio_set_value(dev->led_gpio, sta);
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timerperiod));
}

static int timer_open(struct inode *inode,struct file *file){
    file->private_data = &timerdev;
    return 0;
}

static int timer_release(struct inode *inode,struct file *file){
    return 0;
}
static long timer_ioctl(struct file *file,unsigned int cmd,unsigned long arg){
    struct timer_dev *dev = (struct timer_dev *)file->private_data;
    int value = 0;
    int ret = 0;
    switch (cmd) {
        case CLOSE_CMD:
            del_timer_sync(&dev->timer);
            break;
        case OPEN_CMD:
            mod_timer(&dev->timer, jiffies+ msecs_to_jiffies(dev->timerperiod));
            break;
        case SETPERIOD_CMD:

            if (ret < 0) {
                return -EINVAL;
            }
            dev->timerperiod = arg;
            printk("arg is :%d\r\n", arg);
            mod_timer(&dev->timer, jiffies+ msecs_to_jiffies(dev->timerperiod));
            break;
    }
    return 0;
}
/*设备操作函数*/
static struct file_operations timerdev_fops = {
        .owner = THIS_MODULE,
        .open = timer_open,
        .unlocked_ioctl = timer_ioctl,
        .release = timer_release,
};

/*初始化led灯*/
static int led_init(struct timer_dev *dev){
    int ret = 0;
    dev->node = of_find_node_by_path("/gpioled");
    if (dev->node == NULL) {
        ret = -EINVAL;
        goto fail_find;
    }
    dev->led_gpio = of_get_named_gpio(dev->node, "led-gpios", 0);
    if (dev->led_gpio < 0) {
        ret = -EINVAL;
        goto fail_gpio;
    }
    ret = gpio_request(dev->led_gpio, "led");
    if (ret) {
        ret = -EINVAL;
        printk("request led gpio is fial....\r\n");
        goto fail_request;
    }
    ret = gpio_direction_output(dev->led_gpio, 1);    //设置输出，默认高，关灯
    if (ret < 0) {
        ret = -EINVAL;
        goto fail_gpioset;
    }
    fail_gpioset:
    gpio_free(dev->led_gpio);
    fail_request:
    fail_gpio:
    fail_find:

    return ret;
}

static int __init tiemr_init(void)
{
    int ret = 0;

    /*注册字符设备驱动*/
    //1、创建设备号
    timerdev.major = 0;
    if (timerdev.major) {
        timerdev.devid = MKDEV(timerdev.major, 0);
        ret = register_chrdev_region(timerdev.devid, TIMER_CNT, TIMER_NAME);
    }else{
        alloc_chrdev_region(&timerdev.devid, 0, TIMER_CNT, TIMER_NAME);
        timerdev.major = MAJOR(timerdev.devid);
        timerdev.minor = MINOR(timerdev.devid);
    }
    if (ret < 0) {
        goto fail_devid;
    }
    printk("major = %d minor = %d\r\n", timerdev.major, timerdev.minor);
    //2、初始化cdev
    timerdev.cdev.owner = THIS_MODULE;
    cdev_init(&timerdev.cdev, &timerdev_fops);
    //3、添加一个cdev
    ret = cdev_add(&timerdev.cdev, timerdev.devid,TIMER_CNT);
    if (ret) {
        goto fail_cdevadd;
    }
    //4、创建类
    timerdev.class = class_create(THIS_MODULE, TIMER_NAME);
    if (IS_ERR(timerdev.class)) {
        ret = PTR_ERR(timerdev.class);
        goto fail_class;
    }
    //5、创建设备
    timerdev.device = device_create(timerdev.class, NULL, timerdev.devid, NULL, TIMER_NAME);
    if (IS_ERR(timerdev.device)) {
        ret = PTR_ERR(timerdev.device);
        goto fail_device;
    }
    /*6、初始化led*/
    ret = led_init(&timerdev);
    if (ret < 0) {
        goto fail_ledinit;
    }
    /*6、初始化定时器*/
    init_timer(&timerdev.timer);
    timerdev.timerperiod = 500;
    timerdev.timer.function = timer_func;
    timerdev.timer.expires = jiffies + msecs_to_jiffies(500);
    timerdev.timer.data = (unsigned long)&timerdev;
    add_timer(&timerdev.timer);



    return 0;
    fail_ledinit:
    fail_device:
    class_destroy(timerdev.class);        //销毁类
    fail_class:
    cdev_del(&timerdev.cdev);    //删除cdev
    fail_cdevadd:
    unregister_chrdev_region(timerdev.devid, TIMER_CNT);    //注销设备号
    fail_devid:
        return ret;
}
static void __exit timer_exit(void)
{
    //关灯
    gpio_set_value(timerdev.led_gpio, 1);
    /*删除定时器*/
    del_timer(&timerdev.timer);
    cdev_del(&timerdev.cdev);    //删除cdev
    unregister_chrdev_region(timerdev.devid, TIMER_CNT);    //注销设备号
    device_destroy(timerdev.class, timerdev.devid); //销毁设备
    class_destroy(timerdev.class);        //销毁类
    //释放IO
    gpio_free(timerdev.led_gpio);
}
module_init(tiemr_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");