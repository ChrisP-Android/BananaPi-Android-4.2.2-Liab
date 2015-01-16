/* driver/misc/sunxi-reg.c
 *
 *  Copyright (C) 2011 Reuuimlla Technology Co.Ltd
 *  Tom Cubie <tangliang@reuuimllatech.com>
 *  update by panlong <panlong@reuuimllatech.com> , 2012-4-19 15:39
 *
 *  www.reuuimllatech.com
 *
 *  User access to the registers driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/rwsem.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/sysfs.h>

#include <mach/sys_config.h>
#include <asm/io.h>


DECLARE_RWSEM(gpio_sw_list_lock);
LIST_HEAD(gpio_sw_list);
static struct class *gpio_sw_class;

struct sw_gpio_pd {
	char					name[16];
/* the four member under here is prepare for interrupt*/
	unsigned int			mask;
	enum gpio_eint_trigtype	trig_type;
	peint_handle 			handle;
	void 					*para;
};

struct gpio_sw_classdev{
	const char			*name;
	struct mutex		class_mutex;
	script_item_u 		*item;
	int		(*gpio_sw_cfg_set)(struct gpio_sw_classdev *gpio_sw_cdev,
			int  mul_cfg);
	int		(*gpio_sw_pull_set)(struct gpio_sw_classdev *gpio_sw_cdev,
			int  mul_cfg);
	int		(*gpio_sw_data_set)(struct gpio_sw_classdev *gpio_sw_cdev,
			int  mul_cfg);
	int		(*gpio_sw_drv_set)(struct gpio_sw_classdev *gpio_sw_cdev,
			int  mul_cfg);
	int		(*gpio_sw_cfg_get)(struct gpio_sw_classdev *gpio_sw_cdev);
	int		(*gpio_sw_pull_get)(struct gpio_sw_classdev *gpio_sw_cdev);
	int		(*gpio_sw_data_get)(struct gpio_sw_classdev *gpio_sw_cdev);
	int		(*gpio_sw_drv_get)(struct gpio_sw_classdev *gpio_sw_cdev);
	struct device		*dev;
	struct list_head	 node;
};

struct sw_gpio {
	struct sw_gpio_pd		*pdata;
	spinlock_t				lock;
	struct gpio_sw_classdev	class;
};

static struct platform_device *gpio_sw_dev[256];
static struct sw_gpio_pd *sw_pdata[256];

static int
gpio_sw_cfg_set(struct gpio_sw_classdev *gpio_sw_cdev,int  mul_cfg){
	return sw_gpio_setcfg(gpio_sw_cdev->item->gpio.gpio,mul_cfg);
}

static int
gpio_sw_cfg_get(struct gpio_sw_classdev *gpio_sw_cdev){
	return sw_gpio_getcfg(gpio_sw_cdev->item->gpio.gpio);
}

static int
gpio_sw_pull_set(struct gpio_sw_classdev *gpio_sw_cdev,int  pull){
	return sw_gpio_setpull(gpio_sw_cdev->item->gpio.gpio,pull);
}

static int
gpio_sw_pull_get(struct gpio_sw_classdev *gpio_sw_cdev){
	return sw_gpio_getpull(gpio_sw_cdev->item->gpio.gpio);
}

static int
gpio_sw_drv_set(struct gpio_sw_classdev *gpio_sw_cdev,int  drv){
	return sw_gpio_setdrvlevel(gpio_sw_cdev->item->gpio.gpio,drv);
}

static int
gpio_sw_drv_get(struct gpio_sw_classdev *gpio_sw_cdev){
	return sw_gpio_getdrvlevel(gpio_sw_cdev->item->gpio.gpio);
}

static int
gpio_sw_data_set(struct gpio_sw_classdev *gpio_sw_cdev,int  data){
	__gpio_set_value(gpio_sw_cdev->item->gpio.gpio,data);
	return 0;
}

static int
gpio_sw_data_get(struct gpio_sw_classdev *gpio_sw_cdev){
	return __gpio_get_value(gpio_sw_cdev->item->gpio.gpio);
}

static ssize_t cfg_sel_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	int length;

	mutex_lock(&gpio_sw_cdev->class_mutex);
	gpio_sw_cdev->item->gpio.mul_sel = gpio_sw_cdev->gpio_sw_drv_get(gpio_sw_cdev);
	length = sprintf(buf, "%u\n", gpio_sw_cdev->item->gpio.mul_sel);
	mutex_unlock(&gpio_sw_cdev->class_mutex);

	return length;
}

static ssize_t pull_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	int length;

	mutex_lock(&gpio_sw_cdev->class_mutex);
	gpio_sw_cdev->item->gpio.pull = gpio_sw_cdev->gpio_sw_drv_get(gpio_sw_cdev);
	length = sprintf(buf, "%u\n", gpio_sw_cdev->item->gpio.pull);
	mutex_unlock(&gpio_sw_cdev->class_mutex);

	return length;
}

static ssize_t drv_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	int length;

	mutex_lock(&gpio_sw_cdev->class_mutex);
	gpio_sw_cdev->item->gpio.drv_level = \
	   	gpio_sw_cdev->gpio_sw_drv_get(gpio_sw_cdev);
	length = sprintf(buf, "%u\n", gpio_sw_cdev->item->gpio.drv_level);
	mutex_unlock(&gpio_sw_cdev->class_mutex);

	return length;
}

static ssize_t data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	int length;

	mutex_lock(&gpio_sw_cdev->class_mutex);
	gpio_sw_cdev->item->gpio.data = gpio_sw_cdev->gpio_sw_data_get(gpio_sw_cdev);
	length = sprintf(buf, "%u\n", gpio_sw_cdev->item->gpio.data);
	mutex_unlock(&gpio_sw_cdev->class_mutex);

	return length;
}

static ssize_t cfg_sel_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int cfg = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size){
		ret = count;
		gpio_sw_cdev->gpio_sw_cfg_set(gpio_sw_cdev, cfg);
	}

	return ret;
}


static ssize_t pull_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int pull = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size){
		ret = count;
		gpio_sw_cdev->gpio_sw_pull_set(gpio_sw_cdev, pull);
	}

	return ret;
}


static ssize_t drv_level_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int drv_level = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size){
		ret = count;
		gpio_sw_cdev->gpio_sw_drv_set(gpio_sw_cdev, drv_level);
	}

	return ret;
}

static ssize_t data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	int data = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;
	if (isspace(*after))
		count++;
	if (count == size){
		ret = count;
		gpio_sw_cdev->gpio_sw_data_set(gpio_sw_cdev,data);
	}
	return ret;
}


static int
gpio_sw_suspend(struct device *dev, pm_message_t state)
{
	return 0;
}

static int gpio_sw_resume(struct device *dev)
{
	return 0;
}
#if 0
	/* this node is prepared fot interrupt*/
static ssize_t trigger_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	int i,len=0;

	if (!gpio_sw_cdev->irq)
		len += sprintf(buf+len, "[none] ");
	else
		len += sprintf(buf+len, "none ");
	for (i = 0; i < ARRAY_SIZE(trigger_types); i++){
		if(gpio_sw_cdev->irq_type == trigger_types[i].flags)
			len += sprintf(buf+len, "[%s] ", trigger_types[i].name);
		else
			len += sprintf(buf+len, "%s ", trigger_types[i].name);
	}
	len += sprintf(buf+len, "\n");
	return len;
}

static ssize_t trigger_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpio_sw_classdev *gpio_sw_cdev = dev_get_drvdata(dev);
	int i,ret=-1;

	for (i = 0; i < ARRAY_SIZE(trigger_types); i++){
		if(sysfs_streq(buf,trigger_types[i].name)){
			ret = set_trigger_types(gpio_sw_cdev, trigger_types[i].flags);
			gpio_sw_cdev->irq_type=trigger_types[i].flags;
			break;
		}
	}
	return size;
}
#endif
static struct device_attribute gpio_sw_class_attrs[] = {
	__ATTR(cfg,		0664, cfg_sel_show, cfg_sel_store),
	__ATTR(pull,	0664, pull_show, pull_store),
	__ATTR(drv,		0664, drv_level_show, drv_level_store),
	__ATTR(data,	0664, data_show, data_store),
#if 0
	/* this node is prepared fot interrupt*/
	__ATTR(trigger, 0644,trigger_show,trigger_store),
#endif
	__ATTR_NULL,
};

void
gpio_sw_classdev_unregister(struct gpio_sw_classdev *gpio_sw_cdev)
{
	mutex_destroy(&gpio_sw_cdev->class_mutex);
	device_unregister(gpio_sw_cdev->dev);
	down_write(&gpio_sw_list_lock);
	list_del(&gpio_sw_cdev->node);
	up_write(&gpio_sw_list_lock);
}

static int __devexit gpio_sw_remove(struct platform_device *dev)
{
	struct sw_gpio *sw_gpio_entry=platform_get_drvdata(dev);
	gpio_sw_classdev_unregister(&sw_gpio_entry->class);
	kfree(sw_gpio_entry->class.item);
	kfree(sw_gpio_entry);
	return 0;
}

int
gpio_sw_classdev_register(struct device *parent,
		struct gpio_sw_classdev *gpio_sw_cdev)
{
	gpio_sw_cdev->dev = device_create(gpio_sw_class, parent, 0, gpio_sw_cdev,
			"%s", gpio_sw_cdev->name);
	if (IS_ERR(gpio_sw_cdev->dev))
		return PTR_ERR(gpio_sw_cdev->dev);
	down_write(&gpio_sw_list_lock);
	list_add_tail(&gpio_sw_cdev->node, &gpio_sw_list);
	up_write(&gpio_sw_list_lock);
	mutex_init(&gpio_sw_cdev->class_mutex);

	return 0;
}

static int mapGpioToName(char *name, u32 gpio)
{
	char base;
	int num;
	num = gpio - PA_NR_BASE;
	if(num < 0)
	{
		goto map_fail;
	}
	if((num >= 0) && (num < PA_NR))
	{
		base = 'A';
		goto map_done;
	}
	num = gpio - PB_NR_BASE;
	if((num >= 0) && (num < PB_NR))
	{
		base = 'B';
		goto map_done;
	}
	num = gpio - PC_NR_BASE;
	if((num >= 0) && (num < PC_NR))
	{
		base = 'C';
		goto map_done;
	}
	num = gpio - PD_NR_BASE;
	if((num >= 0) && (num < PD_NR))
	{
		base = 'D';
		goto map_done;
	}
	num = gpio - PE_NR_BASE;
	if((num >= 0) && (num < PE_NR))
	{
		base = 'E';
		goto map_done;
	}
	num = gpio - PF_NR_BASE;
	if((num >= 0) && (num < PF_NR))
	{
		base = 'F';
		goto map_done;
	}
	num = gpio - PG_NR_BASE;
	if((num >= 0) && (num < PG_NR))
	{
		base = 'G';
		goto map_done;
	}
	num = gpio - PH_NR_BASE;
	if((num >= 0) && (num < PH_NR))
	{
		base = 'H';
		goto map_done;
	}
	num = gpio - PI_NR_BASE;
	if((num >= 0) && (num < PI_NR))
	{
		base = 'I';
		goto map_done;
	}

	goto map_fail;
map_done:
	sprintf(name, "P%c%d", base, num);
	return 0;
map_fail:
	return -1;
}

static int __devinit gpio_sw_probe(struct platform_device *dev)
{
	struct sw_gpio				*sw_gpio_entry;
	struct sw_gpio_pd			*pdata = dev->dev.platform_data;
	int							ret;
	unsigned long				flags;
	script_item_value_type_e	type;
	char io_area[16];

	sw_gpio_entry = kzalloc(sizeof(struct sw_gpio), GFP_KERNEL);
	if(!sw_gpio_entry)
		return -ENOMEM;
	sw_gpio_entry->class.item = \
	   	kzalloc(sizeof(script_item_u), GFP_KERNEL);
	type = script_get_item("gpio_para", pdata->name, sw_gpio_entry->class.item);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type){
		printk(KERN_ERR "get config err!\n");
		return -ENOMEM;
	}

	ret = mapGpioToName(io_area,sw_gpio_entry->class.item->gpio.gpio);
	printk("gpio name is %s, ret = %d\n",io_area, ret);

	platform_set_drvdata(dev,sw_gpio_entry);
	spin_lock_init(&sw_gpio_entry->lock);
	spin_lock_irqsave(&sw_gpio_entry->lock, flags);
	sw_gpio_entry->pdata		= pdata;
	//sw_gpio_entry->class.name	= pdata->name;
	if(ret ==0 )
	{
		sw_gpio_entry->class.name	= io_area;
	}
	else
	{
		sw_gpio_entry->class.name	= pdata->name;
	}

	sw_gpio_entry->class.gpio_sw_cfg_set = gpio_sw_cfg_set;
	sw_gpio_entry->class.gpio_sw_cfg_get = gpio_sw_cfg_get;
	sw_gpio_entry->class.gpio_sw_pull_set = gpio_sw_pull_set;
	sw_gpio_entry->class.gpio_sw_pull_get = gpio_sw_pull_get;
	sw_gpio_entry->class.gpio_sw_drv_set = gpio_sw_drv_set;
	sw_gpio_entry->class.gpio_sw_drv_get = gpio_sw_drv_get;
	sw_gpio_entry->class.gpio_sw_data_set = gpio_sw_data_set;
	sw_gpio_entry->class.gpio_sw_data_get = gpio_sw_data_get;

	if(0 != sw_gpio_setall_range(&sw_gpio_entry->class.item->gpio, 1)){
		printk("%s sw_gpio_setall_range failed\n",pdata->name);
		return -ENOMEM;
	}
	spin_unlock_irqrestore(&sw_gpio_entry->lock, flags);

	ret = gpio_sw_classdev_register(&dev->dev, &sw_gpio_entry->class);
	if(ret < 0){
		dev_err(&dev->dev, "gpio_sw_classdev_register failed\n");
		kfree(sw_gpio_entry);
		return -1;
	}
	return 0;
}

static void gpio_sw_release(struct device *dev)
{
	printk("gpio_sw_release good !\n");
}

static int gpio_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int gpio_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver gpio_sw_driver = {
	.probe		= gpio_sw_probe,
	.remove		= gpio_sw_remove,
	.suspend	= gpio_suspend,
	.resume		= gpio_resume,
	.driver		= {
		.name		= "gpio_sw",
		.owner		= THIS_MODULE,
	},
};

static int __init gpio_sw_init(void)
{
	int i,cnt,used=0;
	script_item_u val;
	script_item_u   *list = NULL;
	script_item_value_type_e  type;

	gpio_sw_class = class_create(THIS_MODULE, "gpio_sw");
	if (IS_ERR(gpio_sw_class))
		return PTR_ERR(gpio_sw_class);

	gpio_sw_class->suspend		= gpio_sw_suspend;
	gpio_sw_class->resume		= gpio_sw_resume;
	gpio_sw_class->dev_attrs 	= gpio_sw_class_attrs;

	type = script_get_item("gpio_para", "gpio_used", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
		printk(KERN_ERR "failed to get gpio_para used information\n");
		goto INIT_END;
	}

	used = val.val;
	if(!used){
		printk(KERN_ERR "this module is used not!\n");
		goto INIT_END;
	}

	cnt = script_get_pio_list("gpio_para", &list);
	if(!cnt){
		printk(KERN_ERR "these is zero number for gpio\n");
		goto INIT_END;
	}
	for(i=0;i<cnt;i++){
		if(gpio_request(list[i].gpio.gpio, NULL)){
			printk("gpio_pin_%d gpio_request fail \n",i+1);
			continue;
		}
		sw_pdata[i] = kzalloc(sizeof(struct sw_gpio_pd), GFP_KERNEL);
		if(!sw_pdata[i]){
			printk(KERN_ERR "kzalloc fail for sw_pdata[%d]\n",i);
			return -1;
		}

		gpio_sw_dev[i] = kzalloc(sizeof(struct platform_device), GFP_KERNEL);
		if(!gpio_sw_dev[i]){
			printk(KERN_ERR "kzalloc fail for gpio_sw_dev[%d]\n",i);
			return -1;
		}

		sprintf(sw_pdata[i]->name,"gpio_pin_%d",i+1);
		gpio_sw_dev[i]->name = "gpio_sw";
		gpio_sw_dev[i]->id   = i;
		gpio_sw_dev[i]->dev.platform_data	= sw_pdata[i];
		gpio_sw_dev[i]->dev.release			= gpio_sw_release;

		if(platform_device_register(gpio_sw_dev[i])){
			printk(KERN_ERR "%s platform_device_register fail\n",sw_pdata[i]->name);
			goto INIT_ERR_FREE;
		}
	}
	if(platform_driver_register(&gpio_sw_driver)){
		printk(KERN_ERR "gpio user platform_driver_register  fail\n");
		for(i=0;i<cnt;i++)
			platform_device_unregister(gpio_sw_dev[i]);
		goto INIT_ERR_FREE;
	}

INIT_END:
	printk("gpio_init finish with uesd %d! \n",used);
	return 0;
INIT_ERR_FREE:
	printk("gpio_init err\n");
	kfree(sw_pdata[i]);
	kfree(gpio_sw_dev[i]);
	return -1;
}

static void __exit gpio_sw_exit(void)
{
	int i,cnt,used;
	script_item_u *list = NULL;
	script_item_value_type_e  type;
	script_item_u val;

	type = script_get_item("gpio_para", "gpio_used", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
		printk(KERN_ERR "failed to get gpio_para used information\n");
		goto EXIT_END;
	}

	used = val.val;
	if(!used){
		printk("this module is used not!\n");
		goto EXIT_END;
	}

	platform_driver_unregister(&gpio_sw_driver);

	cnt = script_get_pio_list("gpio_para", &list);
	if(!cnt){
		printk(KERN_WARNING "these is zero number for gpio\n");
		goto EXIT_END;
	}
	for(i=0;i<cnt;i++){
		platform_device_unregister(gpio_sw_dev[i]);
		kfree(gpio_sw_dev[i]);
		kfree(sw_pdata[i]);
		gpio_free(list[i].gpio.gpio);
	}

	class_destroy(gpio_sw_class);
EXIT_END:
	printk("gpio_exit finish !\n");
}

module_init(gpio_sw_init);
module_exit(gpio_sw_exit);

MODULE_AUTHOR("panlong");
MODULE_DESCRIPTION("SW GPIO USER driver");
MODULE_LICENSE("GPL");


