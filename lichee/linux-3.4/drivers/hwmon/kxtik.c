/*
 *  mma7660.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor 
 *
 *  Copyright (C) 2009-2010 Freescale Semiconductor Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/device.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/init-input.h>

/*
 * Defines
 */
#define assert(expr)\
	if (!(expr)) {\
		printk(KERN_ERR "Assertion failed! %s,%d,%s,%s\n",\
			__FILE__, __LINE__, __func__, #expr);\
	}

#define KXTIK_DRV_NAME	                "kxtik"
#define SENSOR_NAME 			KXTIK_DRV_NAME
#define ACCEL_GRP2_RES_12BIT	        (1 << 6)
#define ACCEL_GRP2_G_MASK		(3 << 3)
#define ACCEL_GRP2_G_2G			(0 << 3)
#define ACCEL_GRP2_CTRL_REG1	        0x1B
#define ACCEL_GRP2_DATA_CTRL	        0x21
#define ACCEL_GRP2_PC1_ON		(1 << 7)
#define ACCEL_GRP2_PC1_OFF		(0 << 7)
#define ACCEL_G_MAX			8096
#define ACCEL_FUZZ			3
#define ACCEL_FLAT			3
#define POLL_INTERVAL_MAX	        500
#define POLL_INTERVAL		        100
#define INPUT_FUZZ	                2
#define INPUT_FLAT	                2
#define ACCEL_GRP2_XOUT_L		0x06
#define ACCEL_WHO_AM_I		        0x0F
#define KIONIX_ACCEL_WHO_AM_I_KXTIK     0x05
#define KIONIX_ACCEL_WHO_AM_I_KXTJ9     0x08

#define I2C_ADDRESS                     0x0F 


#define MODE_CHANGE_DELAY_MS            100

static struct device *hwmon_dev;
static struct i2c_client *kxtik_i2c_client;
static struct input_polled_dev *kxtik_idev;

struct kxtik_data_s {
        struct i2c_client       *client;
        struct input_polled_dev *pollDev; 
        struct mutex interval_mutex; 
        atomic_t enable;
    
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	volatile int suspend_indator;
#endif
} kxtik_data;

enum {
	accel_grp2_ctrl_reg1 = 0,
	accel_grp2_data_ctrl,
	accel_grp2_int_ctrl,
	accel_grp2_regs_count,
};

u8 accel_registers[4];

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x0f, I2C_CLIENT_END};
static struct sensor_config_info config_info = {
	.input_type = GSENSOR_TYPE,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void kxtik_early_suspend(struct early_suspend *h);
static void kxtik_late_resume(struct early_suspend *h);
#endif

static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

/**
 * gsensor_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
        struct i2c_adapter *adapter = client->adapter;
        int ret;

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;
            
        if(config_info.twi_id == adapter->nr){
                ret = i2c_smbus_read_byte_data(client,ACCEL_WHO_AM_I);
                dprintk(DEBUG_INIT, "%s: addr = 0x%x, ret :%d", __func__, client->addr, ret);
            
                if (((ret &0x00FF) == KIONIX_ACCEL_WHO_AM_I_KXTIK) || 
                        ((ret &0x00FF) == KIONIX_ACCEL_WHO_AM_I_KXTJ9)) {
                        strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
                        return 0;
        
                }else{
                         printk("%s: kionix Sensor Device not found, \
                                maybe the other gsensor equipment! \n",__func__);
                        return -ENODEV;
                }
        
        }else{
        	return -ENODEV;
        }
}

static int kionix_i2c_read(struct i2c_client *client, u8 addr, u8 *data, int len)
{
	struct i2c_msg msgs[] = {
		{
			.addr   = client->addr,
			.flags  = client->flags,
			.len    = 1,
			.buf    = &addr,
		},
		{
			.addr   = client->addr,
			.flags  = client->flags | I2C_M_RD,
			.len    = len,
			.buf    = data,
		},
	};

	return i2c_transfer(client->adapter, msgs, 2);
}

static ssize_t kxtik_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = kxtik_i2c_client;
	struct kxtik_data_s *kxtik = NULL;
	
        kxtik = i2c_get_clientdata(client);
	dprintk(DEBUG_CONTROL_INFO, "%d, %s\n", kxtik->pollDev->poll_interval, __FUNCTION__);
	
	return sprintf(buf, "%d\n", kxtik->pollDev->poll_interval);

}

static ssize_t kxtik_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = kxtik_i2c_client;
	struct kxtik_data_s *kxtik = NULL;
	
        kxtik = i2c_get_clientdata(client);
	error = strict_strtoul(buf, 10, &data);
	
	if (error)
		return error;
		
	if (data > POLL_INTERVAL_MAX)
		data = POLL_INTERVAL_MAX;
		
	if(kxtik->pollDev->poll_interval == data)
	        return count;	        
    
        mutex_lock(&kxtik->interval_mutex);
        kxtik->pollDev->poll_interval = data;
        mutex_unlock(&kxtik->interval_mutex);

	return count;
}

static ssize_t kxtik_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{		
	dprintk(DEBUG_CONTROL_INFO, "%d, %s\n", atomic_read(&kxtik_data.enable), __FUNCTION__);
	return sprintf(buf, "%d\n", atomic_read(&kxtik_data.enable));

}


static ssize_t kxtik_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int en, old_en;
	int error;
	
	error = strict_strtoul(buf, 10, &data);
	
	if(error) {
		pr_err("%s strict_strtoul error\n", __FUNCTION__);
		goto exit;
	}
	
	en = data ? 1 : 0;
	old_en = atomic_read(&kxtik_data.enable);
	
	if(en == old_en)
	        return count;       

	if(en) {
	        atomic_set(&kxtik_data.enable,1);
                error = i2c_smbus_write_byte_data(kxtik_i2c_client,ACCEL_GRP2_CTRL_REG1,
                accel_registers[accel_grp2_ctrl_reg1] | ACCEL_GRP2_PC1_ON); 
                assert(error==0);
                kxtik_idev->input->open(kxtik_idev->input);
	} else {
	        atomic_set(&kxtik_data.enable,0);
                error = i2c_smbus_write_byte_data(kxtik_i2c_client,ACCEL_GRP2_CTRL_REG1,
                accel_registers[accel_grp2_ctrl_reg1] | ACCEL_GRP2_PC1_OFF);
	        assert(error==0);
	        kxtik_idev->input->close(kxtik_idev->input);
	}

	return count;

exit:
	return error;
}


static DEVICE_ATTR(enable, 0664,
		kxtik_enable_show, kxtik_enable_store);

static DEVICE_ATTR(delay, 0664,
		kxtik_delay_show,  kxtik_delay_store);

static struct attribute *kxtik_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
	NULL
};

static struct attribute_group kxtik_attribute_group = {
	.attrs = kxtik_attributes
};


/*
 * Initialization function
 */
static int kxtik_init_client(struct i2c_client *client)
{
	int result;
   
	kxtik_i2c_client = client;
        accel_registers[accel_grp2_ctrl_reg1] |= ACCEL_GRP2_RES_12BIT;
        accel_registers[accel_grp2_ctrl_reg1] &= ~ACCEL_GRP2_G_MASK;
   
	accel_registers[accel_grp2_ctrl_reg1] |= ACCEL_GRP2_G_2G;
	
	result = i2c_smbus_write_byte_data(client,
					ACCEL_GRP2_CTRL_REG1, 0);
	assert(result==0);				
	result = i2c_smbus_write_byte_data(client,
					ACCEL_GRP2_DATA_CTRL, accel_registers[accel_grp2_data_ctrl]);
	assert(result==0);				
	result = i2c_smbus_write_byte_data(client,ACCEL_GRP2_CTRL_REG1, accel_registers[accel_grp2_ctrl_reg1] | ACCEL_GRP2_PC1_ON);
					

	assert(result==0);

	mdelay(MODE_CHANGE_DELAY_MS);

	return result;
}

static void report_abs(void)
{

        struct { 
                union {
                        s16 accel_data_s16[3];
                        s8 accel_data_s8[6];
                }; 
        } accel_data;
        s16 x, y, z;

	kionix_i2c_read(kxtik_i2c_client , ACCEL_GRP2_XOUT_L, (u8 *)accel_data.accel_data_s16, 6);
	
	x = ((s16) (accel_data.accel_data_s16[0])) >> 4;
	y = ((s16) (accel_data.accel_data_s16[1])) >> 4;
	z = ((s16) (accel_data.accel_data_s16[2])) >> 4;

        dprintk(DEBUG_DATA_INFO, "%s x =%d, y =%d, z =%d \n", __func__, x, y, z);

	input_report_abs(kxtik_idev->input, ABS_X, x);
	input_report_abs(kxtik_idev->input, ABS_Y, y);
	input_report_abs(kxtik_idev->input, ABS_Z, z);
	input_sync(kxtik_idev->input);	

}

static void kxtik_dev_poll(struct input_polled_dev *dev)
{

	report_abs();
        return;
} 

/*
 * I2C init/probing/exit functions
 */

static int __devinit kxtik_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int result;
	struct input_dev *idev;
	struct i2c_adapter *adapter;
        struct kxtik_data_s* data = &kxtik_data;
 
	dprintk(DEBUG_INIT, "kxtik probe\n");
	client->addr = I2C_ADDRESS;
	kxtik_i2c_client = client;
	adapter = to_i2c_adapter(client->dev.parent);
 	result = i2c_check_functionality(adapter,
 					 I2C_FUNC_SMBUS_BYTE |
 					 I2C_FUNC_SMBUS_BYTE_DATA);
	assert(result);

	result = kxtik_init_client(client);/* Initialize the kxtik chip */
	assert(result==0);
	
	dprintk(DEBUG_INIT, "<%s> kxtik_init_client result %d\n", __func__, result);
	if(result != 0)
	{
		printk("<%s> init err !", __func__);
		return result;
	}

	hwmon_dev = hwmon_device_register(&client->dev);
	assert(!(IS_ERR(hwmon_dev)));

	dev_info(&client->dev, "build time %s %s\n", __DATE__, __TIME__);
  
	/*input poll device register */
	kxtik_idev = input_allocate_polled_device();
	if (!kxtik_idev) {
		dev_err(&client->dev, "alloc poll device failed!\n");
		result = -ENOMEM;
		return result;
	}
	
	kxtik_idev->poll = kxtik_dev_poll;
	kxtik_idev->poll_interval = POLL_INTERVAL;
	kxtik_idev->poll_interval_max = POLL_INTERVAL_MAX;
	idev = kxtik_idev->input;
	idev->name = KXTIK_DRV_NAME;
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);
        mutex_init(&data->interval_mutex);

	input_set_abs_params(idev, ABS_X, -ACCEL_G_MAX, ACCEL_G_MAX, ACCEL_FUZZ, ACCEL_FLAT);
	input_set_abs_params(idev, ABS_Y, -ACCEL_G_MAX, ACCEL_G_MAX, ACCEL_FUZZ, ACCEL_FLAT);
	input_set_abs_params(idev, ABS_Z, -ACCEL_G_MAX, ACCEL_G_MAX, ACCEL_FUZZ, ACCEL_FLAT);
	
	result = input_register_polled_device(kxtik_idev);
	if (result) {
		dev_err(&client->dev, "register poll device failed!\n");
		return result;
	}
	
	result = sysfs_create_group(&kxtik_idev->input->dev.kobj, &kxtik_attribute_group);
	if(result) {
		dev_err(&client->dev, "create sys failed\n");
	}

        data->client  = client;
        data->pollDev = kxtik_idev;
	i2c_set_clientdata(client, data);

#ifdef CONFIG_HAS_EARLYSUSPEND
	kxtik_data.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	kxtik_data.early_suspend.suspend = kxtik_early_suspend;
	kxtik_data.early_suspend.resume = kxtik_late_resume;
	register_early_suspend(&kxtik_data.early_suspend);
	kxtik_data.suspend_indator = 0;
#endif

	return result;
}

static int __devexit kxtik_remove(struct i2c_client *client)
{
	int result;

        result = i2c_smbus_write_byte_data(kxtik_i2c_client,ACCEL_GRP2_CTRL_REG1,\
                accel_registers[accel_grp2_ctrl_reg1] | ACCEL_GRP2_PC1_OFF);
	assert(result==0);
	hwmon_device_unregister(hwmon_dev);
#ifdef CONFIG_HAS_EARLYSUSPEND	
	  unregister_early_suspend(&kxtik_data.early_suspend);
#endif
        sysfs_remove_group(&kxtik_idev->input->dev.kobj, &kxtik_attribute_group);
	input_unregister_polled_device(kxtik_idev);
	input_free_polled_device(kxtik_idev);
	i2c_set_clientdata(kxtik_i2c_client, NULL);
	return result;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void kxtik_early_suspend(struct early_suspend *h)
{
	int result;
        result = i2c_smbus_write_byte_data(kxtik_i2c_client,ACCEL_GRP2_CTRL_REG1,\
                accel_registers[accel_grp2_ctrl_reg1] | ACCEL_GRP2_PC1_OFF);               
	assert(result==0);
	
	kxtik_idev->input->close(kxtik_idev->input);
	return;
}

static void kxtik_late_resume(struct early_suspend *h)
{
	int result;
        result = i2c_smbus_write_byte_data(kxtik_i2c_client,ACCEL_GRP2_CTRL_REG1,\
                accel_registers[accel_grp2_ctrl_reg1] | ACCEL_GRP2_PC1_ON);
	assert(result==0);
	
	kxtik_idev->input->open(kxtik_idev->input);
	return;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static const struct i2c_device_id kxtik_id[] = {
	{ KXTIK_DRV_NAME, 1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, kxtik_id);

static struct i2c_driver kxtik_driver = {
	.class  = I2C_CLASS_HWMON,
	.driver = {
		.name	= KXTIK_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe	        = kxtik_probe,
	.remove	        = __devexit_p(kxtik_remove),
	.id_table       = kxtik_id,
	.address_list	= normal_i2c,
};

static int __init kxtik_init(void)
{
	int ret = -1;
	dprintk(DEBUG_INIT, "======%s=========. \n", __func__);
	
	if(input_fetch_sysconfig_para(&(config_info.input_type))){
		printk("%s: err.\n", __func__);
		return -1;
	}
	
	if(config_info.sensor_used == 0){
		printk("*** used set to 0 !\n");
		printk("*** if use sensor,please put the sys_config.fex gsensor_used set to 1. \n");
		return 0;
	}
	
	kxtik_driver.detect = gsensor_detect;

	ret = i2c_add_driver(&kxtik_driver);
	if (ret < 0) {
		printk("add kxtik i2c driver failed\n");
		return -ENODEV;
	}
	dprintk(DEBUG_INIT, "add kxtik i2c driver\n");

	return ret;
}

static void __exit kxtik_exit(void)
{
	dprintk(DEBUG_INIT, "remove kxtik i2c driver.\n");
	i2c_del_driver(&kxtik_driver);
}

MODULE_AUTHOR("Chen Gang <gang.chen@freescale.com>");
MODULE_DESCRIPTION("KXTIK 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

module_init(kxtik_init);
module_exit(kxtik_exit);

