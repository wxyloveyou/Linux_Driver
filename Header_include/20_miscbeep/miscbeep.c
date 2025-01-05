
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

static struct of_device_id beep_of_match[] = {
        {.compatible = "alientek,beep"},
        {/*Sentinel*/},
};
static int miscbeep_probe(struct platform_device *dev )
{
    return 0;
}
static int miscbeep_remove(struct platform_device *dev)
{
    return 0;
}
/*platform*/
static struct platform_driver miscbeep_driver={
    .driver = {
            .name = "alientek,beep",
            .of_match_table = beep_of_match,        //设备树匹配表

    },
    .probe = miscbeep_probe,
    .remove = miscbeep_remove,
};

/*驱动入口函数*/
static int __init miscbeep_init(void)
{
    return platform_driver_register(&miscbeep_driver);
}
static void __exit miscbeep_exit(void)
{
    platform_driver_unregister(&miscbeep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);

MODULE_AUTHOR("WXY");
MODULE_LICENSE("GPL");




