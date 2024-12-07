#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

static int __init dtsof_init(void)
{
    int ret = 0;
    struct device_node *bl_nd = NULL;
    struct property *comppro = NULL;
    const char *str;
    unsigned int def_value = 0;
    u32 elesize = 0;
    unsigned int *brival;
    int i = 0;
    /*找到backlight节点，路径是:/blaaklight*/
    bl_nd = of_find_node_by_path("/backlight");
    ;
    ;
    if (bl_nd == NULL)
    {
        ret = -EINVAL;
        goto faile_findnd;
    }
    /*2.获取属性*/;
    comppro = of_find_property(bl_nd, "compatible", NULL);
    if (comppro == NULL)
    {
        ret = -EINVAL;
        goto faile_findpro;
    }
    else
    {
        printk("compatible = %s\r\n", (char *)comppro->value);
    }

    ret = of_property_read_string(bl_nd, "status", &str);
    if (ret < 0)
    {
        goto faile_rs;
    }
    else
    {
        printk("status = %s\r\n", str);

    }
    /*3.读取数字属性数值*/;
    ret = of_property_read_u32(bl_nd, "default-brightness-level", &def_value);
    if (ret < 0)
    {
        goto file_read32;
    }
    else
    {
        printk("default-brightness-level =  %d\r\n", def_value);
    }
    /*4.获取数组类型长度*/;
    elesize = of_property_count_elems_of_size(bl_nd, "brightness-levels", sizeof(u32));
    if (elesize < 0)
    {
        ret = -EINVAL;
        goto file_readele;
    }
    else
    {
        printk("brightness-level =  %d\r\n", elesize);
    }
    /*申请内存*/
    brival = kmalloc(elesize * sizeof(u32), GFP_KERNEL);
    if (brival == NULL)
    {
        ret = -EINVAL;
        goto fale_mem;
    }
    /*获取数组*/;
    ret = of_property_read_u32_array(bl_nd, "brightness-levels", brival, elesize);
    if (ret < 0)
    {
        goto fail_read32array;
    }
    else
    {
        for (i = 0; i < elesize; i++)
            printk("brightness-levels[%d] = %d\r\n", i, *(brival + i));
    }
    kfree(brival);
    return 0;
fail_read32array:
    kfree(brival);
faile_findnd:
faile_findpro:
faile_rs:
file_read32:
file_readele:
fale_mem:
    return ret;
}
static void __exit dtsof_exit(void)
{
}
module_init(dtsof_init);
module_exit(dtsof_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WXY");
