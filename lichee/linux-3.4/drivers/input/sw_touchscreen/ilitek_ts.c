/* 
 * drivers/input/touchscreen/ilitek_ts.c
 *
 * FocalTech ilitek TouchScreen driver. 
 *
 * Copyright (c) 2013  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *	note: only support mulititouch	Wenfs 2010-10-01
 *  for this touchscreen to work, it's slave addr must be set to 0x7e | 0x70
 */
#include <linux/i2c.h>
#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif
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
#include <linux/time.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/gpio.h> 
#include <linux/init-input.h>
#include "ilitek_ts.h"


#define TOUCH_POINT    0x80
#define TOUCH_KEY      0xC0
#define RELEASE_KEY    0x40
#define RELEASE_POINT    0x00
//#define ROTATE_FLAG

// definitions
#define ILITEK_I2C_RETRY_COUNT			3


// i2c command for ilitek touch screen
#define ILITEK_TP_CMD_READ_DATA			0x10
#define ILITEK_TP_CME_GET_SLEEP                 0x30
#define ILITEK_TP_CMD_READ_SUB_DATA		0x11
#define ILITEK_TP_CMD_GET_RESOLUTION		0x20
#define ILITEK_TP_CMD_GET_FIRMWARE_VERSION	0x40
#define ILITEK_TP_CMD_GET_PROTOCOL_VERSION	0x42
#define	ILITEK_TP_CMD_CALIBRATION			0xCC
#define	ILITEK_TP_CMD_CALIBRATION_STATUS	0xCD
#define ILITEK_TP_CMD_ERASE_BACKGROUND		0xCE

// define the application command
#define ILITEK_IOCTL_BASE                       100
#define ILITEK_IOCTL_I2C_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 0, unsigned char*)
#define ILITEK_IOCTL_I2C_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 1, int)
#define ILITEK_IOCTL_I2C_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 2, unsigned char*)
#define ILITEK_IOCTL_I2C_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 3, int)
#define ILITEK_IOCTL_USB_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 4, unsigned char*)
#define ILITEK_IOCTL_USB_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 5, int)
#define ILITEK_IOCTL_USB_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 6, unsigned char*)
#define ILITEK_IOCTL_USB_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 7, int)
#define ILITEK_IOCTL_I2C_UPDATE_RESOLUTION      _IOWR(ILITEK_IOCTL_BASE, 8, int)
#define ILITEK_IOCTL_USB_UPDATE_RESOLUTION      _IOWR(ILITEK_IOCTL_BASE, 9, int)
#define ILITEK_IOCTL_I2C_SET_ADDRESS            _IOWR(ILITEK_IOCTL_BASE, 10, int)
#define ILITEK_IOCTL_I2C_UPDATE                 _IOWR(ILITEK_IOCTL_BASE, 11, int)
#define ILITEK_IOCTL_STOP_READ_DATA             _IOWR(ILITEK_IOCTL_BASE, 12, int)
#define ILITEK_IOCTL_START_READ_DATA            _IOWR(ILITEK_IOCTL_BASE, 13, int)
#define ILITEK_IOCTL_GET_INTERFANCE				_IOWR(ILITEK_IOCTL_BASE, 14, int)//default setting is i2c interface
#define ILITEK_IOCTL_I2C_SWITCH_IRQ				_IOWR(ILITEK_IOCTL_BASE, 15, int)

static void ilitek_set_input_param(struct input_dev*, int, int, int);
static int ilitek_i2c_read_tp_info(void);


#define FOR_TSLIB_TEST
//#define TOUCH_KEY_SUPPORT
#ifdef TOUCH_KEY_SUPPORT
#define TOUCH_KEY_FOR_EVB13
//#define TOUCH_KEY_FOR_ANGDA
#ifdef TOUCH_KEY_FOR_ANGDA
#define TOUCH_KEY_X_LIMIT	        (60000)
#define TOUCH_KEY_NUMBER	        (4)
#endif
#ifdef TOUCH_KEY_FOR_EVB13
#define TOUCH_KEY_LOWER_X_LIMIT	        (848)
#define TOUCH_KEY_HIGHER_X_LIMIT	(852)
#define TOUCH_KEY_NUMBER	        (5)
#endif
#endif

#define CONFIG_SUPPORT_FTS_CTP_UPG
#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
unsigned char CTPM_FW[]=
{	
 	#include "ilitek.ili"
};

static bool flag;
#endif
int touch_key_hold_press = 0;
int touch_key_code[] = {KEY_MENU,KEY_HOME,KEY_BACK,KEY_VOLUMEDOWN,KEY_VOLUMEUP};
int touch_key_press[] = {0, 0, 0, 0, 0};

        
struct i2c_dev{
        struct list_head list;	
        struct i2c_adapter *adap;
        struct device *dev;
};

static struct class *i2c_dev_class;
static LIST_HEAD (i2c_dev_list);
static DEFINE_SPINLOCK(i2c_dev_list_lock);

#define ilitek_NAME	"ilitek_ts"

static struct i2c_client *this_client;

#ifdef TOUCH_KEY_SUPPORT
static int key_tp  = 0;
static int key_val = 0;
#endif

/*********************************************************************************************/
#define CTP_IRQ_NUMBER                  (config_info.int_number)
#define CTP_IRQ_MODE			(TRIG_EDGE_NEGATIVE)
#define SCREEN_MAX_X			(screen_max_x)
#define SCREEN_MAX_Y			(screen_max_y)
#define CTP_NAME			 ilitek_NAME
#define PRESS_MAX			(255)

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;
static __u32 twi_id = 0;
static bool is_suspend = false;

static struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
};

static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("[CTP]:"fmt, ## arg)
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);
/*********************************************************************************************/
/*------------------------------------------------------------------------------------------*/        
/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x41,I2C_CLIENT_END};

static void ilitek_resume_events(struct work_struct *work);
struct workqueue_struct *ilitek_resume_wq;
static DECLARE_WORK(ilitek_resume_work, ilitek_resume_events);

static void ilitek_init_events(struct work_struct *work);
struct workqueue_struct *ilitek_wq;
static DECLARE_WORK(ilitek_init_work, ilitek_init_events);
/*------------------------------------------------------------------------------------------*/ 

static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;

	if(twi_id == adapter->nr){
                dprintk(DEBUG_INIT,"%s: addr= %x\n",__func__,client->addr);
                ret = i2c_test(client);
                if(!ret){
        		printk("%s:I2C connection might be something wrong \n",__func__);
        		return -ENODEV;
		}else{
			strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
			return 0;
	        }

	}else{
		return -ENODEV;
	}
}

static struct i2c_dev *get_free_i2c_dev(struct i2c_adapter *adap) 
{
	struct i2c_dev *i2c_dev;

	if (adap->nr >= I2C_MINORS){
		dprintk(DEBUG_OTHERS_INFO,"i2c-dev:out of device minors (%d) \n",adap->nr);
		return ERR_PTR (-ENODEV);
	}

	i2c_dev = kzalloc(sizeof(*i2c_dev), GFP_KERNEL);
	if (!i2c_dev){
		return ERR_PTR(-ENOMEM);
	}
	i2c_dev->adap = adap;

	spin_lock(&i2c_dev_list_lock);
	list_add_tail(&i2c_dev->list, &i2c_dev_list);
	spin_unlock(&i2c_dev_list_lock);
	
	return i2c_dev;
}

struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
	u16	x3;
	u16	y3;
	u16	x4;
	u16	y4;
	u16	x5;
	u16	y5;
	u16	pressure;
	s16     touch_ID1;
	s16     touch_ID2;
	s16     touch_ID3;
	s16     touch_ID4;
	s16     touch_ID5;
        u8      touch_point;
};

struct ilitek_ts_data {
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
	int protocol_ver;
	// maximum x
        int max_x;
        // maximum y
        int max_y;
	// maximum touch point
	int max_tp;
	// maximum key button
	int max_btn;
        // the total number of x channel
        int x_ch;
        // the total number of y channel
        int y_ch;

#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
	unsigned char firmware_ver[4];
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
};
static char DBG_FLAG;
static char Report_Flag;


/* ---------------------------------------------------------------------
*
*   Focal Touch panel upgrade related driver
*
*
----------------------------------------------------------------------*/

typedef enum
{
	ERR_OK,
	ERR_MODE,
	ERR_READID,
	ERR_ERASE,
	ERR_STATUS,
	ERR_ECC,
	ERR_DL_ERASE_FAIL,
	ERR_DL_PROGRAM_FAIL,
	ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

/*
description
	send message to i2c adaptor
parameter
	client
		i2c client
	msgs
		i2c message
	cnt
		i2c message count
return
	>= 0 if success
	others if error
*/
static int ilitek_i2c_transfer(struct i2c_client *client, struct i2c_msg *msgs, int cnt)
{
	int ret, count=ILITEK_I2C_RETRY_COUNT;
	while(count >= 0){
		count-= 1;
		ret = i2c_transfer(client->adapter, msgs, cnt);
		if(ret < 0){
			//msleep(500);
			msleep(500);
			continue;
		}
		break;
	}
	return ret;
}

static int yftech_ilitek_i2c_transfer(struct i2c_client *client, struct i2c_msg *msgs, int cnt)
{
	int ret, count=ILITEK_I2C_RETRY_COUNT;
	while(count >= 0){
		count-= 1;
		ret = i2c_transfer(client->adapter, msgs, cnt);
		if(ret < 0){
			msleep(10);
			continue;
		}
		break;
	}
	return ret;
}
/*
description
	open function for character device driver
prarmeters
	inode
	    inode
	filp
	    file pointer
return
	status
*/
static int ilitek_file_open(struct inode *inode, struct file *filp)
{
	dprintk(DEBUG_INIT,"%s\n",__func__);
	return 0; 
}
/*
description
	read data from i2c device
parameter
	client
		i2c client data
	addr
		i2c address
	data
		data for transmission
	length
		data length
return
	status
*/
static int ilitek_i2c_read(struct i2c_client *client,uint8_t cmd, uint8_t *data, int length)
{
	int ret;
    	struct i2c_msg msgs[] = {
		{.addr = client->addr, .flags = 0, .len = 1, .buf = &cmd,},
    	};
   	struct i2c_msg cmds[] = {
		{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = data,}
	};
	
    	ret = ilitek_i2c_transfer(client, msgs, 1);
	if(ret < 0){
		dprintk(DEBUG_INIT,"%s,i2c send command error. ret %d\n",__func__,ret);
	}
    	ret = ilitek_i2c_transfer(client, cmds, 1);
	if(ret < 0){
		dprintk(DEBUG_INIT,"%s, i2c read error, ret %d\n", __func__, ret);
	}
	return ret;
}
/*
description
	calibration function
prarmeters
	count
	    buffer length
return
	status
*/
static int ilitek_i2c_calibration(size_t count)
{

	int ret;
	unsigned char buffer[128]={0};
	struct i2c_msg msgs[] = {
		{.addr = this_client->addr, .flags = 0, .len = count, .buf = buffer,}
	};
	
	buffer[0] = ILITEK_TP_CMD_ERASE_BACKGROUND;
	msgs[0].len = 1;
	ret = ilitek_i2c_transfer(this_client, msgs, 1);
	if(ret < 0){
		dprintk(DEBUG_INIT,"%s, i2c erase background, failed\n", __func__);
	}
	else{
		dprintk(DEBUG_INIT,"%s, i2c erase background, success\n", __func__);
	}

	buffer[0] = ILITEK_TP_CMD_CALIBRATION;
	msgs[0].len = 1;
	msleep(2000);
	ret = ilitek_i2c_transfer(this_client, msgs, 1);
	msleep(1000);
	return ret;
}

static int ilitek_i2c_calibration_status(size_t count)
{
	int ret;
	unsigned char buffer[128]={0};
	struct i2c_msg msgs[] = {
		{.addr = this_client->addr, .flags = 0, .len = count, .buf = buffer,}
	};
	buffer[0] = ILITEK_TP_CMD_CALIBRATION_STATUS;
	ilitek_i2c_transfer(this_client, msgs, 1);
	msleep(500);
	ilitek_i2c_read(this_client, ILITEK_TP_CMD_CALIBRATION_STATUS, buffer, 1);
	dprintk(DEBUG_INIT,"%s, i2c calibration status:0x%X\n",__func__,buffer[0]);
	ret=buffer[0];
	return ret;
}
/*
description
	write function for character device driver
prarmeters
	filp
	    file pointer
	buf
	    buffer
	count
	    buffer length
	f_pos
	    offset
return
	status
*/
static ssize_t ilitek_file_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	unsigned char buffer[128]={0};

	// check the buffer size whether it exceeds the local buffer size or not
	if(count > 128){
		dprintk(DEBUG_INIT,"%s, buffer exceed 128 bytes\n", __func__);
		return -1;
	}

	// copy data from user space
	ret = copy_from_user(buffer, buf, count-1);
	if(ret < 0){
		dprintk(DEBUG_INIT,"%s, copy data from user space, failed", __func__);
		return -1;
	}

	// parsing command
	if(strcmp(buffer, "calibrate") == 0){
		ret=ilitek_i2c_calibration(count);
		if(ret < 0){
			dprintk(DEBUG_INIT,"%s, i2c send calibration command, failed\n", __func__);
		}
		else{
			dprintk(DEBUG_INIT,"%s, i2c send calibration command, success\n", __func__);
		}
		ret=ilitek_i2c_calibration_status(count);
		if(ret == 0x5A){
			dprintk(DEBUG_INIT,"%s, i2c calibration, success\n", __func__);
		}
		else if (ret == 0xA5){
			dprintk(DEBUG_INIT,"%s, i2c calibration, failed\n", __func__);
		}
		else{
			dprintk(DEBUG_INIT,"%s, i2c calibration, i2c protoco failed\n", __func__);
		}
		return count;
	}else if(strcmp(buffer, "dbg") == 0){
		DBG_FLAG=!DBG_FLAG;
		printk("%s, %s message(%X).\n",__func__,DBG_FLAG?"Enabled":"Disabled",DBG_FLAG);
	}else if(strcmp(buffer, "info") == 0){
		ilitek_i2c_read_tp_info();
	}else if(strcmp(buffer, "report") == 0){
		Report_Flag=!Report_Flag;
	}
	return -1;
}
/*
description
        ioctl function for character device driver
prarmeters
	inode
		file node
        filp
            file pointer
        cmd
            command
        arg
            arguments
return
        status
*/
static long ilitek_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	static unsigned char buffer[64]={0};
	static int len=0;
	int ret;
	struct i2c_msg msgs[] = {
		{.addr = this_client->addr, .flags = 0, .len = len, .buf = buffer,}
        };

	// parsing ioctl command
	switch(cmd){
		case ILITEK_IOCTL_I2C_WRITE_DATA:
			ret = copy_from_user(buffer, (unsigned char*)arg, len);
			if(ret < 0){
						dprintk(DEBUG_INIT,"%s, copy data from user space, failed\n", __func__);
						return -1;
				}
			ret = ilitek_i2c_transfer(this_client, msgs, 1);
			if(ret < 0){
				dprintk(DEBUG_INIT,"%s, i2c write, failed\n", __func__);
				return -1;
			}
			break;
		case ILITEK_IOCTL_I2C_READ_DATA:
			msgs[0].flags = I2C_M_RD;
	
			ret = ilitek_i2c_transfer(this_client, msgs, 1);
			if(ret < 0){
				dprintk(DEBUG_INIT,"%s, i2c read, failed\n", __func__);
				return -1;
			}
			ret = copy_to_user((unsigned char*)arg, buffer, len);
			
			if(ret < 0){
				dprintk(DEBUG_INIT,"%s, copy data to user space, failed\n", __func__);
				return -1;
			}
			break;
		case ILITEK_IOCTL_I2C_WRITE_LENGTH:
		case ILITEK_IOCTL_I2C_READ_LENGTH:
			len = arg;
			break;
		case ILITEK_IOCTL_I2C_UPDATE_RESOLUTION:
		case ILITEK_IOCTL_I2C_SET_ADDRESS:
		case ILITEK_IOCTL_I2C_UPDATE:
			break;
		case ILITEK_IOCTL_START_READ_DATA:
			break;
		case ILITEK_IOCTL_STOP_READ_DATA:
			break;
		case ILITEK_IOCTL_I2C_SWITCH_IRQ:
			break;	
		default:
			return -1;
	}
    	return 0;
}
/*
description
	read function for character device driver
prarmeters
	filp
	    file pointer
	buf
	    buffer
	count
	    buffer length
	f_pos
	    offset
return
	status
*/
static ssize_t ilitek_file_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}
/*
description
	close function
prarmeters
	inode
	    inode
	filp
	    file pointer
return
	status
*/
static int ilitek_file_close(struct inode *inode, struct file *filp)
{
        return 0;
}

/*
description
	set input device's parameter
prarmeters
	input
		input device data
	max_tp
		single touch or multi touch
	max_x
		maximum	x value
	max_y
		maximum y value
return
	nothing
*/
static void ilitek_set_input_param(struct input_dev *input, int max_tp, int max_x, int max_y)
{
	int key;
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	#ifndef ROTATE_FLAG    
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, max_x, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	#else
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, max_y, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, max_x, 0, 0);
	#endif
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, max_tp, 0, 0);
	for(key=0; key<sizeof(touch_key_code); key++){
        	if(touch_key_code[key] <= 0){
            		continue;
		}
        	set_bit(touch_key_code[key] & KEY_MAX, input->keybit);
	}
	input->name = CTP_NAME;
	//input->id.bustype = BUS_I2C;
	
	/* YF */
    	//input->phys = "I2C";	
	//input->dev.parent = &(i2c.client)->dev;
	
}
/*
description
	read data from i2c device with delay between cmd & return data
parameter
	client
		i2c client data
	addr
		i2c address
	data
		data for transmission
	length
		data length
return
	status
*/
static int ilitek_i2c_read_info(struct i2c_client *client,uint8_t cmd, uint8_t *data, int length)
{
	int ret;
	struct i2c_msg msgs_cmd[] = {
	{.addr = client->addr, .flags = 0, .len = 1, .buf = &cmd,},
	};
	
	struct i2c_msg msgs_ret[] = {
	{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = data,}
	};

	msleep(100);
	//msleep(10);
	ret = yftech_ilitek_i2c_transfer(client, msgs_cmd, 1);		// elf
	if(ret < 0){
		dprintk(DEBUG_INIT,"%s, i2c read error, ret %d\n", __func__, ret);
		return ret;
	}
	
	msleep(10);
	ret = ilitek_i2c_transfer(client, msgs_ret, 1);		// elf
	if(ret < 0){
		dprintk(DEBUG_INIT,"%s, i2c read error, ret %d\n", __func__, ret);		
	}
	
	//dprintk(DEBUG_INIT,"%s, Driver Vesrion: %s\n", __func__, DRIVER_VERSION);
	return ret;
}

/*
description
	read touch information
parameters
	none
return
	status
*/
static int ilitek_i2c_read_tp_info(void)
{
	int res_len;
	unsigned char buf[32]={0};
	struct ilitek_ts_data *ilitek_ts = i2c_get_clientdata(this_client);
	
	// read firmware version
	//msleep(100);
        if(ilitek_i2c_read_info(this_client, ILITEK_TP_CMD_GET_FIRMWARE_VERSION, buf, 4) < 0){
		return -1;
	}

#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
	ilitek_ts->firmware_ver[0] = buf[0];
	ilitek_ts->firmware_ver[1] = buf[1];
	ilitek_ts->firmware_ver[2] = buf[2];
	ilitek_ts->firmware_ver[3] = buf[3];
#endif
	dprintk(DEBUG_INIT,"%s, firmware version %d.%d.%d.%d\n", __func__, buf[0], buf[1], buf[2], buf[3]);

	// read protocol version
        res_len = 6;
	//msleep(100);
        if(ilitek_i2c_read_info(this_client, ILITEK_TP_CMD_GET_PROTOCOL_VERSION, buf, 2) < 0){
		return -1;
	}	

        ilitek_ts->protocol_ver = (((int)buf[0]) << 8) + buf[1];
#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
	if(ilitek_ts->protocol_ver > 1){
		if(buf[1] >2)
			flag = true;
		else 
			flag = false;
	}
	else{
		if(buf[1]>4)
			flag = true;
		else
			flag = false;
	}
#endif
        dprintk(DEBUG_INIT,"%s, protocol version: %d.%d\n", __func__,buf[0],buf[1]);
        if(ilitek_ts->protocol_ver >= 0x200){
               	res_len = 8;
        }

        // read touch resolution
	ilitek_ts->max_tp = 2;
	//msleep(100);
        if(ilitek_i2c_read_info(this_client, ILITEK_TP_CMD_GET_RESOLUTION, buf, res_len) < 0){
		return -1;
	}
        if(ilitek_ts->protocol_ver >= 0x200){
                // maximum touch point
                ilitek_ts->max_tp = buf[6];
                // maximum button number
                ilitek_ts->max_btn = buf[7];
        }
	// calculate the resolution for x and y direction
        ilitek_ts->max_x = buf[0];
        ilitek_ts->max_x+= ((int)buf[1]) * 256;
        ilitek_ts->max_y = buf[2];
        ilitek_ts->max_y+= ((int)buf[3]) * 256;
        ilitek_ts->x_ch = buf[4];
        ilitek_ts->y_ch = buf[5];
        dprintk(DEBUG_INIT,"%s, max_x: %d, max_y: %d, ch_x: %d, ch_y: %d\n", 
		__func__, ilitek_ts->max_x, ilitek_ts->max_y, ilitek_ts->x_ch, ilitek_ts->y_ch);
        dprintk(DEBUG_INIT,"%s, max_tp: %d, max_btn: %d\n", __func__, ilitek_ts->max_tp, ilitek_ts->max_btn);
	return 0;
}
/* YF END */
/*
description
	upgrade F/W
prarmeters
		
return
	status
*/
static int ilitek_upgrade_firmware(void)
{
	struct ilitek_ts_data *ilitek_ts = i2c_get_clientdata(this_client);
	int ret=0,upgrade_status=0,i,j,k = 0,ap_len = 0,df_len = 0;
	unsigned char buffer[128]={0};
	unsigned long ap_startaddr,df_startaddr,ap_endaddr,df_endaddr,ap_checksum = 0,df_checksum = 0;
	unsigned char firmware_ver[4];
	struct i2c_msg msgs[] = {
		{.addr = this_client->addr, .flags = 0, .len = 0, .buf = buffer,}
	};
	if(flag)
		printk("-------- ILITEK BL1010 upgrade -----------\n");
	else
		printk("-------- ILITEK BL1003 upgrade -----------\n");
	ap_startaddr = ( CTPM_FW[0] << 16 ) + ( CTPM_FW[1] << 8 ) + CTPM_FW[2];
	ap_endaddr = ( CTPM_FW[3] << 16 ) + ( CTPM_FW[4] << 8 ) + CTPM_FW[5];
	ap_checksum = ( CTPM_FW[6] << 16 ) + ( CTPM_FW[7] << 8 ) + CTPM_FW[8];
	df_startaddr = ( CTPM_FW[9] << 16 ) + ( CTPM_FW[10] << 8 ) + CTPM_FW[11];
	df_endaddr = ( CTPM_FW[12] << 16 ) + ( CTPM_FW[13] << 8 ) + CTPM_FW[14];
	df_checksum = ( CTPM_FW[15] << 16 ) + ( CTPM_FW[16] << 8 ) + CTPM_FW[17];
	firmware_ver[0] = CTPM_FW[18];
	firmware_ver[1] = CTPM_FW[19];
	firmware_ver[2] = CTPM_FW[20];
	firmware_ver[3] = CTPM_FW[21];
	df_len = ( CTPM_FW[22] << 16 ) + ( CTPM_FW[23] << 8 ) + CTPM_FW[24];
	ap_len = ( CTPM_FW[25] << 16 ) + ( CTPM_FW[26] << 8 ) + CTPM_FW[27];
    printk("i2c.firmware_ver[%d]=%x,firmware_ver[%d]=%x,\n",0,ilitek_ts->firmware_ver[0],1,firmware_ver[1]);
    printk("i2c.firmware_ver[%d]=%x,firmware_ver[%d]=%x,\n",2,ilitek_ts->firmware_ver[2],3,firmware_ver[3]);
	printk("ap_startaddr=0x%ld,ap_endaddr=0x%ld,ap_checksum=0x%ld\n",ap_startaddr,ap_endaddr,ap_checksum);
	printk("df_startaddr=0x%ld,df_endaddr=0x%ld,df_checksum=0x%ld\n",df_startaddr,df_endaddr,df_checksum);	
	for(i=0;i<4;i++){
		if(ilitek_ts->firmware_ver[i] != firmware_ver[i]){				
			printk("i2c.firmware_ver[%d]=%d,firmware_ver[%d]=%d\n",i,ilitek_ts->firmware_ver[i],i,firmware_ver[i]);
			break;
			}
			}
	if(i>=4)
		return 1;

	for(i=0;i<5;i++){

		if(flag){
			buffer[0]=0xc4;
			buffer[1]=0x5A;
			buffer[2]=0xA5;
			msgs[0].len=3;
			}
		else{
		
			buffer[0]=0xc0;
			msgs[0].len = 1;
		}
		ilitek_i2c_read(this_client, 0xc0, buffer, 1);	
		msleep(30);
		if(buffer[0] != 0x55){					
			buffer[0]=0xc4;
			msgs[0].len = 1;
			ret = ilitek_i2c_transfer(this_client, msgs, 1);
			msleep(30);
			buffer[0]=0xc2; 
			ret = ilitek_i2c_transfer(this_client, msgs, 1);
			msleep(100+i*10);
			}
			else
			    break;
	}
	if(buffer[0] != 0x55 && i== 5){
		printk(" ----  mode:0x%x,i:%d  ---  \n",buffer[0],i);
		printk(" ---  start: can't set to BL mode ! --- \n");
		return -1;
		}
	else{
		printk("  ---- mode :0x%x,i:%d  --- \n",buffer[0],i);
		printk("  ---- start :set mode to BL mode! ---- \n");
		}


	msleep(30);
	printk("ILITEK:%s, upgrade firmware...\n", __func__);
	buffer[0]=0xc4;
	msgs[0].len = 10;
	buffer[1] = 0x5A;
	buffer[2] = 0xA5;
	buffer[3] = 0;
	buffer[4] = CTPM_FW[3];
	buffer[5] = CTPM_FW[4];
	buffer[6] = CTPM_FW[5];
	buffer[7] = CTPM_FW[6];
	buffer[8] = CTPM_FW[7];
	buffer[9] = CTPM_FW[8];
	ret = ilitek_i2c_transfer(this_client, msgs, 1);
	msleep(30);

	buffer[0]=0xc4;
	msgs[0].len = 10;
	buffer[1] = 0x5A;
	buffer[2] = 0xA5;
	buffer[3] = 1;
	buffer[4] = 0;
	buffer[5] = 0x7A;
	buffer[6] = 0x22;
	buffer[7] = 0;
	buffer[8] = 0;
	buffer[9] = 0x04;
	ret = ilitek_i2c_transfer(this_client, msgs, 1);
	msleep(30);

	j=0;
	for(i=0; i < df_len; i+=32){
		j+= 1;
		if(flag){
		if((j % 8) == 2){
			//if(j != 1)			   
			    msleep(30);

				}
			}
		else{
			if((j % 16) == 1){
				msleep(60);
				}
			}
		for(k=0; k<32; k++){
			buffer[1 + k] = CTPM_FW[i + 32 + k];
		}

		buffer[0]=0xc3;
		msgs[0].len = 33;
		ret = ilitek_i2c_transfer(this_client, msgs, 1);		
		upgrade_status = (i * 100) / df_len;
		if(flag)
		msleep(20);
		else
		msleep(10);
		printk("%cILITEK: Firmware Upgrade(Data flash), %02d%c. ",0x0D,upgrade_status,'%');
	}

	buffer[0]=0xc4;
	msgs[0].len = 10;
	buffer[1] = 0x5A;
	buffer[2] = 0xA5;
	buffer[3] = 0;
	buffer[4] = CTPM_FW[3];
	buffer[5] = CTPM_FW[4];
	buffer[6] = CTPM_FW[5];
	buffer[7] = CTPM_FW[6];
	buffer[8] = CTPM_FW[7];
	buffer[9] = CTPM_FW[8];
	ret = ilitek_i2c_transfer(this_client, msgs, 1);
	msleep(30);

	j=0;
	for(i = 0; i < ap_len; i+=32){
		j+= 1;
		if(flag){
		if((j % 8) == 2){
			//if(j != 1)			   
			    msleep(30);

				}
			}
		else{
			if((j % 16) == 1){
				msleep(60);
				}
			}

		for(k=0; k<32; k++){
			buffer[1 + k] = CTPM_FW[i + df_len + 32 + k];
		}

		buffer[0]=0xc3;
		msgs[0].len = 33;
		ret = ilitek_i2c_transfer(this_client, msgs, 1);		
		upgrade_status = (i * 100) / ap_len;
		msleep(10);
		printk("%cILITEK: Firmware Upgrade(AP), %02d%c. ",0x0D,upgrade_status,'%');
	}

	printk("ILITEK:%s, upgrade firmware completed\n", __func__);
	/*
	buffer[0]=0xc4;
	msgs[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	msleep(30);
	buffer[0]=0xc1;
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	msleep(100);
	*/
	buffer[0]=0xc0;
	msgs[0].len = 1;
	ilitek_i2c_read(this_client, 0xc0, buffer, 1);
	msleep(30);
    //0xc0 check the mode : return 0x5A is AP mode ,0x55 is BL mode
    for(i=0;i<5;i++){
	if(buffer[0]!=0x5A){
		buffer[0]=0xc4;
		buffer[1]=0x5A;
		buffer[2]=0xA5;
		msgs[0].len = 3;
		ret = ilitek_i2c_transfer(this_client, msgs, 1);
		msleep(30);
		buffer[0]=0xc1; //0xc1 is set AP mode
		ret = ilitek_i2c_transfer(this_client, msgs, 1);
		msleep(100+i*20);
		buffer[0]=0xc0;
		msgs[0].len = 1;
		ilitek_i2c_read(this_client, 0xc0, buffer, 1);
		}
	else 
		return 2;
    }
	if(buffer[0]!=0x5A && i==5){ 
		printk(" ---- mode:0x%x,i:%d ---- \n",buffer[0],i);
		printk(" ---- can't set AP mode! ----\n");
		return -1;
		}
	else{
		printk("  ---- mode :0x%x,i:%d  --- \n",buffer[0],i);
		printk("  ---- end :set mode to AP mode! ---- \n");
		}

	return 2;
}
/*
description
	process i2c data and then report to kernel
parameters
	none
return
	status
*/
static int ilitek_i2c_process_and_report(void)
{
	int i, len, ret, x=0, y=0,key;//org_x=0, org_y=0,
	int mult_tp_id,j;
	//unsigned char key_id=0,key_flag=1;
	static unsigned char last_id=0;
	
    	unsigned char buf[9]={0};
	unsigned char tp_id;
	//static unsigned char touch_pt[20];
	//unsigned char touch_flag=0;
	//unsigned char release_counter=0;
    	//int xx=0,yy=0;
	struct ilitek_ts_data *ilitek_ts = i2c_get_clientdata(this_client);
	struct input_dev *input = ilitek_ts->input_dev;
	// multipoint process
	if(ilitek_ts->protocol_ver & 0x200){
		// read i2c data from device
		ret = ilitek_i2c_read(this_client, ILITEK_TP_CMD_READ_DATA, buf, 1);
		if(ret < 0){
			return ret;
		}

		len = buf[0];  
     		//dprintk(DEBUG_INIT,"buf[0] = %d", buf[0]);
		ret = 1;
		// read touch point
		for(i=0; i<len; i++){
			// parse point
			if(ilitek_i2c_read(this_client, ILITEK_TP_CMD_READ_SUB_DATA, buf, 5)){
				x = (((int)buf[1]) << 8) + buf[2];
				y = (((int)buf[3]) << 8) + buf[4];
				//pr_info("buf[1] = %d", buf[1]);
				//pr_info("buf[2] = %d", buf[2]);
				//pr_info("buf[3] = %d", buf[3]);
				//pr_info("buf[4] = %d", buf[4]);
				//printk("---------x = %d,---y = %d\n", x, y);
				//printk("---x = =%x,---y==%x\n",x,y);
				if (Report_Flag!=0){
					//printk("%s(%d):",__func__,__LINE__);
					for (j=0;j<16;j++)
						dprintk(DEBUG_INIT,"%02X,",buf[j]);
					dprintk(DEBUG_INIT,"\n");
				}
				mult_tp_id = buf[0];
			 //   printk("---mult_tp_id 0x= %x\n", mult_tp_id);
				switch ((mult_tp_id & 0xC0)){
					case TOUCH_POINT:
						{		
                               		        dprintk(DEBUG_INIT,"ILITEK:x = %d,y = %d\n", x, y);
						// report to android system
						input_report_key(input, BTN_TOUCH,  1);//Luca 20120402
						input_event(input, EV_ABS, ABS_MT_TRACKING_ID, (buf[0] & 0x3F)-1);
						input_event(input, EV_ABS, ABS_MT_POSITION_X, x);
						input_event(input, EV_ABS, ABS_MT_POSITION_Y, y);
						input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 15);
						input_mt_sync(input);
						ret=0;
						//touch_pt[i]=255;
						}
						break;	
					case RELEASE_POINT:			
						break;
					default:
						break;
				}
			}
		}
		if(len== 0){
			dprintk(DEBUG_INIT,"Release3, ID=%02X, X=%04d, Y=%04d\n",buf[0]  & 0x3F, x,y);
		  //   printk("release key \n");
			input_report_key(input, BTN_TOUCH,  0);
			//input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
			input_mt_sync(input);
			if (touch_key_hold_press !=0){
				for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
					if(touch_key_press[key]){
						input_report_key(input, touch_key_code[key], 0);
						touch_key_press[key] = 0;
						dprintk(DEBUG_INIT,"=========%s key release, %X, %d, %d\n", __func__, buf[0], x, y);
					}
					touch_key_hold_press=0;
				}		
			}
		}
		input_sync(input);
		dprintk(DEBUG_INIT,"%s,ret=%d\n",__func__,ret);
	}
	else{
		// read i2c data from device
		ret = ilitek_i2c_read(this_client, ILITEK_TP_CMD_READ_DATA, buf, 9);
		if(ret < 0){
			return ret;
		}

		ret = 0;

		tp_id = buf[0];
		if (Report_Flag!=0){
			dprintk(DEBUG_INIT,"%s(%d):",__func__,__LINE__);
			for (i=0;i<16;i++)
				dprintk(DEBUG_INIT,"%02X,",buf[i]);
			dprintk(DEBUG_INIT,"\n");
		}
		switch (tp_id)
		{
			case 0://release point
				{
					for(i=0; i<ilitek_ts->max_tp; i++){
						// check 
						if (!(last_id & (1<<i)))
							continue;	
							
						#ifndef ROTATE_FLAG
						x = (int)buf[1 + (i * 4)] + ((int)buf[2 + (i * 4)] * 256);
						y = (int)buf[3 + (i * 4)] + ((int)buf[4 + (i * 4)] * 256);
						#else
						org_x = (int)buf[1 + (i * 4)] + ((int)buf[2 + (i * 4)] * 256);
						org_y = (int)buf[3 + (i * 4)] + ((int)buf[4 + (i * 4)] * 256);
						x = ilitek_ts->max_y - org_y + 1;
						y = org_x + 1;					
						#endif
						touch_key_hold_press=2; //2: into available area
						input_report_key(input, BTN_TOUCH,  1);//Luca 20120402
						input_event(input, EV_ABS, ABS_MT_TRACKING_ID, i);
						input_event(input, EV_ABS, ABS_MT_POSITION_X, x+1);
						input_event(input, EV_ABS, ABS_MT_POSITION_Y, y+1);
						input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
						input_mt_sync(input);
						dprintk(DEBUG_INIT,"Last Point[%d]= %d, %d\n", buf[0]&0x3F, x, y);
						last_id=0;
					}
					input_sync(input);//20120407				
					input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
					input_mt_sync(input);
					ret = 1; // stop timer interrupt
				}
				break;
			default:
				last_id=buf[0];
				for(i=0; i<ilitek_ts->max_tp; i++){
					// check 
					if (!(buf[0] & (1<<i)))
						continue;	
						
					#ifndef ROTATE_FLAG
					x = (int)buf[1 + (i * 4)] + ((int)buf[2 + (i * 4)] * 256);
					y = (int)buf[3 + (i * 4)] + ((int)buf[4 + (i * 4)] * 256);
					#else
					org_x = (int)buf[1 + (i * 4)] + ((int)buf[2 + (i * 4)] * 256);
					org_y = (int)buf[3 + (i * 4)] + ((int)buf[4 + (i * 4)] * 256);
					x = ilitek_ts->max_y - org_y + 1;
					y = org_x + 1;					
					#endif
					{
						touch_key_hold_press=2; //2: into available area
						input_report_key(input, BTN_TOUCH,  1);//Luca 20120402
						input_event(input, EV_ABS, ABS_MT_TRACKING_ID, i);
						input_event(input, EV_ABS, ABS_MT_POSITION_X, x+1);
						input_event(input, EV_ABS, ABS_MT_POSITION_Y, y+1);
						input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
						input_mt_sync(input);
						dprintk(DEBUG_INIT,"Point[%d]= %d, %d\n", buf[0]&0x3F, x, y);
					}	
				}
				break;
		}

		input_sync(input);
	}
    return ret;
}

static void ilitek_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
	ret = ilitek_i2c_process_and_report();
	dprintk(DEBUG_INT_INFO,"%s:ret:%d\n",__func__,ret);
}

static u32 ilitek_ts_interrupt(struct ilitek_ts_data *ilitek_ts)
{
	dprintk(DEBUG_INT_INFO,"==========ilitek_ts TS Interrupt============\n"); 
	queue_work(ilitek_ts->ts_workqueue, &ilitek_ts->pen_event_work);
	return 0;
}

static void ilitek_resume_events (struct work_struct *work)
{
	ctp_wakeup(config_info.wakeup_number,0,20);
	ilitek_i2c_read_tp_info();
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
	return;
}

static int ilitek_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{	
	int ret;
	struct ilitek_ts_data *data = i2c_get_clientdata(this_client);
	uint8_t cmd = ILITEK_TP_CME_GET_SLEEP;
	struct i2c_msg msgs_cmd[] = {
	{.addr = this_client->addr, .flags = 0, .len = 1, .buf = &cmd,},
	};
	dprintk(DEBUG_SUSPEND,"==ilitek_ts_suspend=\n"); 
	is_suspend = false;	
	
	ret = ilitek_i2c_transfer(this_client, msgs_cmd, 1);
	if(ret < 0){
		printk("%s,i2c suspend send cmd error!ret is %d\n",__func__,ret);
	}
	
	flush_workqueue(ilitek_resume_wq);
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
	cancel_work_sync(&data->pen_event_work);
	flush_workqueue(data->ts_workqueue);
	
	return 0;
}
static int ilitek_ts_resume(struct i2c_client *client)
{
	dprintk(DEBUG_SUSPEND,"==CONFIG_PM:ilitek_ts_resume== \n");
	if(is_suspend == false)
	        queue_work(ilitek_resume_wq, &ilitek_resume_work);
	        
	return 0;		
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ilitek_ts_early_suspend(struct early_suspend *handler)
{
	struct ilitek_ts_data *data = i2c_get_clientdata(this_client);
	dprintk(DEBUG_SUSPEND,"==ilitek_ts_suspend=\n");
	dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND: write ilitek0X_REG_PMODE .\n");
	
#ifndef CONFIG_HAS_EARLYSUSPEND
        is_suspend = true;
#endif  
        if(is_suspend == true){ 
		int ret;
		uint8_t cmd = ILITEK_TP_CME_GET_SLEEP;
		struct i2c_msg msgs_cmd[] = {
		{.addr = this_client->addr, .flags = 0, .len = 1, .buf = &cmd,},
		};
		ret = ilitek_i2c_transfer(this_client, msgs_cmd, 1);
		if(ret < 0){
			printk("%s,i2c suspend send cmd error!ret is %d\n",__func__,ret);
		}
	        flush_workqueue(ilitek_resume_wq);
	        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
	        cancel_work_sync(&data->pen_event_work);
	        flush_workqueue(data->ts_workqueue);
	}
	
	is_suspend = true;
}

static void ilitek_ts_late_resume(struct early_suspend *handler)
{
	dprintk(DEBUG_SUSPEND,"==CONFIG_HAS_EARLYSUSPEND:ilitek_ts_resume== \n");
	
	queue_work(ilitek_resume_wq, &ilitek_resume_work);	
	is_suspend = true;
}
#endif

static void ilitek_init_events (struct work_struct *work)
{
	int ret;
	dprintk(DEBUG_INIT,"====%s begin=====.  \n", __func__);
#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
	// read touch parameter
	//ilitek_i2c_read_tp_info();
	ret = ilitek_upgrade_firmware();
	if(ret==1){
		printk("Do not need update\n");
		return;
	}
	else if(ret==2)
		printk("update end\n");
 
        ctp_wakeup(config_info.wakeup_number,0,20);
	ilitek_i2c_read_tp_info();
	//msleep(150);
#endif 
	return;
}

static int ilitek_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ilitek_ts_data *ilitek_ts;
	struct input_dev *input_dev;
	struct device *dev;
	struct i2c_dev *i2c_dev;
	int err = 0;       

	dprintk(DEBUG_INIT,"====%s begin=====.  \n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		printk("check_functionality_failed\n");
		goto exit_check_functionality_failed;
	}

	ilitek_ts = kzalloc(sizeof(*ilitek_ts), GFP_KERNEL);
	if (!ilitek_ts)	{
		err = -ENOMEM;
		printk("alloc_data_failed\n");
		goto exit_alloc_data_failed;
	}

	this_client = client;
	i2c_set_clientdata(client, ilitek_ts);

	ilitek_wq = create_singlethread_workqueue("ilitek_init");
	if (ilitek_wq == NULL) {
		printk("create ilitek_wq fail!\n");
		return -ENOMEM;
	}

	queue_work(ilitek_wq, &ilitek_init_work);

	INIT_WORK(&ilitek_ts->pen_event_work, ilitek_ts_pen_irq_work);
	ilitek_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ilitek_ts->ts_workqueue) {
		err = -ESRCH;
		printk("ts_workqueue fail!\n");
		goto exit_create_singlethread;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	ilitek_i2c_read_tp_info();
	ilitek_ts->input_dev = input_dev;
	ilitek_set_input_param(input_dev, ilitek_ts->max_tp, screen_max_x, screen_max_y);	
	//input_dev->name	= CTP_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,"ilitek_ts_probe: failed to register input device: %s\n",
		        dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

	ilitek_resume_wq = create_singlethread_workqueue("ilitek_resume");
	if (ilitek_resume_wq == NULL) {
		printk("create ilitek_resume_wq fail!\n");
		return -ENOMEM;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	printk("==register_early_suspend =\n");
	ilitek_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ilitek_ts->early_suspend.suspend = ilitek_ts_early_suspend;
	ilitek_ts->early_suspend.resume	= ilitek_ts_late_resume;
	register_early_suspend(&ilitek_ts->early_suspend);
#endif

#ifdef CONFIG_ilitek0X_MULTITOUCH
	dprintk(DEBUG_INIT,"CONFIG_ilitek0X_MULTITOUCH is defined. \n");
#endif
        int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)ilitek_ts_interrupt,ilitek_ts);
	if (!int_handle) {
		printk("ilitek_ts_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	
	ctp_set_int_port_rate(config_info.int_number, 1);
	ctp_set_int_port_deb(config_info.int_number, 0x07);
	dprintk(DEBUG_INIT,"reg clk: 0x%08x\n", readl(0xf1c20a18));

    	i2c_dev = get_free_i2c_dev(client->adapter);	
	if (IS_ERR(i2c_dev)){	
		err = PTR_ERR(i2c_dev);	
		printk("i2c_dev fail!");	
		return err;	
	}
	
	dev = device_create(i2c_dev_class, &client->adapter->dev, MKDEV(I2C_MAJOR,client->adapter->nr),
	         NULL, "aw_i2c_ts%d", client->adapter->nr);	
	if (IS_ERR(dev))	{		
			err = PTR_ERR(dev);
			printk("dev fail!\n");		
			return err;	
	}

	device_enable_async_suspend(&client->dev);
	dprintk(DEBUG_INIT,"==%s over =\n", __func__);

	return 0;

exit_irq_request_failed:
        sw_gpio_irq_free(int_handle);
        cancel_work_sync(&ilitek_resume_work);
	destroy_workqueue(ilitek_resume_wq);	
exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
        i2c_set_clientdata(client, NULL);
        cancel_work_sync(&ilitek_ts->pen_event_work);
	destroy_workqueue(ilitek_ts->ts_workqueue);
exit_create_singlethread:
	kfree(ilitek_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:        
	cancel_work_sync(&ilitek_init_work);
	destroy_workqueue(ilitek_wq);

	return err;
}

static int __devexit ilitek_ts_remove(struct i2c_client *client)
{

	struct ilitek_ts_data *ilitek_ts = i2c_get_clientdata(client);
	
	printk("==ilitek_ts_remove=\n");
	device_destroy(i2c_dev_class, MKDEV(I2C_MAJOR,client->adapter->nr));
	sw_gpio_irq_free(int_handle);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ilitek_ts->early_suspend);
#endif
	cancel_work_sync(&ilitek_resume_work);
	destroy_workqueue(ilitek_resume_wq);
	input_unregister_device(ilitek_ts->input_dev);
	input_free_device(ilitek_ts->input_dev);
	cancel_work_sync(&ilitek_ts->pen_event_work);
	destroy_workqueue(ilitek_ts->ts_workqueue);
	kfree(ilitek_ts);
    
	i2c_set_clientdata(this_client, NULL);

	return 0;

}

static const struct i2c_device_id ilitek_ts_id[] = {
	{ CTP_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ilitek_ts_id);

static struct i2c_driver ilitek_ts_driver = {
	.class          = I2C_CLASS_HWMON,
	.probe		= ilitek_ts_probe,
	.remove		= __devexit_p(ilitek_ts_remove),
	.id_table	= ilitek_ts_id,
	.suspend        = ilitek_ts_suspend,
	.resume         = ilitek_ts_resume,
	.driver	= {
		.name	= CTP_NAME,
		.owner	= THIS_MODULE,
	},
	.address_list	= normal_i2c,

};

static const struct file_operations aw_i2c_ts_fops ={	
	.owner 		= THIS_MODULE, 		
	.unlocked_ioctl = ilitek_file_ioctl,
	.read 		= ilitek_file_read,
	.write 		= ilitek_file_write,
	.open 		= ilitek_file_open,
	.release 	= ilitek_file_close,
};
static int ctp_get_system_config(void)
{  
        twi_id = config_info.twi_id;
        screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;
        if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}
static int __init ilitek_ts_init(void)
{ 
	int ret = -1;      
	dprintk(DEBUG_INIT,"***************************init begin*************************************\n");
	if (input_fetch_sysconfig_para(&(config_info.input_type))) {
		printk("%s: ctp_fetch_sysconfig_para err.\n", __func__);
		return 0;
	} else {
		ret = input_init_platform_resource(&(config_info.input_type));
		if (0 != ret) {
			printk("%s:ctp_ops.init_platform_resource err. \n", __func__);    
		}
	}	
	
	if(config_info.ctp_used == 0){
	        printk("*** ctp_used set to 0 !\n");
	        printk("*** if use ctp,please put the sys_config.fex ctp_used set to 1. \n");
	        return 0;
	}
	
        if(!ctp_get_system_config()){
                printk("%s:read config fail!\n",__func__);
                return ret;
        }

	ctp_wakeup(config_info.wakeup_number, 0, 80);
	ilitek_ts_driver.detect = ctp_detect;

	ret= register_chrdev(I2C_MAJOR,"aw_i2c_ts",&aw_i2c_ts_fops );	
	if(ret) {	
		printk("%s:register chrdev failed\n",__FILE__);	
		return ret;
	}
	
	i2c_dev_class = class_create(THIS_MODULE,"aw_i2c_dev");
	if (IS_ERR(i2c_dev_class)) {		
		ret = PTR_ERR(i2c_dev_class);		
		class_destroy(i2c_dev_class);	
	}
        ret = i2c_add_driver(&ilitek_ts_driver);
        
        dprintk(DEBUG_INIT,"****************************init end************************************\n");
	return ret;
}

static void __exit ilitek_ts_exit(void)
{
	printk("==ilitek_ts_exit==\n");
	i2c_del_driver(&ilitek_ts_driver);
	class_destroy(i2c_dev_class);
	unregister_chrdev(I2C_MAJOR, "aw_i2c_ts");
	input_free_platform_resource(&(config_info.input_type));
}

late_initcall(ilitek_ts_init);
module_exit(ilitek_ts_exit);
MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ilitek TouchScreen driver");
MODULE_LICENSE("GPL");

