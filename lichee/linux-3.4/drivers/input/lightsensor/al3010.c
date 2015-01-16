/*
 * This file is part of the AL3010 sensor driver.
 *
 * Copyright (c) 2011 Liteon-semi Corporation
 *
 * Contact: YC Hou <yc_hou@liteon-semi.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 * Filename: al3010.c
 *
 * Summary:
 *	AL3010 sensor dirver for kernel version 3.0.8.
 *
 * Modification History:
 * Date     By       Summary
 * -------- -------- -------------------------------------------------------
 * 06/13/11 YC		 Original Creation (Test version:1.9)
 *
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/string.h>


#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <linux/input/mt.h>

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/device.h>


#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/init-input.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif

#include <mach/gpio.h> 

#define AL3010_DRV_NAME	                "al3010"
#define DRIVER_VERSION		        "1.9"
#define AL3010_MAX_DELAY                200

#define AL3010_NUM_CACHABLE_REGS	9

#define	AL3010_ALS_COMMAND	        0x10
#define	AL3010_RAN_MASK	                0x70
#define	AL3010_RAN_SHIFT	        (4)

#define AL3010_MODE_COMMAND	        0x00
#define AL3010_MODE_SHIFT	        (0)
#define AL3010_MODE_MASK	        0x07

#define AL3010_POW_MASK		        0x01
#define AL3010_POW_UP		        0x01
#define AL3010_POW_DOWN		        0x00
#define AL3010_POW_SHIFT	        (0)

#define	AL3010_ADC_LSB	                0x0c
#define	AL3010_ADC_MSB	                0x0d

static struct sensor_config_info config_info = {
	.input_type = LS_TYPE,
};

static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("[LIGHTSENSOR]:"fmt, ## arg)
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);

static const unsigned short normal_i2c[2] = {0x1c, I2C_CLIENT_END};

static u8 al3010_reg[AL3010_NUM_CACHABLE_REGS] = 
	{0x00, 0x01, 0x0c, 0x0d, 0x10, 0x1a, 0x1b, 0x1c, 0x1d};

static int al3010_range[4] = {77806, 19452, 4863, 1216};

#define LSC_DBG
#ifdef LSC_DBG
#define LDBG(s,args...)	{printk("LDBG: func [%s], line [%d], ",__func__,__LINE__); printk(s,## args);}
#else
#define LDBG(s,args...) {}
#endif

struct al3010_data {
        struct  input_dev       *input;
	struct  i2c_client      *client;
	struct  delayed_work sensor_work;
	struct  workqueue_struct *sensor_workq;
	struct  mutex lock;
	int     irq;
	atomic_t delay;
	atomic_t enable;
	u8 reg_cache[AL3010_NUM_CACHABLE_REGS];
	u8 power_state_before_suspend;	
#ifdef CONFIG_HAS_EARLYSUSPEND	
        struct early_suspend early_suspend;
#endif
};

int cali = 100;

#define ADD_TO_IDX(addr,idx)    {                       \
        int i;                                          \
        for(i = 0; i < AL3010_NUM_CACHABLE_REGS; i++) { \
                if (addr == al3010_reg[i]) {            \
                        idx = i;                        \
                        break;                          \
                }                                       \
        }                                               \
}

/*
 * register access helpers
 */

static int __al3010_read_reg(struct i2c_client *client,
			       u32 reg, u8 mask, u8 shift)
{
	struct al3010_data *data = i2c_get_clientdata(client);
	u8 idx = 0xff;

	ADD_TO_IDX(reg,idx)
	return (data->reg_cache[idx] & mask) >> shift;
}

static int __al3010_write_reg(struct i2c_client *client,
				u32 reg, u8 mask, u8 shift, u8 val)
{
	struct al3010_data *data = i2c_get_clientdata(client);
	int ret = 0;
	u8 tmp;
	u8 idx = 0xff;

	ADD_TO_IDX(reg,idx)
	if (idx >= AL3010_NUM_CACHABLE_REGS)
		return -EINVAL;

	mutex_lock(&data->lock);

	tmp = data->reg_cache[idx];
	tmp &= ~mask;
	tmp |= val << shift;

	ret = i2c_smbus_write_byte_data(client, reg, tmp);
	if (!ret)
		data->reg_cache[idx] = tmp;

	mutex_unlock(&data->lock);
	return ret;
}

/*
 * internally used functions
 */

/* range */
static int al3010_get_range(struct i2c_client *client)
{
	int tmp;
	tmp = __al3010_read_reg(client, AL3010_ALS_COMMAND,
	                        AL3010_RAN_MASK, AL3010_RAN_SHIFT);
	return al3010_range[tmp];
}

static int al3010_set_range(struct i2c_client *client, int range)
{
	return __al3010_write_reg(client, AL3010_ALS_COMMAND, 
	                        AL3010_RAN_MASK, AL3010_RAN_SHIFT, range);
}


/* mode */
static int al3010_get_mode(struct i2c_client *client)
{
	return __al3010_read_reg(client, AL3010_MODE_COMMAND,
		        AL3010_MODE_MASK, AL3010_MODE_SHIFT);
}

static int al3010_set_mode(struct i2c_client *client, int mode)
{
	return __al3010_write_reg(client, AL3010_MODE_COMMAND,
		        AL3010_MODE_MASK, AL3010_MODE_SHIFT, mode);
}

/* power_state */
static int al3010_set_power_state(struct i2c_client *client, int state)
{
	return __al3010_write_reg(client, AL3010_MODE_COMMAND,
				AL3010_POW_MASK, AL3010_POW_SHIFT, 
				state ? AL3010_POW_UP : AL3010_POW_DOWN);
}

static int al3010_get_power_state(struct i2c_client *client)
{
	struct al3010_data *data = i2c_get_clientdata(client);
	u8 cmdreg = data->reg_cache[AL3010_MODE_COMMAND];
	return (cmdreg & AL3010_POW_MASK) >> AL3010_POW_SHIFT;
}

static int al3010_get_adc_value(struct i2c_client *client)
{
	struct al3010_data *data = i2c_get_clientdata(client);
	int lsb, msb, range, val;

	mutex_lock(&data->lock);
	lsb = i2c_smbus_read_byte_data(client, AL3010_ADC_LSB);

	if (lsb < 0) {
		mutex_unlock(&data->lock);
		return lsb;
	}

	msb = i2c_smbus_read_byte_data(client, AL3010_ADC_MSB);
	mutex_unlock(&data->lock);

	if (msb < 0) {
	        printk("[LIGHTSENSOR] %s:read AL3010_ADC_MSB fail!\n", __func__);
		return msb;
	}

	range = al3010_get_range(client);
	val = (((msb << 8) | lsb) * range) >> 16;
	
	val = 8 * val;
	
	dprintk(DEBUG_INIT, "[LIGHTSENSOR] range = %d\n",range);
	dprintk(DEBUG_INIT, "[LIGHTSENSOR] row data lux = %d,cali = %d\n",val,cali);
	
	val *= cali;
	
	dprintk(DEBUG_INIT, "lsensor data lux = %d\n",val / 100);

	return (val / 100);
}

/*
 * sysfs layer
 */
static int al3010_input_init(struct al3010_data *data)
{
        struct input_dev *dev;
        int err;

        dev = input_allocate_device();
        if (!dev) {
                return -ENOMEM;
        }
        
        dev->name = "al3010";
        dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_ABS, ABS_MISC);
	input_set_abs_params(dev, ABS_MISC, 0, 65535, 0, 0);

        input_set_drvdata(dev, data);

        err = input_register_device(dev);
        if (err < 0) {
                input_free_device(dev);
		printk("input device register error! ret = [%d]\n", err);
                return err;
        }
        data->input = dev;
	
        return 0;
}

static void al3010_input_fini(struct al3010_data *data)
{
        struct input_dev *dev = data->input;

        input_unregister_device(dev);
        input_free_device(dev);
}

/* range */
static ssize_t al3010_show_range(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	
	return sprintf(buf, "%i\n", al3010_get_range(data->client));
}

static ssize_t al3010_store_range(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if ((strict_strtoul(buf, 10, &val) < 0) || (val > 3))
		return -EINVAL;

	ret = al3010_set_range(data->client, val);
	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(range, S_IWUSR | S_IRUGO,
		   al3010_show_range, al3010_store_range);


/* mode */
static ssize_t al3010_show_mode(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	
	return sprintf(buf, "%d\n", al3010_get_mode(data->client));
}

static ssize_t al3010_store_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if ((strict_strtoul(buf, 10, &val) < 0) || (val > 5))
		return -EINVAL;

	ret = al3010_set_mode(data->client, val);
	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(mode, S_IWUSR | S_IRUGO,
		   al3010_show_mode, al3010_store_mode);


/* power state */
static ssize_t al3010_show_power_state(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	
	return sprintf(buf, "%d\n", al3010_get_power_state(data->client));
}

static ssize_t al3010_store_power_state(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if ((strict_strtoul(buf, 10, &val) < 0) || (val > 1))
		return -EINVAL;

	ret = al3010_set_power_state(data->client, val);
	return ret ? ret : count;
}

static DEVICE_ATTR(power_state, S_IWUSR | S_IRUGO,
		   al3010_show_power_state, al3010_store_power_state);


/* lux */
static ssize_t al3010_show_lux(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);

	/* No LUX data if not operational */
	if (al3010_get_power_state(data->client) != 0x01)
		return sprintf((char*)buf, "%s\n", "Please power up first!");

	return sprintf(buf, "%d\n", al3010_get_adc_value(data->client));
}

static DEVICE_ATTR(lux, S_IRUGO, al3010_show_lux, NULL);


/* calibration */
static ssize_t al3010_show_calibration_state(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	return sprintf(buf, "%d\n", cali);
}

static ssize_t al3010_store_calibration_state(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	int stdls, lux; 
	char tmp[10];

	/* No LUX data if not operational */
	if (al3010_get_power_state(data->client) != 0x01) {
		printk("Please power up first!");
		return -EINVAL;
	}

	cali = 100;
	sscanf(buf, "%d %s", &stdls, tmp);

	if (!strncmp(tmp, "-setcv", 6)) {
		cali = stdls;
		return -EBUSY;
	}

	if (stdls < 0) {
		printk("Std light source: [%d] < 0 !!!\nCheck again, please.\n\
		Set calibration factor to 100.\n", stdls);
		return -EBUSY;
	}

	lux = al3010_get_adc_value(data->client);
	cali = stdls * 100 / lux;

	return -EBUSY;
}

static DEVICE_ATTR(calibration, S_IWUSR | S_IRUGO,
		   al3010_show_calibration_state, al3010_store_calibration_state);


#ifdef LSC_DBG
/* engineer mode */
static ssize_t al3010_em_read(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct al3010_data *data = i2c_get_clientdata(client);
	int i;
	u8 tmp;
	
	for (i = 0; i < ARRAY_SIZE(data->reg_cache); i++) {
		mutex_lock(&data->lock);
		tmp = i2c_smbus_read_byte_data(data->client, al3010_reg[i]);
		mutex_unlock(&data->lock);

		printk("Reg[0x%x] Val[0x%x]\n", al3010_reg[i], tmp);
	}

	return 0;
}

static ssize_t al3010_em_write(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct al3010_data *data = i2c_get_clientdata(client);
	u32 addr,val;
	int ret = 0;

	sscanf(buf, "%x%x", &addr, &val);

	printk("Write [%x] to Reg[%x]...\n",val,addr);
	mutex_lock(&data->lock);

	ret = i2c_smbus_write_byte_data(data->client, addr, val);
	if (!ret)
		data->reg_cache[addr] = val;

	mutex_unlock(&data->lock);

	return count;
}
static DEVICE_ATTR(em, S_IWUSR |S_IRUGO,
				   al3010_em_read, al3010_em_write);
#endif

static ssize_t al3010_show_enable(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
        struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	
	return sprintf(buf, "%d\n", atomic_read(&data->enable));
}

static ssize_t al3010_store_enable(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	unsigned long val;
	int pre_enable = atomic_read(&data->enable);

	if (strict_strtoul(buf, 10, &val) < 0)
		return -EINVAL; 
		
        dprintk(DEBUG_INIT, "%s: val = %ld , pre_enable = %d\n", __func__, val, pre_enable);
        
	if(val) {
	        if(pre_enable == 0) {
	                al3010_set_power_state(data->client, 1);
	                queue_delayed_work(data->sensor_workq,&data->sensor_work, 
	                                msecs_to_jiffies(atomic_read(&data->delay)));
			atomic_set(&data->enable, 1);
		}
	} else {
	        if(pre_enable == 1) {
	                cancel_delayed_work_sync(&data->sensor_work);
	                flush_workqueue(data->sensor_workq);
	                al3010_set_power_state(data->client, 0);
		        atomic_set(&data->enable, 0);
		}
	}
	
	return count;
}

static DEVICE_ATTR(enable, 0660,
		   al3010_show_enable, al3010_store_enable);

static ssize_t al3010_show_delay(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	
	return sprintf(buf, "%d\n", atomic_read(&data->delay));
}

static ssize_t al3010_store_delay(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct al3010_data *data = input_get_drvdata(input);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val) < 0)
		return -EINVAL;
		
        dprintk(DEBUG_INIT, "**%s: val = %ld \n", __func__, val);
        
        if(val > AL3010_MAX_DELAY)
                val = AL3010_MAX_DELAY;
	atomic_set(&data->delay, val);
	
	return count;
}

static DEVICE_ATTR(delay, 0660,
		   al3010_show_delay, al3010_store_delay);

static struct attribute *al3010_attributes[] = {
	&dev_attr_range.attr,
	&dev_attr_mode.attr,
	&dev_attr_power_state.attr,
	&dev_attr_lux.attr,
	&dev_attr_calibration.attr,
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
#ifdef LSC_DBG
	&dev_attr_em.attr,
#endif
	NULL
};

static const struct attribute_group al3010_attr_group = {
	.attrs = al3010_attributes,
};

static int al3010_init_client(struct i2c_client *client)
{
	struct al3010_data *data = i2c_get_clientdata(client);
	int i;

	/* read all the registers once to fill the cache.
	 * if one of the reads fails, we consider the init failed */
	for (i = 0; i < ARRAY_SIZE(data->reg_cache); i++) {
		int v = i2c_smbus_read_byte_data(client, al3010_reg[i]);
		if (v < 0)
			return -ENODEV;
		data->reg_cache[i] = v;
	}

	/* set defaults */
	al3010_set_range(client, 0);
	al3010_set_mode(client, 0);
	al3010_set_power_state(client, 1);

	return 0;
}


static void my_work_queue(struct work_struct *work)
{
        struct al3010_data *data = container_of((struct delayed_work *)work,
			struct al3010_data, sensor_work);
        int Aval;
        unsigned long delay = msecs_to_jiffies(atomic_read(&data->delay));
	
	dprintk(DEBUG_INIT, "**%s start!**\n", __func__);
	al3010_set_power_state(data->client, 1);
	Aval = al3010_get_adc_value(data->client);
	
	input_report_abs(data->input, ABS_MISC, Aval);
	input_sync(data->input);

	queue_delayed_work(data->sensor_workq,&data->sensor_work, delay);
	return ;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void al3010_early_suspend(struct early_suspend *h)
{
        struct al3010_data *data = container_of(h, struct al3010_data, early_suspend);
        
        dprintk(DEBUG_SUSPEND, "******%s start!******\n", __func__);
        
        data->power_state_before_suspend = al3010_get_power_state(data->client);
	cancel_delayed_work_sync(&data->sensor_work);
	flush_workqueue(data->sensor_workq);
	al3010_set_power_state(data->client, 0);
	atomic_set(&data->enable, 0);
        
}
static void al3010_later_resume(struct early_suspend *h)
{
       int i;
       struct al3010_data *data = container_of(h, struct al3010_data, early_suspend);
       
        dprintk(DEBUG_SUSPEND, "******%s start!******\n", __func__);
	/* restore registers from cache */
	for (i = 0; i < ARRAY_SIZE(data->reg_cache); i++)
		if (i2c_smbus_write_byte_data(data->client, i, data->reg_cache[i]))
			return ;

	al3010_set_power_state(data->client, 1);
	queue_delayed_work(data->sensor_workq,&data->sensor_work, 
	                                msecs_to_jiffies(atomic_read(&data->delay)));
	atomic_set(&data->enable, 1);
}
#endif

/*
 * I2C layer
 */

static int __devinit al3010_probe(struct i2c_client *client,
				    const struct i2c_device_id *id)
{
	struct al3010_data *data;
	int err = 0;
	
	dprintk(DEBUG_INIT, "******%s start!******\n",__func__);

	data = kzalloc(sizeof(struct al3010_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	
	data->client = client;
	i2c_set_clientdata(client, data);
	mutex_init(&data->lock);
	
	/* initialize the AL3010 chip */
	err = al3010_init_client(client);
	if (err)
		goto exit_kfree;
	
	err = al3010_input_init(data);
	if (err)
		goto exit_kfree;

	/* register sysfs hooks */
	
	err = sysfs_create_group(&data->input->dev.kobj, &al3010_attr_group);
	if (err)
		goto exit_input;
	
        data->sensor_workq = create_singlethread_workqueue("al3010");
        INIT_DELAYED_WORK(&data->sensor_work,my_work_queue);
        atomic_set(&data->delay, AL3010_MAX_DELAY);
	al3010_set_power_state(data->client, 1);
	atomic_set(&data->enable, 1);
	queue_delayed_work(data->sensor_workq,&data->sensor_work, 
	                msecs_to_jiffies(atomic_read(&data->delay)));
	                
#ifdef CONFIG_HAS_EARLYSUSPEND	
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;	
	data->early_suspend.suspend = al3010_early_suspend;
	data->early_suspend.resume  = al3010_later_resume;	
	register_early_suspend(&data->early_suspend);
#endif			

	dprintk(DEBUG_INIT, "AL3010 driver version %s enabled\n", DRIVER_VERSION);
	dprintk(DEBUG_INIT, "******%s end!******\n",__func__);
	return 0;

exit_input:
	al3010_input_fini(data);

exit_kfree:
	kfree(data);
	return err;
}

static int __devexit al3010_remove(struct i2c_client *client)
{
	struct al3010_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &al3010_attr_group);
	al3010_set_power_state(client, 0);
	cancel_delayed_work_sync(&data->sensor_work);
	flush_workqueue(data->sensor_workq);
	destroy_workqueue(data->sensor_workq);
	kfree(data);
	return 0;
}

#ifndef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_PM
static void al3010_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct al3010_data *data = i2c_get_clientdata(client);
        dprintk(DEBUG_SUSPEND, "******%s start!******\n", __func__);
        
	data->power_state_before_suspend = al3010_get_power_state(client);
	cancel_delayed_work_sync(&data->sensor_work);
	flush_workqueue(data->sensor_workq);
	al3010_set_power_state(client, 0);
	atomic_set(&data->enable, 0);
}

static void al3010_resume(struct i2c_client *client)
{
	int i;
	struct al3010_data *data = i2c_get_clientdata(client);
        
         dprintk(DEBUG_SUSPEND, "******%s start!******\n", __func__);
	/* restore registers from cache */
	for (i = 0; i < ARRAY_SIZE(data->reg_cache); i++)
		if (i2c_smbus_write_byte_data(client, i, data->reg_cache[i]))
			return -EIO;

	al3010_set_power_state(data->client, 1);
	queue_delayed_work(data->sensor_workq,&data->sensor_work, 
	                                msecs_to_jiffies(atomic_read(&data->delay)));
	atomic_set(&data->enable, 1);
}
#endif
#endif

static const struct i2c_device_id al3010_id[] = {
	{ "al3010", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, al3010_id);

static struct i2c_driver al3010_driver = {
    .class          = I2C_CLASS_HWMON,
	.driver = {
		.name	= AL3010_DRV_NAME,
		.owner	= THIS_MODULE,
	},
#ifndef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_PM
	.suspend = al3010_suspend,
	.resume	= al3010_resume,
#endif
#endif
	.probe	= al3010_probe,
	.remove	= __devexit_p(al3010_remove),
	.id_table = al3010_id,
	.address_list   =normal_i2c,
};

/**
 * ls_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ls_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret = 0;

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;
    
	if(config_info.twi_id == adapter->nr){
                dprintk(DEBUG_INIT, "%s: addr= %x\n",__func__,client->addr);
                ret = i2c_test(client);
        	if(!ret){
        		printk("%s:I2C connection might be something wrong ! \n",__func__);
        		return -ENODEV;
        	}else{           	    
            	        dprintk(DEBUG_INIT, "I2C connection sucess!\n");
            	        strlcpy(info->type, "al3010", I2C_NAME_SIZE);
    		        return 0;	
	        }

	}else{
		return -ENODEV;
	}
}


static int __init al3010_init(void)
{
	int ret = -1;
	
	dprintk(DEBUG_INIT, "******%s start!******\n", __func__);
	if(input_fetch_sysconfig_para(&(config_info.input_type))){
		printk("%s: err.\n", __func__);
		return -1;
	}
	
	if(config_info.sensor_used == 0){
		printk("*** used set to 0 !\n");
		printk("*** if use sensor,please put the sys_config.fex gy_used set to 1. \n");
		return 0;
	}
	
	al3010_driver.detect = ls_detect;
	ret = i2c_add_driver(&al3010_driver);
	
	dprintk(DEBUG_INIT, "******%s end!******\n", __func__);
	return ret;
}

static void __exit al3010_exit(void)
{
	i2c_del_driver(&al3010_driver);
}

MODULE_AUTHOR("YC Hou, LiteOn-semi corporation.");
MODULE_DESCRIPTION("Test AL3010 driver on mini6410.");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);

module_init(al3010_init);
module_exit(al3010_exit);


