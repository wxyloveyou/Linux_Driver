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
#include <linux/input.h>

#define KEYINPUT_CNT 1
#define KEYINPUT_NAME "keyinput"
#define KEY_NUM 1
#define KEY0VALUE 0x01
#define INVAKEY 0xFF
/*key结构体*/
struct irq_keydesc
{
    int gpio;                            // io编号
    int irqnum;                          // 中断号
    int value;                 //  按键值
    char name[10];                       // 名字
    irqreturn_t (*handler)(int, void *); // 中断处理函数

};

/*keyinput设备结构体*/
struct keyinput_dev
{
    struct device_node *node;
    struct irq_keydesc irqkey[KEY_NUM];
    struct timer_list timer;
    struct input_dev *inputdev;         //输入设备
};
struct keyinput_dev keyinputdev;

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
    struct keyinput_dev *dev = (struct keyinput_dev *)arg;

    // 获取GPIO引脚的电平状态
    value = gpio_get_value(dev->irqkey[0].gpio);

    // 检查GPIO电平状态
    if (value == 0)
    { // 按下
        /*上报按键值*/
        input_event(dev->inputdev, EV_KEY, BTN_0, 1);
        input_sync(dev->inputdev);
    }
    else if (value == 1)
    {
        /*上报按键值*/
        input_event(dev->inputdev, EV_KEY, BTN_0, 0);
        input_sync(dev->inputdev);
    }
}

/*按键中断处理函数*/
static irqreturn_t key0_handler(int irq, void *dev_id)
{

    struct keyinput_dev *dev = dev_id;
    printk("key0_handler is runing\r\n");
    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));     /*10ms定时*/

    return IRQ_HANDLED;
}

/*按键初始化*/
static int keyio_init(struct keyinput_dev *dev)
{
    int ret = 0;
    int i = 0;
    /*1、按键初始化*/
    dev->node = of_find_node_by_path("/key");
    if (keyinputdev.node == NULL)
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
    dev->irqkey[0].value = BTN_0;

    for (i = 0; i < KEY_NUM; ++i)
    {
        ret = request_irq(dev->irqkey[i].irqnum, dev->irqkey[i].handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                          dev->irqkey[i].name, &keyinputdev);
        if (ret)
        {
            printk("irq %d request fail...\r\n", dev->irqkey[i].irqnum);
            goto fail_requset;
        }

    }

    /*3、初始化定时器*/
    init_timer(&dev->timer);
    dev->timer.function = timer_fun;

        /**/
    return 0;
fail_requset:
    for (i = 0; i < KEY_NUM; i++)
    {
        gpio_free(dev->irqkey[i].gpio);
    }
fail_node:
    return ret;
}
static int __init keyinput_init(void)
{
   int ret = 0;
    /*初始化keyIO*/
    
    ret = keyio_init(&keyinputdev);
    if (ret < 0)
    {
        goto fail_keyinit;
    }
/*注册input dev*/
    keyinputdev.inputdev = input_allocate_device();
    if (keyinputdev.inputdev == NULL) {
        goto fail_keyinit;
    }
    keyinputdev.inputdev->name = KEYINPUT_NAME;
    __set_bit(EV_KEY, keyinputdev.inputdev->evbit);     //按键事件
    __set_bit(EV_REP, keyinputdev.inputdev->evbit);     //重复事件
    __set_bit(BTN_0, keyinputdev.inputdev->keybit);     //按键值

    ret = input_register_device(keyinputdev.inputdev);
    if (ret < 0) {
        goto fail_input_register;
    }
    return 0;
    fail_input_register:
    input_free_device(keyinputdev.inputdev);
fail_keyinit:
    return ret;
}
static void __exit keyinput_exit(void)
{
    int i = 0;
    /*1、释放中断*/
    for (i = 0; i < KEY_NUM; i++)
    {
        free_irq(keyinputdev.irqkey[i].irqnum, &keyinputdev);
    }
    for (i = 0; i < KEY_NUM; i++)
    {
        gpio_free(keyinputdev.irqkey[i].gpio);
    }
    /*2\删除定时器*/
    del_timer_sync(&keyinputdev.timer);
    /* 注销input _dev  */
    input_unregister_device(keyinputdev.inputdev);
    input_free_device(keyinputdev.inputdev);

}
module_init(keyinput_init);
module_exit(keyinput_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");