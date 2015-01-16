/*
 * drivers/input/misc/sunxi-temperature.c
 *
 * Copyright (C) 2013-2014 allwinner.
 *	Li Ming<liming@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <asm/io.h>

#define TEMP_MAX_DELAY		(1000)
#define TEMP_BASSADDRESS	(0xf1c25000)
#define TEMP_CTRL_REG		(0X18)
#define TEMP_DATA_REG		(0x20)

#define TEMP_EN			(1<<16)
#define TEMP_PERIOD		(100<<0)

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

struct sunxi_temp_data {
	atomic_t delay;
	atomic_t enable;
	void __iomem *base_addr;
	struct input_dev *temp_dev;
	struct delayed_work work;
	struct mutex enable_mutex;
};
struct sunxi_temp_data *temp_data;
static unsigned int circle_num = 0;
static unsigned int history_record[10];

static ssize_t sunxi_temp_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	dprintk(DEBUG_CONTROL_INFO, "%d, %s\n", atomic_read(&temp_data->delay), __FUNCTION__);
	return sprintf(buf, "%d\n", atomic_read(&temp_data->delay));

}

static ssize_t sunxi_temp_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (data > TEMP_MAX_DELAY)
		data = TEMP_MAX_DELAY;
	atomic_set(&temp_data->delay, (unsigned int) data);

	return count;
}


static ssize_t sunxi_temp_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	dprintk(DEBUG_CONTROL_INFO, "%d, %s\n", atomic_read(&temp_data->enable), __FUNCTION__);
	return sprintf(buf, "%d\n", atomic_read(&temp_data->enable));

}

static void sunxi_temp_set_enable(struct device *dev, int enable)
{	
	int pre_enable = atomic_read(&temp_data->enable);

	mutex_lock(&temp_data->enable_mutex);
	if (enable) {
		if (pre_enable == 0) {
			schedule_delayed_work(&temp_data->work,
				msecs_to_jiffies(atomic_read(&temp_data->delay)));
			atomic_set(&temp_data->enable, 1);
		}
		
	} else {
		if (pre_enable == 1) {
			cancel_delayed_work_sync(&temp_data->work);
			atomic_set(&temp_data->enable, 0);
		} 
	}
	mutex_unlock(&temp_data->enable_mutex);
	
}

static ssize_t sunxi_temp_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if ((data == 0)||(data==1)) {
		sunxi_temp_set_enable(dev,data);
	}

	return count;
}

static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
		sunxi_temp_delay_show, sunxi_temp_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		sunxi_temp_enable_show, sunxi_temp_enable_store);

static struct attribute *sunxi_temp_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group sunxi_temp_attribute_group = {
	.attrs = sunxi_temp_attributes
};

static void calc_temperature(uint64_t *data)
{
	unsigned int i;
	uint64_t avg_temp = 0;

	if (circle_num % 10)
		return;

	for (i = 0; i < circle_num; i++) {
		avg_temp += history_record[i];
	}

	do_div(avg_temp, circle_num);
	printk("avg_temp_reg=%llu\n", avg_temp);

	avg_temp *= 100;
	if (0 != avg_temp)
		do_div(avg_temp, 583);

	avg_temp -= 295;
	printk("avg_temp=%llu C\n", avg_temp);

	circle_num = 0;

	*data = avg_temp;
}

static void sunxi_temp_read(uint64_t *data)
{
	history_record[circle_num] = readl(temp_data->base_addr + 0x20);
	dprintk(DEBUG_DATA_INFO, "history_record[%u]=%u\n", circle_num, history_record[circle_num]);
	circle_num++;
	calc_temperature(data);
	dprintk(DEBUG_DATA_INFO, "%s: temperature =%lld\n", __func__, *data);
	return;
}

static void sunxi_temp_work_func(struct work_struct *work)
{
	static uint64_t tempetature = 5;
	struct sunxi_temp_data *data = container_of((struct delayed_work *)work,
			struct sunxi_temp_data, work);
	unsigned long delay = msecs_to_jiffies(atomic_read(&data->delay));

	sunxi_temp_read(&tempetature);
	input_report_abs(data->temp_dev, ABS_MISC, tempetature);
	input_sync(data->temp_dev);
	dprintk(DEBUG_DATA_INFO, "%s: temperature %lld,\n", __func__, tempetature);
	
	schedule_delayed_work(&data->work, delay);
}

static void sunxi_temp_reg_init(void)
{
	//writel(TEMP_EN|TEMP_PERIOD, temp_data->base_addr + TEMP_CTRL_REG);
	writel(0x00a73fff,  temp_data->base_addr + 0x00);
	writel(0x130,       temp_data->base_addr + 0x04);
	writel(0x5,         temp_data->base_addr + 0x0c);
	writel(0x50313,     temp_data->base_addr + 0x10);
	writel(0x10000,     temp_data->base_addr + 0x18);

	printk("AW_VIR_TP_BASE + 0x00 = 0x%x\n", readl(temp_data->base_addr + 0x00));
	printk("AW_VIR_TP_BASE + 0x04 = 0x%x\n", readl(temp_data->base_addr + 0x04));
	printk("AW_VIR_TP_BASE + 0x0c = 0x%x\n", readl(temp_data->base_addr + 0x0c));
	printk("AW_VIR_TP_BASE + 0x10 = 0x%x\n", readl(temp_data->base_addr + 0x10));
	printk("AW_VIR_TP_BASE + 0x18 = 0x%x\n", readl(temp_data->base_addr + 0x18));
}

static int __init sunxi_temp_init(void)
{
	int err = 0;

	temp_data = kzalloc(sizeof(struct sunxi_temp_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(temp_data)) {
		printk(KERN_ERR "temp_data: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}
	temp_data->temp_dev = input_allocate_device();
	if (IS_ERR_OR_NULL(temp_data->temp_dev)) {
		printk(KERN_ERR "temp_dev: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail2;
	}

	temp_data->temp_dev->name = "sunxi-temp";  
	temp_data->temp_dev->phys = "sunxitemp/input0"; 
	temp_data->temp_dev->id.bustype = BUS_HOST;      
	temp_data->temp_dev->id.vendor = 0x0001;
	temp_data->temp_dev->id.product = 0x0001;
	temp_data->temp_dev->id.version = 0x0100;

	input_set_capability(temp_data->temp_dev, EV_ABS, ABS_MISC);
	//max 16bit 
	input_set_abs_params(temp_data->temp_dev, ABS_MISC, -50, 180, 0, 0);
	
	err = input_register_device(temp_data->temp_dev);
	if (0 < err) {
		printk("%s: could not register input device\n", __func__);
		input_free_device(temp_data->temp_dev);
		goto fail2;
	}

	INIT_DELAYED_WORK(&temp_data->work, sunxi_temp_work_func);

	mutex_init(&temp_data->enable_mutex);
	atomic_set(&temp_data->enable, 0);
	atomic_set(&temp_data->delay, TEMP_MAX_DELAY);

	err = sysfs_create_group(&temp_data->temp_dev->dev.kobj,
						 &sunxi_temp_attribute_group);
	if (err < 0)
	{
		printk("%s: sysfs_create_group err\n", __func__);
		goto fail3;
	}

	temp_data->base_addr = (void __iomem *)TEMP_BASSADDRESS;
	sunxi_temp_reg_init();

	dprintk(DEBUG_INIT, "%s: OK!\n", __func__);

	return 0;

	//sysfs_remove_group(&temp_data->temp_dev->dev.kobj, &sunxi_temp_attribute_group);
fail3:
	input_unregister_device(temp_data->temp_dev);
fail2:
	kfree(temp_data);
fail1:
	return err;	
}

static void __exit sunxi_temp_exit(void)
{
	sysfs_remove_group(&temp_data->temp_dev->dev.kobj, &sunxi_temp_attribute_group);
	input_unregister_device(temp_data->temp_dev);
	kfree(temp_data);
}

module_init(sunxi_temp_init);
module_exit(sunxi_temp_exit);

MODULE_DESCRIPTION("Temperature seneor driver");
MODULE_AUTHOR("Li Ming");
MODULE_LICENSE("GPL");


