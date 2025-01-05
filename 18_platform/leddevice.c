/**
 * @FilePath     : /18_platform/leddevice.c
 * @Description  :
 * @Author       : wxy 173055256627@163.com
 * @Version      : 0.0.1
 * @LastEditors  : wxy 173055256627@163.com
 * @LastEditTime : 2024-12-24 22:27:10
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2024.
 **/
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

#define CCM_CCGR1_BASE (0X020C406C)
#define SW_MUX_GPIO1_IO03_BASE (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE (0X020E02F4)
#define GPIO1_DR_BASE (0X0209C000)
#define GPIO1_GDIR_BASE (0X0209C004)

#define REGISTER_LENGTH 4

void leddevice_release(struct device *dev)
{
        printk("leddevice_release\n");
}

static struct resource led_resources[] = {
    [0] = {
        .start = CCM_CCGR1_BASE,
        .end = CCM_CCGR1_BASE + REGISTER_LENGTH + -1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = SW_MUX_GPIO1_IO03_BASE,
        .end = SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH + -1,
        .flags = IORESOURCE_MEM,
    },
    [2] = {
        .start = SW_PAD_GPIO1_IO03_BASE,
        .end = SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH + -1,
        .flags = IORESOURCE_MEM,
    },
    [3] = {
        .start = GPIO1_DR_BASE,
        .end = GPIO1_DR_BASE + REGISTER_LENGTH + -1,
        .flags = IORESOURCE_MEM,
    },
    [4] = {
        .start = GPIO1_GDIR_BASE,
        .end = GPIO1_GDIR_BASE + REGISTER_LENGTH + -1,
        .flags = IORESOURCE_MEM,
    },

};

static struct platform_device leddevice = {
    .name = "imx6ull-led",
    .id = -1,
    .dev = {
        .release = leddevice_release,
    },
    .num_resources = ARRAY_SIZE(led_resources),
    .resource = led_resources,
};

/*设备加载  */
static int __init leddevice_init(void)
{
        int ret = 0;
        ret = platform_device_register(&leddevice);

        return ret;
}
static void __exit leddevice_exit(void)
{
        platform_device_unregister(&leddevice);
}

module_init(leddevice_init);
module_exit(leddevice_exit);

MODULE_AUTHOR("wxy");
MODULE_LICENSE("GPL");