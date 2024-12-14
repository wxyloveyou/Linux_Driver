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
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/wait.h>
#include <linux/poll.h>


#define IMX6UIRQ_CNT 1
#define IMX6UIRQ_NAME "imx6uirq"
#define KEY_NUM 1
#define KEY0VALUE 0x01
#define INVAKEY 0xFF
/*key结构体*/
struct irq_keydesc
{
    int gpio;                            // io编号
    int irqnum;                          // 中断号
    unsigned char value;                 //  按键值
    char name[10];                       // 名字
    irqreturn_t (*handler)(int, void *); // 中断处理函数
    struct tasklet_struct tasklet;
};

/*imx6uirq设备结构体*/
struct imx6uirq_dev
{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    struct irq_keydesc irqkey[KEY_NUM];
    struct timer_list timer;

    atomic_t keyvalue;
    atomic_t releasekey;

    wait_queue_head_t r_wait; // 读等待队列头
};
struct imx6uirq_dev imx6uirqdev;

static int imx6uirq_open(struct inode *inode, struct file *file)
{

    file->private_data = &imx6uirqdev;
    return 0;
}
static ssize_t imx6uirq_read(struct file *file, char __user *buf, size_t cnt, loff_t *offt)
{
    int ret = 0;
    unsigned char keyvalue;
    unsigned char releasekey;
    struct imx6uirq_dev *dev = file->private_data;

    if (file->f_flags & O_NONBLOCK) // 非阻塞模式
    {                                          
        if (atomic_read(&dev->releasekey) == 0) // 没有按下按键
        {
            return -EAGAIN;
        }
    }
    else
    {                                            // 阻塞模式
        /* 等待事件 ,若第二个参数不为真,则一直阻塞,不执行后面的代码,为真则唤醒*/
        wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey)); // 等待按键值有效
    }

#if 0
    /* 等待事件 ,若第二个参数不为真,则一直阻塞,不执行后面的代码,为真则唤醒*/
    wait_event_interruptible(dev->r_wait,atomic_read(&dev->releasekey));      //等待按键值有效
#endif
#if 0
    DECLARE_WAITQUEUE(wait, current);       //定义一个等待队列项
    if (atomic_read(&dev->releasekey) == 0)     //按键没有按下
    {
        add_wait_queue(&dev->r_wait, &wait);        //将队列项添加到等待队列头
        __set_current_state(TASK_INTERRUPTIBLE);    //设置进程状态为可打断状态
        schedule();                             //切换;


        /* 唤醒之后从这里开始运行 */
        if (signal_pending(current))
        {
            ret = -ERESTARTSYS;
            goto data_error;
        }
        __set_current_state(TASK_RUNNING);      // 将当前进程状态设置为运行状态
        remove_wait_queue(&dev->r_wait, &wait); // 将对应的队列项从等待队列头中删除
    }

#endif

    keyvalue = atomic_read(&dev->keyvalue);
    releasekey = atomic_read(&dev->releasekey);
    if (releasekey)
    { // 有效的按键
        if (keyvalue & 0x80)
        {
            //            printk("read : keyvalue is = %#x\r\n", keyvalue);
            keyvalue &= ~0x80;
            //            printk("read : keyvalue & ~0x80 is = %#x\r\n", keyvalue);

            ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
        }
        else
        {
            goto data_error;
        }
        atomic_set(&dev->releasekey, 0);
    }
    else
    {
        goto data_error;
    }
data_error:
#if 0
    __set_current_state(TASK_RUNNING);      // 将当前进程状态设置为运行状态
    remove_wait_queue(&dev->r_wait, &wait); // 将对应的队列项从等待队列头中删除
#endif
    return ret;
}

static int imx6uirq_release(struct inode *inode, struct file *file)
{
    return 0;
}
static unsigned int imx6uirq_poll(struct file * file, struct poll_table_struct * wait)
{
    int mask = 0;

    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)file->private_data;
    poll_wait(file, &dev->r_wait, wait);
    /* 判断是否可读 */
    if (atomic_read(&dev->releasekey))     //按键按下可读
    {
        mask = POLL_IN | POLLRDNORM;                //返回POLLIN   可读
    }
    
    return mask;
}
/*操作类*/
static const struct file_operations imx6uirq_fops = {
    .owner = THIS_MODULE,
    .open = imx6uirq_open,
    .read = imx6uirq_read,
    .release = imx6uirq_release,
    .poll = imx6uirq_poll,
};

/**
 * timer_fun - 定时器中断服务例程
 * @arg: 传递给定时器例程的参数，这里预期是一个设备结构体的地址
 *
 * 此函数在定时器到期时被调用，它检查指定GPIO引脚的电平状态
 * 如果按键被按下（GPIO电平为低），则更新设备的keyvalue值为按键的值
 * 如果按键被释放（GPIO电平为高），则更新keyvalue值，并设置释放标志
 */
static void timer_fun(unsigned long arg)
{
    int value;
    // 将传入的参数转换为设备结构体指针
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)arg;

    // 获取GPIO引脚的电平状态
    value = gpio_get_value(dev->irqkey[0].gpio);

    // 检查GPIO电平状态
    if (value == 0)
    { // 按下
        // 当按键被按下时，更新设备的keyvalue值为按键的值
        atomic_set(&dev->keyvalue, dev->irqkey[0].value);
        //        printk("KEY0 push\r\n");
    }
    else if (value == 1)
    {
        //        printk("KEY0 release\r\n");
        // 当按键被释放时，更新keyvalue值，并设置释放标志
        atomic_set(&dev->keyvalue, ((dev->irqkey[0].value) | 0x80));
        // printk("value = %#x \r\n",dev->irqkey[0].value);
        atomic_set(&dev->releasekey, 1); /* 完成的按键过程 */
    }
    /* 唤醒等待的进程 */
    if (atomic_read(&dev->releasekey))
    {
        wake_up(&dev->r_wait);
    }
}

/*按键中断处理函数*/
static irqreturn_t key0_handler(int irq, void *dev_id)
{

    struct imx6uirq_dev *dev = dev_id;
    printk("key0_handler is runing\r\n");
    tasklet_schedule(&dev->irqkey[0].tasklet);
    return IRQ_HANDLED;
}
static void key_tasklet(unsigned long data)
{
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)data;
    printk("key_tasklet is runing\r\n");
    dev->timer.data = data;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10)); /*10ms定时*/
}

/*按键初始化*/
static int keyio_init(struct imx6uirq_dev *dev)
{
    int ret = 0;
    int i = 0;
    /*1、按键初始化*/
    dev->node = of_find_node_by_path("/key");
    if (imx6uirqdev.node == NULL)
    {
        ret = -EINVAL;
        goto fail_node;
    }
    for (i = 0; i < KEY_NUM; ++i)
    {
        dev->irqkey[i].gpio = of_get_named_gpio(dev->node, "key-gpios", i);
    }
    for (i = 0; i < KEY_NUM; ++i)
    {
        memset(dev->irqkey[i].name, 0, sizeof(dev->irqkey[i].name));
        sprintf(dev->irqkey[i].name, "KEY%d", i);
        gpio_request(dev->irqkey[i].gpio, dev->irqkey[i].name);
        gpio_direction_input(dev->irqkey[i].gpio);

        dev->irqkey[i].irqnum = gpio_to_irq(dev->irqkey[i].gpio); /*获取中断号*/
#if 0
        dev->irqkey[i].irqnum = irq_of_parse_and_map(dev->node, i);
#endif
    }
    /*2、按键中断初始化*/
    dev->irqkey[0].handler = key0_handler;
    dev->irqkey[0].value = KEY0VALUE;
    // dev->irqkey[0].tasklet = key_tasklet;
    for (i = 0; i < KEY_NUM; ++i)
    {
        ret = request_irq(dev->irqkey[i].irqnum, dev->irqkey[i].handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                          dev->irqkey[i].name, &imx6uirqdev);
        if (ret)
        {
            printk("irq %d request fail...\r\n", dev->irqkey[i].irqnum);
            goto fail_requset;
        }
        tasklet_init(&dev->irqkey[i].tasklet, key_tasklet, (unsigned long)dev);
    }

    /*3、初始化定时器*/
    init_timer(&dev->timer);
    dev->timer.function = timer_fun;

    return 0;
fail_requset:
    for (i = 0; i < KEY_NUM; i++)
    {
        gpio_free(dev->irqkey[i].gpio);
    }
fail_node:
    return ret;
}
static int __init imx6uirq_init(void)
{
    int ret = 0;
    imx6uirqdev.major = 0;
    if (imx6uirqdev.major)
    {
        imx6uirqdev.devid = MKDEV(imx6uirqdev.major, 0);
        ret = register_chrdev_region(imx6uirqdev.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&imx6uirqdev.devid, 0, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
        imx6uirqdev.major = MAJOR(imx6uirqdev.devid);
        imx6uirqdev.minor = MINOR(imx6uirqdev.devid);
    }
    if (ret < 0)
    {
        goto fail_devid;
    }
    printk("major is %d , minor is %d\r\n", imx6uirqdev.major, imx6uirqdev.minor);

    imx6uirqdev.cdev.owner = THIS_MODULE;
    cdev_init(&imx6uirqdev.cdev, &imx6uirq_fops);
    ret = cdev_add(&imx6uirqdev.cdev, imx6uirqdev.devid, IMX6UIRQ_CNT);
    if (ret)
    {
        goto fail_cdevadd;
    }
    imx6uirqdev.class = class_create(THIS_MODULE, IMX6UIRQ_NAME);
    if (IS_ERR(imx6uirqdev.class))
    {
        ret = PTR_ERR(imx6uirqdev.class);
        goto fail_class;
    }
    imx6uirqdev.device = device_create(imx6uirqdev.class, NULL, imx6uirqdev.devid, NULL, IMX6UIRQ_NAME);
    if (IS_ERR(imx6uirqdev.device))
    {
        ret = PTR_ERR(imx6uirqdev.device);
        goto fail_device;
    }
    /*初始化keyIO*/

    ret = keyio_init(&imx6uirqdev);
    if (ret < 0)
    {
        goto fail_keyinit;
    }
    /*初始化原子变量*/
    atomic_set(&imx6uirqdev.keyvalue, INVAKEY);
    atomic_set(&imx6uirqdev.releasekey, 0);

    /* 初始化等待队列头 */
    init_waitqueue_head(&imx6uirqdev.r_wait);
    return 0;
fail_keyinit:
fail_device:
    class_destroy(imx6uirqdev.class);
fail_class:
    cdev_del(&imx6uirqdev.cdev);
fail_cdevadd:
    unregister_chrdev_region(imx6uirqdev.devid, IMX6UIRQ_CNT);
fail_devid:
    return ret;
}
static void __exit imx6uirq_exit(void)
{
    int i = 0;
    /*1、释放中断*/
    for (i = 0; i < KEY_NUM; i++)
    {
        free_irq(imx6uirqdev.irqkey[i].irqnum, &imx6uirqdev);
    }
    for (i = 0; i < KEY_NUM; i++)
    {
        gpio_free(imx6uirqdev.irqkey[i].gpio);
    }
    /*2\删除定时器*/
    del_timer_sync(&imx6uirqdev.timer);
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