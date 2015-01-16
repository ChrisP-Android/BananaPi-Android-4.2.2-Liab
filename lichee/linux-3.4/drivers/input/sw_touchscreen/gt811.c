/* drivers/input/touchscreen/gt811.c
 * 
 * 2010 - 2012 Goodix Technology.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the GOODiX's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 * Version:1.0
 * Author:andrew@goodix.com
 * Release Date:2012/05/01
 * Revision record:
 *      V1.0:2012/05/01,create file,by andrew
 */

#include <linux/irq.h>
#include "gt811.h"
#include "gt811_info.h"
#if GTP_AUTO_UPDATE
#include "gt811_firmware.h"
#endif

//extern int ctp_is_used;


static const char *goodix_ts_name = "gt811";
static struct workqueue_struct *goodix_wq;
struct i2c_client * i2c_connect_client = NULL;
static u8 config[GTP_CONFIG_LENGTH+GTP_ADDR_LENGTH] = {(u8)(GTP_REG_CONFIG_DATA>>8), (u8)GTP_REG_CONFIG_DATA, };
#if GTP_HAVE_TOUCH_KEY
static const u16 touch_key_array[] = GTP_KEY_TAB;
#define GTP_MAX_KEY_NUM	 (sizeof(touch_key_array)/sizeof(touch_key_array[0])) 
#endif

#if 0
static s8 gtp_i2c_test(struct i2c_client *client);
#endif
	
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif
 
#if GTP_CREATE_WR_NODE
extern s32 init_wr_node(struct i2c_client*);
extern void uninit_wr_node(void);
#endif


///////////////////////////////////////////////
//specific tp related macro: need be configured for specific tp
#define CTP_IRQ_NUMBER          (config_info.int_number)
#define CTP_IRQ_MODE		(TRIG_EDGE_NEGATIVE)
#define CTP_NAME		"gt811"	//GOODIX_I2C_NAME
#define SCREEN_MAX_X		(screen_max_x)
#define SCREEN_MAX_Y		(screen_max_y)
#define PRESS_MAX		(255)

static struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
};

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static __u32 twi_id = 0;
static char* cfgname;
static int cfg_index = -1;

static u32 int_handle = 0;

static const unsigned short normal_i2c[2] = {0x5d,I2C_CLIENT_END};

static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("[CTP]:"fmt, ## arg)
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);

struct gt811_cfg_array {
	const char*     name;
	unsigned int    size;
	uint8_t         *config_info;
} gt811_cfg_grp[] = {
	{"gt811_group1",        ARRAY_SIZE(gt811_group1),        gt811_group1},
	{"gt811_group2",        ARRAY_SIZE(gt811_group2),        gt811_group2},
};

static int gtp_find_cfg_idx(const char* name)
{
	int i = 0;
	
	for (i=0; i<ARRAY_SIZE(gt811_cfg_grp); i++) {
		if (!strcmp(name, gt811_cfg_grp[i].name))
			return i;
	}
	return 0;
}


/*******************************************************	
Function:
	Read data from the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:read data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer
*********************************************************/
s32 gtp_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
        struct i2c_msg msgs[2];
        s32 ret = -1;
        s32 retries = 0;      
        
        msgs[0].flags = !I2C_M_RD;
        msgs[0].addr = client->addr;
        msgs[0].len = GTP_ADDR_LENGTH;
        msgs[0].buf = &buf[0];
        
        msgs[1].flags = I2C_M_RD;
        msgs[1].addr = client->addr;
        msgs[1].len = len-GTP_ADDR_LENGTH;
        msgs[1].buf = &buf[GTP_ADDR_LENGTH];
        
        while(retries < 5)
        {
                ret = i2c_transfer(client->adapter,msgs, 2);
                if (!(ret <= 0))break;
                retries++;
        }
        return ret;
}

/*******************************************************	
Function:
	write data to the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:write data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer.
*********************************************************/
s32 gtp_i2c_write(struct i2c_client *client, u8 *data,s32 len)
{
        struct i2c_msg msg;
        s32 ret = -1;
        s32 retries = 0;
  
        msg.flags = !I2C_M_RD;
        msg.addr = client->addr;
        msg.len = len;
        msg.buf = data;		
	
        while(retries < 5)
        {
                ret = i2c_transfer(client->adapter,&msg, 1);
                if (ret == 1)break;
                retries++;
        }
        return ret;
}

/*******************************************************
Function:
	Enable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_disable(struct goodix_ts_data *ts)
{	
        unsigned long irqflags;
        
	dprintk(DEBUG_INT_INFO, "%s start!\n", __func__);
	
        spin_lock_irqsave(&ts->irq_lock, irqflags);
        if (!ts->irq_is_disable) 
        {   		
                sw_gpio_eint_set_enable(CTP_IRQ_NUMBER, 0);		
                ts->irq_is_disable = 1;	
        }	
        spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Disable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_enable(struct goodix_ts_data *ts)
{	
        unsigned long irqflags;
        dprintk(DEBUG_INT_INFO, "%s start!\n", __func__);
			
        spin_lock_irqsave(&ts->irq_lock, irqflags);
        if (ts->irq_is_disable) 
        {		
                sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);		
                ts->irq_is_disable = 0;	
        }	
        spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Send config Function.

Input:
	client:	i2c client.
	
Output:
	Executive outcomes.0¡ª¡ªsuccess,non-0¡ª¡ªfail.
*******************************************************/
s32 gtp_send_cfg(struct i2c_client *client)
{
#if GTP_DRIVER_SEND_CFG
        s32 retry = 0;
        s32 ret = -1;
    
        for (retry = 0; retry < 5; retry++)
        {
                ret = gtp_i2c_write(client, config , GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH);        
                if (ret > 0)
                {
                        return 0;
                }
        }

        return retry;
#else
        return 0;
#endif
}

/*******************************************************
Function:
	GTP initialize function.

Input:
	ts:	i2c client private struct.
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
s32 gtp_init_panel(struct goodix_ts_data *ts)
{
        s32 ret = -1;
        dprintk(DEBUG_INIT, "*****%s start!\n******\n", __func__);
        dprintk(DEBUG_INIT, " cfg_index :%d\n", cfg_index);
        
	memcpy(config, gt811_cfg_grp[cfg_index].config_info, gt811_cfg_grp[cfg_index].size);

#if GTP_CUSTOM_CFG
        config[57] &= 0xf7;
        if(GTP_INT_TRIGGER&0x01)
        {
                config[57] += 0x08; 
        }
        config[59] = GTP_REFRESH;
        config[60] = GTP_MAX_TOUCH>5 ? 5 : GTP_MAX_TOUCH;
        config[61] = (u8)SCREEN_MAX_WIDTH;
        config[62] = (u8)(SCREEN_MAX_WIDTH >> 8);
        config[63] = (u8)SCREEN_MAX_X;
        config[64] = (u8)(SCREEN_MAX_Y >> 8);
#endif
        ts->abs_x_max = (config[62]<<8) + config[61];
        ts->abs_y_max = (config[64]<<8) + config[63];
        ts->max_touch_num = config[60];
        ts->int_trigger_type = ((config[57]>>3)&0x01);
        if ((!ts->abs_x_max)||(!ts->abs_y_max)||(!ts->max_touch_num))
        {
                GTP_ERROR("GTP resolution & max_touch_num invalid, use default value!");
                ts->abs_x_max = SCREEN_MAX_X;
                ts->abs_y_max = SCREEN_MAX_Y;
                ts->max_touch_num = GTP_MAX_TOUCH;
        }
    
        ret = gtp_send_cfg(ts->client);
        if (ret)
        {
                printk("Send config error.");
        }

        msleep(10);
        return 0;
}

/*******************************************************
Function:
	Read goodix touchscreen version function.

Input:
	client:	i2c client struct.
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
s32 gtp_read_version(struct goodix_ts_data *ts)
{
        s32 ret = -1;
        s32 count = 0;
        u8 version_data[6] = {(u8)(GTP_REG_VERSION>>8), (u8)GTP_REG_VERSION, 0, 0, 0, 0};
        u8 version_comfirm[6] = {(u8)(GTP_REG_VERSION>>8), (u8)GTP_REG_VERSION, 0, 0, 0, 0};
	

        ret = gtp_i2c_read(ts->client,version_data, 6);
        if (ret <= 0)
        {
                GTP_ERROR("GTP read version failed"); 
                return ret;
        }
        dprintk(DEBUG_INIT, "Read version:%02x%02x", version_data[4], version_data[5]);
    
        while(count++ < 10)
        {
                gtp_i2c_read(ts->client, version_comfirm, 6);
                if((version_data[4] !=version_comfirm[4])||(version_data[5] != version_comfirm[5]))
                {
                        dprintk(DEBUG_INIT, "Comfirm version:%02x%02x", version_comfirm[4], version_comfirm[5]);
                        version_data[4] = version_comfirm[4];
                        version_data[5] = version_comfirm[5];
                        break;
                }
                msleep(5);
        }
        if(count == 11)
        {
                dprintk(DEBUG_INIT, "GTP chip version:%02x%02x_%02x%02x", version_data[3], version_data[2], version_data[4], version_data[5]);
                ts->version = (version_data[4]<<8)+version_data[5];
                ret = 0;
        }
        else
        {
                printk("GTP read version confirm error!");
                ret = 1;
        }
        return ret;
}

/*******************************************************
Function:
	Touch down report function.

Input:
	ts:private data.
	id:tracking id.
	x:input x.
	y:input y.
	w:input weight.
	
Output:
	None.
*******************************************************/
static void gtp_touch_down(struct goodix_ts_data* ts, s32 id, s32 x, s32 y, s32 w)
{
        dprintk(DEBUG_X_Y_INFO, "source data:ID:%d, X:%d, Y:%d, W:%d\n", id, x, y, w);
        
        if(1 == exchange_x_y_flag){
                swap(x, y);
        }
        
        if(1 == revert_x_flag){
                x = ts->abs_x_max - x;
        }
        
        if(1 == revert_y_flag){
                y = ts->abs_y_max - y;
        }
        
        dprintk(DEBUG_X_Y_INFO,"report data:ID:%d, X:%d, Y:%d, W:%d\n", id, x, y, w);


        input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
        input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);			
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
        input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
        input_mt_sync(ts->input_dev);

}

/*******************************************************
Function:
	Touch up report function.

Input:
	ts:private data.
	
Output:
	None.
*******************************************************/
static void gtp_touch_up(struct goodix_ts_data* ts)
{
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
        input_mt_sync(ts->input_dev);
}

/*******************************************************
Function:
	Goodix touchscreen work function.

Input:
	work:	work_struct of goodix_wq.
	
Output:
	None.
*******************************************************/
int touch_down_flag = 0;
static void goodix_ts_work_func(struct work_struct *work)
{
        u8  point_data[GTP_READ_BYTES] = {(u8)(GTP_REG_COOR>>8),(u8)GTP_REG_COOR,0}; 
        u8  check_sum = 0;
        u8  read_position = 0;
        u8  track_id[GTP_MAX_TOUCH];
        u8  point_index = 0;
        u8  point_tmp = 0;
        u8  touch_num = 0;
        u8  input_w = 0;
        u16 input_x = 0;
        u16 input_y = 0;
        u32 count = 0;
        u32 position = 0;	
        s32 ret = -1;

        struct goodix_ts_data *ts;
    
        GTP_DEBUG_FUNC();

        ts = container_of(work, struct goodix_ts_data, work);
        ret = gtp_i2c_read(ts->client, point_data, sizeof(point_data)/sizeof(point_data[0]));
        if (ret <= 0) 
        {
                goto exit_work_func;  
        }
        GTP_DEBUG_ARRAY(point_data, sizeof(point_data)/sizeof(point_data[0]));
        if(point_data[GTP_ADDR_LENGTH]&0x20)
        {
                if(point_data[3]==0xF0)
                {
                        GTP_DEBUG("Reload config!");
                        ret = gtp_send_cfg(ts->client);
                        if (ret)
                       {
                                GTP_ERROR("Send config error.");
                        }
                        goto exit_work_func;
                }
        }
    
        point_index = point_data[2]&0x1f;
        point_tmp = point_index;
        for(position=0; (position<GTP_MAX_TOUCH)&&point_tmp; position++)
        {
                if(point_tmp&0x01)
                {
                        track_id[touch_num++] = position;
                }
                point_tmp >>= 1;
        }
        GTP_DEBUG("Touch num:%d", touch_num);
        if(touch_num)
        {
                switch(point_data[2]& 0x1f)
                {
                case 0:
                        read_position = 3;
                        break;
                case 1:
                        for (count=2; count<9; count++)
                        {       
                                check_sum += (s32)point_data[count];
                        }
                        read_position = 9;
                        break;
                case 2:
                case 3:
                        for (count=2; count<14;count++)
                        {
                                check_sum += (s32)point_data[count];
                        }
                        read_position = 14;
                        break;
                        //touch number more than 3 fingers
                default:
                        for (count=2; count<35;count++)
                        {
                                check_sum += (s32)point_data[count];
                        }
                        read_position = 35;
                }
                
                if (check_sum != point_data[read_position])
                {
                        GTP_DEBUG("Cal_chksum:%d,  Read_chksum:%d", check_sum, point_data[read_position]);
                        GTP_ERROR("Coordinate checksum error!");
                        goto exit_work_func;
                }

                for(count=0; count<touch_num; count++)
                {
                        if(track_id[count]!=3)
                        {
                                 if(track_id[count]<3)
                                {
                                        position = 4+track_id[count]*5;
                                }
                                else
                                {
                                        position = 30;
                                }
                                input_x = (u16)(point_data[position]<<8)+(u16)point_data[position+1];
                                input_y = (u16)(point_data[position+2]<<8)+(u16)point_data[position+3];
                                input_w = point_data[position+4];
                        }
                        else
                        {
                                input_x = (u16)(point_data[19]<<8)+(u16)point_data[26];
                                input_y = (u16)(point_data[27]<<8)+(u16)point_data[28];
                                input_w = point_data[29];	
                        }

                        if ((input_x > ts->abs_x_max)||(input_y > ts->abs_y_max))
                        {
                                continue;
                        }  
                        gtp_touch_down(ts, track_id[count], input_x, input_y, input_w);       	
    		
                }
        }
        else
        {
        	dprintk(DEBUG_X_Y_INFO, "Touch Release!");
        	gtp_touch_up(ts);
        }
	
	input_sync(ts->input_dev);

#if GTP_HAVE_TOUCH_KEY
        key_value = point_data[3]&0x0F;
        for (count = 0; count < GTP_MAX_KEY_NUM; count++)
        {
                input_report_key(ts->input_dev, touch_key_array[count], !!(key_value&(0x01<<count)));	
        }

        input_report_key(ts->input_dev, BTN_TOUCH, (touch_num || key_value));
        input_sync(ts->input_dev);

#endif

exit_work_func:
	return;
}
#if 0 
/*******************************************************
Function:
	Timer interrupt service routine.

Input:
	timer:	timer struct pointer.
	
Output:
	Timer work mode. HRTIMER_NORESTART---not restart mode
*******************************************************/
static enum hrtimer_restart goodix_ts_timer_handler(struct hrtimer *timer)
{
        struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);
	
        GTP_DEBUG_FUNC();
	
        queue_work(goodix_wq, &ts->work);
        hrtimer_start(&ts->timer, ktime_set(0, (GTP_POLL_TIME+6)*1000000), HRTIMER_MODE_REL);
        return HRTIMER_NORESTART;
}
#endif

/*******************************************************
Function:
	External interrupt service routine.

Input:
	irq:	interrupt number.
	dev_id: private data pointer.
	
Output:
	irq execute status.
*******************************************************/

static u32 goodix_ts_irq_hanbler(struct goodix_ts_data *ts)
{
        //dprintk(DEBUG_INT_INFO,"==========------TS Interrupt-----============\n");
        queue_work(goodix_wq, &ts->work);
        return 0;
}


/*******************************************************
Function:
	Eter sleep function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0¡ª¡ªsuccess,non-0¡ª¡ªfail.
*******************************************************/
static s8 gtp_enter_sleep(struct goodix_ts_data * ts)
{
        s8 ret = -1;
        s8 retry = 0;
        u8 i2c_control_buf[3] = {(u8)(GTP_REG_SLEEP>>8), (u8)GTP_REG_SLEEP, 1};
	
        GTP_DEBUG_FUNC();
    
        while(retry++<5)
        {
                ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
                if (ret == 1)
                {
                        dprintk(DEBUG_SUSPEND, "GTP enter sleep!");
                        return 0;
                }
                msleep(10);
        }
        printk("GTP send sleep cmd failed.");
        return retry;
}

/*******************************************************
Function:
	Wakeup from sleep mode Function.

Input:
	ts:	private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_wakeup_sleep(struct goodix_ts_data * ts)
{
        u8 retry = 0;
        s8 ret = -1;
        
        ctp_wakeup(config_info.wakeup_number, 0, 20);

        while(retry++ < 5)
        {
                ret = gtp_send_cfg(ts->client);
                if (ret > 0)
                {
                        dprintk(DEBUG_SUSPEND, "Wakeup sleep send config success.");
                return ret;
                }
        }

        printk("GTP wakeup sleep failed.");
        return ret;


}
#if 0
/*******************************************************
Function:
	I2c test Function.

Input:
	client:i2c client.
	
Output:
	Executive outcomes.0¡ª¡ªsuccess,non-0¡ª¡ªfail.
*******************************************************/
static s8 gtp_i2c_test(struct i2c_client *client)
{
        u8 retry = 0;
        u8 test_data = 1;
        s8 ret = -1;
  
        GTP_DEBUG_FUNC();
  
        while(retry++<5)
        {
                ret =gtp_i2c_write(client, &test_data, 1);
                if (ret > 0)
                {
                        return 0;
                }
                printk("GTP i2c test failed time %d.",retry);
                msleep(10);
        }
        return retry;
}
#endif

/*******************************************************
Function:
	Request input device Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0¡ª¡ªsuccess,non-0¡ª¡ªfail.
*******************************************************/
static s8 gtp_request_input_dev(struct goodix_ts_data *ts)
{
        s8 ret = -1;
        s8 phys[32];
#if GTP_HAVE_TOUCH_KEY
        u8 index = 0;
#endif
        u16 input_dev_x_max = ts->abs_x_max;
        u16 input_dev_y_max = ts->abs_y_max;
  
  
        ts->input_dev = input_allocate_device();
        if (ts->input_dev == NULL)
        {
                printk("Failed to allocate input device.");
                return -ENOMEM;
        }

        ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
        ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
        ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);

#if GTP_HAVE_TOUCH_KEY
        for (index = 0; index < GTP_MAX_KEY_NUM; index++)
        {
                input_set_capability(ts->input_dev,EV_KEY,touch_key_array[index]);	
        }
#endif

#if GTP_CHANGE_X2Y
        GTP_SWAP(input_dev_x_max, input_dev_y_max);
#endif
        input_set_abs_params(ts->input_dev, ABS_X, 0, input_dev_x_max, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_Y, 0, input_dev_y_max, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
        input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);	
        input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, ts->max_touch_num, 0, 0);

	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

        sprintf(phys, "input/ts");
        ts->input_dev->name = goodix_ts_name;
        ts->input_dev->phys = phys;
        ts->input_dev->id.bustype = BUS_I2C;
        ts->input_dev->id.vendor = 0xDEAD;
        ts->input_dev->id.product = 0xBEEF;
        ts->input_dev->id.version = 10427;
	
        ret = input_register_device(ts->input_dev);
        if (ret)
        {
                printk("Register %s input device failed", ts->input_dev->name);
                return -ENODEV;
        }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
        ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
        ts->early_suspend.suspend = goodix_ts_early_suspend;
        ts->early_suspend.resume = goodix_ts_late_resume;
        register_early_suspend(&ts->early_suspend);
#endif
        return 0;
}

/*******************************************************
Function:
	Goodix touchscreen probe function.

Input:
	client:	i2c device struct.
	id:device id.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        s32 ret = -1;
        struct goodix_ts_data *ts;

        GTP_INFO("GTP Driver Version:%s",GTP_DRIVER_VERSION);
        GTP_INFO("GTP Driver build@%s,%s", __TIME__,__DATE__);

        i2c_connect_client = client;
        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
        {
                printk("I2C check functionality failed.");
                return -ENODEV;
        }
        
        ts = kzalloc(sizeof(*ts), GFP_KERNEL);
        if (ts == NULL)
        {
                printk("Alloc GFP_KERNEL memory failed.");
                return -ENOMEM;
        }
    
        memset(ts, 0, sizeof(*ts));
        INIT_WORK(&ts->work, goodix_ts_work_func);
        ts->client = client;
        i2c_set_clientdata(client, ts);
        	
#if GTP_AUTO_UPDATE
        ret = gtp_read_version(ts);
        if (ret)
        {
                printk("Read version failed.");
		goto exit_device_detect;
        }else {
                ret = gup_downloader( ts, goodix_gt811_firmware);
                if(ret < 0)
                {
                        printk("Warnning:update might be ERROR!\n");
                }
        }
#endif
	
        ret = gtp_init_panel(ts);
        if (ret)
        { 
                printk("GTP init panel failed.");
		goto exit_device_detect;
        }

        ret = gtp_request_input_dev(ts);
        if (ret)
        {
                printk("GTP request input dev failed");
        }
    
        ret = gtp_read_version(ts);
        if (ret)
        {   
                printk("Read version failed.");
        }

        int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)goodix_ts_irq_hanbler,ts);
       	if (!int_handle) {
		printk( "goodix_probe: request irq failed\n");
		goto exit_device_detect;
	}

        ctp_set_int_port_rate(config_info.int_number, 1);
	ctp_set_int_port_deb(config_info.int_number, 0x07);
    
#if GTP_CREATE_WR_NODE
    init_wr_node(client);
#endif
    
        dprintk(DEBUG_INIT, "probe OK !\n");
        return 0;
exit_device_detect:
        sw_gpio_irq_free(int_handle);
	i2c_set_clientdata(client, NULL);
	kfree(ts);
	return ret;
}


/*******************************************************
Function:
	Goodix touchscreen driver release function.

Input:
	client:	i2c device struct.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
	
        dprintk(DEBUG_INIT,"%s start!\n", __func__);
		
	
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif

#if GTP_CREATE_WR_NODE
    uninit_wr_node();
#endif

        sw_gpio_irq_free(int_handle);
	flush_workqueue(goodix_wq);
        destroy_workqueue(goodix_wq);
  	i2c_set_clientdata(ts->client, NULL);
	input_unregister_device(ts->input_dev);
	input_free_device(ts->input_dev);
	kfree(ts);
	
        return 0;
}


/*******************************************************
Function:
	Early suspend function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
        struct goodix_ts_data *ts;
        s8 ret = -1;	
        ts = container_of(h, struct goodix_ts_data, early_suspend);
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0); 
        flush_workqueue(goodix_wq);
        ret = gtp_enter_sleep(ts);
        if (ret)
        {
                printk("GTP early suspend failed.");
        }
}

static void goodix_ts_late_resume(struct early_suspend *h)
{
        struct goodix_ts_data *ts;
        int ret = -1;
        
        ts = container_of(h, struct goodix_ts_data, early_suspend);
	
	ret = gtp_wakeup_sleep(ts);
	if (ret < 0)
	        printk("later resume power on failed\n");
	        
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1); 
       
}
#else
#ifdef CONFIG_PM
static void goodix_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
        struct goodix_ts_data *ts;
        s8 ret = -1;	
        
        ts = i2c_get_clientdata(client);
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0); 
        flush_workqueue(goodix_wq);
        
        ret = gtp_enter_sleep(ts);
        if (ret)
        {
                printk("suspend failed.");
        }
}

/*******************************************************
Function:
	Late resume function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
static void goodix_ts_resume(struct i2c_client *client)
{
        struct goodix_ts_data *ts;
        int ret = -1;
        
        ts = i2c_get_clientdata(client);;
	
	ret = gtp_wakeup_sleep(ts);
	if (ret < 0)
	        printk("resume power on failed\n");
	        
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1); 
       
}
#endif
#endif

static const struct i2c_device_id goodix_ts_id[] = {
    { GTP_I2C_NAME, 0 },
    { }
};

static struct i2c_driver goodix_ts_driver = {
    .class = I2C_CLASS_HWMON,
    .probe      = goodix_ts_probe,
    .remove     = goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_PM
    .suspend    = goodix_ts_suspend,
    .resume     = goodix_ts_resume,
#endif
#endif
    .id_table   = goodix_ts_id,
    .driver = {
        .name     = GTP_I2C_NAME,
        .owner    = THIS_MODULE,
    },
   .address_list	= normal_i2c,
};

static int ctp_get_system_config(void)
{   
        char *name[] = {"ctp_para", "ctp_name"};
        
        get_str_para(name, &cfgname, 1);
        dprintk(DEBUG_INIT,"%s:cfgname:%s\n",__func__,cfgname);
        cfg_index = gtp_find_cfg_idx(cfgname);
        if (cfg_index == -1) {
        	printk("gt82x: no matched TP cfgware(%s)!\n", cfgname);
        	return 0;
        }
        
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
/**
 * ctp_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

        uint8_t buf[5] = {0x07,0x15,0};
        int ret = 0;
        
        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;
    
	if(twi_id == adapter->nr){
	        msleep(50);
	        
                printk("%s: addr= %x\n",__func__,client->addr);
	        ret = gtp_i2c_read(client,buf,3);
                printk("chip_id_value:0x%x\n",buf[2]);
 
                if((buf[2] == 0x11) || (buf[2] == 0x11)){
                        dprintk(DEBUG_INIT, "I2C connection sucess!\n");
            	        strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
    		        return 0;
                } else {                  
        	        printk("%s:I2C connection might be something wrong ! \n",__func__);
        	        return -ENODEV;
                }
	}else{
		return -ENODEV;
	}
}
/*******************************************************	
Function:
	Driver Install function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static int __devinit goodix_ts_init(void)
{

        int ret = -1;

	dprintk(DEBUG_INIT, "===========================%s=====================\n", __func__);
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

	
	goodix_wq = create_singlethread_workqueue("goodix_wq");
	if (!goodix_wq) {
	//printk(KERN_ALERT "Creat %s workqueue failed.\n", f3x_ts_name);
	        return -ENOMEM;
	
	}
	
	ctp_wakeup(config_info.wakeup_number, 0, 5);
	goodix_ts_driver.detect = ctp_detect;

	ret = i2c_add_driver(&goodix_ts_driver);
	
        dprintk(DEBUG_INIT, "===========================%s=====================\n", __func__);
	return ret;
}

/*******************************************************	
Function:
	Driver uninstall function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static void __exit goodix_ts_exit(void)
{
        printk("gt811 driver exited.");
        i2c_del_driver(&goodix_ts_driver);
        input_free_platform_resource(&(config_info.input_type));
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("GTP Series Driver");
MODULE_LICENSE("GPL");
