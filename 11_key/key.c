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
#include <linux/semaphore.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define KEY_CNT			1		/* 设备号个数 	*/
#define KEY_NAME		"key"	/* 名字 		*/

/* 定义按键值 */
#define KEY0VALUE		0XF0	/* 按键值 		*/
#define INVAKEY			0X00	/* 无效的按键值  */

struct key_dev {
    dev_t devid;                //设备号
    struct cdev cdev;           //cdev
    struct class *class;        //类
    struct device *device;      //设备
    int major;
    int minor;
    struct device_node *node;   //设备节点
    int key_gpio;               //key使用的GPIO编号
    atomic_t keyvalue;          //按键值
};
struct key_dev keyDev;          //key设备

static int key_open(struct inode *inode,struct file *file){
    file->private_data = &keyDev;
    return 0;
}
static ssize_t key_read(struct file *file,char __user *buf,size_t count,loff_t *offt){
    int value;
    struct key_dev *dev = file->private_data;
    int ret = 0;
    if (gpio_get_value(dev->key_gpio) == 0) {   //按下
        while(!gpio_get_value(dev->key_gpio));
        atomic_set(&dev->keyvalue,KEY0VALUE);
    }else{
        atomic_set(&dev->keyvalue, INVAKEY);
    }
    value = atomic_read(&dev->keyvalue);
    ret = copy_to_user(buf, &value, sizeof(value));
    return ret;
}
static int key_write(struct file *file,const char __user *buf,size_t count, loff_t *offt){
    return 0;
}
static int key_release(struct inode *inode,struct file *file){
    return 0;
}

/*设备操作函数*/
static struct file_operations key_fops = {
        .owner = THIS_MODULE,
        .open = key_open,
        .read = key_read,
        .write = key_write,
        .release = key_release,
};
/*key io初始化*/
static int keyio_init(struct key_dev *dev){
    int ret = 0;
    /*初始化atomic_t*/
    atomic_set(&keyDev.keyvalue,INVAKEY);
    dev->node = of_find_node_by_path("/key");
    if (dev->node == NULL) {
        ret = -EINVAL;
        goto fail_node;
    }
    dev->key_gpio = of_get_named_gpio(dev->node, "key-gpios", 0);
    if (dev->key_gpio < 0) {
        ret = -EINVAL;
        goto fail_gpio;
    }
    ret = gpio_request(dev->key_gpio, "key");
    if (ret) {
        ret = -EINVAL;
        printk("IO %d can't request\r\n ", dev->key_gpio);
        goto fail_request;
    }
    ret = gpio_direction_input(dev->key_gpio);
    if (ret < 0) {
        ret = -EINVAL;
        goto fail_input;
    }
    return 0;
    fail_input:
    gpio_free(dev->key_gpio);
    fail_request:
    fail_gpio:
    fail_node:
    return ret;

}
static int __init mykey_init(void)
{
    int ret = 0;
    atomic_set(&keyDev.keyvalue,INVAKEY);
    /*注册字符设备驱动*/
    //1、创建设备号
    keyDev.major = 0;
    if (keyDev.major) {
        keyDev.devid = MKDEV(keyDev.major, 0);
        ret = register_chrdev_region(keyDev.devid, KEY_CNT, KEY_NAME);
    }else{
        alloc_chrdev_region(&keyDev.devid, 0, KEY_CNT, KEY_NAME);
        keyDev.major = MAJOR(keyDev.devid);
        keyDev.minor = MINOR(keyDev.devid);
    }
    if (ret < 0) {
        goto fail_devid;
    }
    printk("major = %d minor = %d\r\n", keyDev.major, keyDev.minor);
    //2、初始化cdev
    keyDev.cdev.owner = THIS_MODULE;
    cdev_init(&keyDev.cdev, &key_fops);
    //3、添加一个cdev
    ret = cdev_add(&keyDev.cdev, keyDev.devid, KEY_CNT);
    if (ret) {
        goto fail_cdevadd;
    }
    //4、创建类
    keyDev.class = class_create(THIS_MODULE, KEY_NAME);
    if (IS_ERR(keyDev.class)) {
        ret = PTR_ERR(keyDev.class);
        goto fail_class;
    }
    //5、创建设备
    keyDev.device = device_create(keyDev.class, NULL, keyDev.devid, NULL, KEY_NAME);
    if (IS_ERR(keyDev.device)) {
        ret = PTR_ERR(keyDev.device);
        goto fail_device;
    }
    ret = keyio_init(&keyDev);
    if (ret < 0) {
        goto fail_device;
    }
    return 0;
    fail_device:
    class_destroy(keyDev.class);        //销毁类
    fail_class:
    cdev_del(&keyDev.cdev);    //删除cdev
    fail_cdevadd:
    unregister_chrdev_region(keyDev.devid, KEY_CNT);    //注销设备号
    fail_devid:
        return 0;
}
static void __exit mykey_exit(void)
{

    cdev_del(&keyDev.cdev);    //删除cdev
    unregister_chrdev_region(keyDev.devid, KEY_CNT);    //注销设备号
    device_destroy(keyDev.class, keyDev.devid); //销毁设备
    class_destroy(keyDev.class);        //销毁类
    gpio_free(keyDev.key_gpio);     //释放申请的GPIO
}
module_init(mykey_init);
module_exit(mykey_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");