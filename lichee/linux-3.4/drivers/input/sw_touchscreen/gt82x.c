/*---------------------------------------------------------------------------------------------------------
 * driver/input/touchscreen/goodix_touch.c
 *
 * Copyright(c) 2010 Goodix Technology Corp.     
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
 * Change Date: 
 *		2010.11.11, add point_queue's definiens.     
 *                             
 * 		2011.03.09, rewrite point_queue's definiens.  
 *   
 * 		2011.05.12, delete point_queue for Android 2.2/Android 2.3 and so on.
 *                                                                                              
 *---------------------------------------------------------------------------------------------------------*/
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/gpio.h> 
#include <linux/pm.h>
#include <linux/init-input.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif

#include "gt82x.h"

#include "goodix_touch.h"

#define FOR_TSLIB_TEST
#define TEST_I2C_TRANSFER

struct goodix_ts_data {
	int retry;
	int panel_type;
	uint8_t bad_data;
	char phys[32];		
	struct i2c_client *client;
	struct input_dev *input_dev;
	uint8_t use_irq;
	uint8_t use_shutdown;
	uint32_t gpio_shutdown;
	uint32_t gpio_irq;
	uint32_t screen_width;
	uint32_t screen_height;
	struct ts_event		event;
	struct hrtimer timer;
	struct work_struct  work;
	int (*power)(struct goodix_ts_data * ts, int on);
#ifdef CONFIG_HAS_EARLYSUSPEND	
        struct early_suspend early_suspend;
#endif
};

struct gt82x_cfg_array {
	const char*     name;
	unsigned int    size;
	uint8_t         *config_info;
} gt82x_cfg_grp[] = {
	{"gt813_evb",   ARRAY_SIZE(gt813_evb),  gt813_evb},
	{"gt828",       ARRAY_SIZE(gt828),      gt828},
};



const char *f3x_ts_name = "gt82x";
/**************************************************************************************************/
#define CTP_IRQ_NUMBER          (config_info.int_number)
#define CTP_IRQ_MODE		(TRIG_EDGE_NEGATIVE)
#define SCREEN_MAX_X		(screen_max_x)
#define SCREEN_MAX_Y		(screen_max_y)
#define CTP_NAME		"gt82x"
#define PRESS_MAX		(255)


#define READ_TOUCH_ADDR_H   0x0F
#define READ_TOUCH_ADDR_L   0x40

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;
static __u32 twi_id = 0;
static char* cfgname;
static int cfg_index = -1;

static bool is_suspend = false;

static struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
};

static u32 debug_mask = 0x00;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("[CTP]:"fmt, ## arg)
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);

/**************************************************************************************************/
/*------------------------------------------------------------------------------------------*/ 
/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x5d,I2C_CLIENT_END};
static const int chip_id_value[3] = {0x27,0x28,0x13};
static uint8_t read_chip_value[3] = {0x0f,0x7d,0};

static void goodix_init_events(struct work_struct *work);
static void goodix_resume_events(struct work_struct *work);
static struct workqueue_struct *goodix_wq;
static struct workqueue_struct *goodix_init_wq;
static struct workqueue_struct *goodix_resume_wq;
static DECLARE_WORK(goodix_init_work, goodix_init_events);
static DECLARE_WORK(goodix_resume_work, goodix_resume_events);
/*------------------------------------------------------------------------------------------*/ 
/*used by GT80X-IAP module */
struct i2c_client * i2c_connect_client = NULL;

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
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msgs[2];
	int ret = -1;
	//发送写地址
	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 2;		//data address
	msgs[0].buf   = buf;
	//接收数据
	msgs[1].flags = I2C_M_RD;//读消息
	msgs[1].addr  = client->addr;
	msgs[1].len   = len-2;
	msgs[1].buf   = buf+2;
	
	ret = i2c_transfer(client->adapter, msgs, 2);
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
static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret=-1;
	
	msg.flags = !I2C_M_RD;//写消息
	msg.addr  = client->addr;
	msg.len   = len;
	msg.buf   = data;		
	
	ret = i2c_transfer(client->adapter, &msg,1);
	return ret;
}


/*******************************************************
Function:
	write i2c end cmd.	
	ts:	client Private data structures
return：
    Successful returns 1
*******************************************************/
static s32 i2c_end_cmd(struct goodix_ts_data *ts)
{
        s32 ret;
        u8 end_cmd_data[2] = {0x80, 0x00};    
        
        ret = i2c_write_bytes(ts->client,end_cmd_data,2);
        return ret;
}

/*******************************************************
Foundation
	 i2c communication test
	 ts:i2c client structure
return：
	Successful :1 fail:0
*******************************************************/
//Test i2c to check device. Before it SHUTDOWN port Must be low state 30ms or more.
static bool goodix_i2c_test(struct i2c_client * client)
{
	int ret, retry;
	uint8_t test_data[1] = { 0 };	//only write a data address.
	
	for(retry=0; retry < 2; retry++) {
		ret =i2c_write_bytes(client, test_data, 1);	//Test i2c.
		if (ret == 1)
			break;
		msleep(5);
	}
	
	return ret==1 ? true : false;
}

static int goodix_find_cfg_idx(const char* name)
{
	int i = 0;
	
	for (i=0; i<ARRAY_SIZE(gt82x_cfg_grp); i++) {
		if (!strcmp(name, gt82x_cfg_grp[i].name))
			return i;
	}
	return 0;
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
        int  i = 0;
      
        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)){
        	printk("======return=====\n");
                return -ENODEV;
        }
        if(twi_id == adapter->nr){
	        i2c_read_bytes(client, read_chip_value, 3);
                dprintk(DEBUG_INIT,"addr:0x%x,chip_id_value:0x%x\n", client->addr, read_chip_value[2]);
                while((chip_id_value[i++]) && (i <= sizeof(chip_id_value)/sizeof(chip_id_value[0]))){
                        if(read_chip_value[2] == chip_id_value[i - 1]){
            	                strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
    		                return 0;
                        }                   
		}
		printk("%s:I2C connection might be something wrong ! \n", __func__);
		return -ENODEV;
	}else{
	        return -ENODEV;
	}
}

/*******************************************************
Function:
	GTP initialize function.
Input:
	ts:	i2c client private struct.	
Output:
	Executive outcomes.1---succeed.
*******************************************************/
static int goodix_init_panel(struct goodix_ts_data *ts)
{
	int ret=-1;
	int i = 0;
	uint8_t config_info1[114];
        uint8_t *cfg_data = gt82x_cfg_grp[cfg_index].config_info ;
       
        dprintk(DEBUG_OTHERS_INFO,"init panel\n"); 
        ret=i2c_write_bytes(ts->client,cfg_data, 114);             
	i2c_end_cmd(ts);
        msleep(10);       
	dprintk(DEBUG_OTHERS_INFO,"init panel ret = %d\n",ret);
		
	if (ret < 0)
		return ret;
		
	msleep(100);
	config_info1[0] = 0x0F;
	config_info1[1] = 0x80;
	ret=i2c_read_bytes(ts->client,config_info1,114);
		
	for( i = 0;i<114;i++){
	    dprintk(DEBUG_OTHERS_INFO,"i = %d config_info1[i] = %x \n",i,config_info1[i]);
	    
	}
	
	msleep(10);

	return 1;

} 
static s32 goodix_ts_version(struct goodix_ts_data *ts)
{
        u8 buf[8];
        buf[0] = 0x0f;
        buf[1] = 0x7d;
        
        i2c_read_bytes(ts->client, buf, 5);
        i2c_end_cmd(ts);
        dprintk(DEBUG_INIT,"PID:%02x, VID:%02x%02x\n", buf[2], buf[3], buf[4]);
        return 1;
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
static void goodix_touch_down(struct goodix_ts_data* ts,s32 id,s32 x,s32 y,s32 w)
{
        dprintk(DEBUG_X_Y_INFO,"source data:ID:%d, X:%d, Y:%d, W:%d\n", id, x, y, w);
        if(1 == exchange_x_y_flag){
                swap(x, y);
        }
        
        if(1 == revert_x_flag){
                x = SCREEN_MAX_X - x;
        }
        
        if(1 == revert_y_flag){
                y = SCREEN_MAX_Y - y;
        }
        
        dprintk(DEBUG_X_Y_INFO,"report data:ID:%d, X:%d, Y:%d, W:%d\n", id, x, y, w);
        input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
        input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
        input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
        input_mt_sync(ts->input_dev);
        dprintk(DEBUG_X_Y_INFO,"report data end!\n");
}
/*******************************************************
Function:
	Touch up report function.
Input:
	ts:private data.	
Output:
	None.
*******************************************************/
static void goodix_touch_up(struct goodix_ts_data* ts)
{
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
        input_mt_sync(ts->input_dev);
}

static void goodix_ts_work_func(struct work_struct *work)
{
        u8* coor_data = NULL;
        u8  point_data[2 + 2 + 5 * MAX_FINGER_NUM + 1]={READ_TOUCH_ADDR_H,READ_TOUCH_ADDR_L};
        u8  check_sum = 0;
        u8  touch_num = 0;
        u8  finger = 0;
        u8  key_value = 0;
        s32 input_x = 0;
        s32 input_y = 0;
        s32 input_w = 0;
        s32 idx = 0;
        s32 ret = -1;
        struct goodix_ts_data *ts = NULL;
    
        dprintk(DEBUG_X_Y_INFO,"===enter %s===\n",__func__);
    
        ts = container_of(work, struct goodix_ts_data, work);

        ret = i2c_read_bytes(ts->client, point_data, 10); 
        finger = point_data[2];
        touch_num = (finger & 0x01) + !!(finger & 0x02) + !!(finger & 0x04) + !!(finger & 0x08) + !!(finger & 0x10);
        
        if (touch_num > 1){
                u8 buf[25];
                buf[0] = READ_TOUCH_ADDR_H;
                buf[1] = READ_TOUCH_ADDR_L + 8;
                ret = i2c_read_bytes(ts->client, buf, 5 * (touch_num - 1) + 2); 
                memcpy(&point_data[10], &buf[2], 5 * (touch_num - 1));
        }
        i2c_end_cmd(ts);

        if (ret <= 0){
                printk("%s:I2C read error!",__func__);
                goto exit_work_func;
        }

        if((finger & 0xC0) != 0x80){
                dprintk(DEBUG_INIT,"%s:Data not ready!",__func__);
                goto exit_work_func;
        }

        key_value = point_data[3]&0x0f; // 1, 2, 4, 8
        if ((key_value & 0x0f) == 0x0f){
                if (!goodix_init_panel(ts)){
                        printk("%s:Reload config failed!\n",__func__);
                }
                goto exit_work_func;
        }

        coor_data = &point_data[4];
        check_sum = 0;
        for ( idx = 0; idx < 5 * touch_num; idx++){
                check_sum += coor_data[idx];
        }
        
        if (check_sum != coor_data[5 * touch_num]){
                printk("%s:Check sum error!",__func__);
                goto exit_work_func;
        }

        if (touch_num){
                s32 pos = 0;
        
                for (idx = 0; idx < MAX_FINGER_NUM; idx++){
                        if (!(finger & (0x01 << idx))){
                                continue;
                        }
                        
                        input_x  = coor_data[pos] << 8;
                        input_x |= coor_data[pos + 1];
        
                        input_y  = coor_data[pos + 2] << 8;
                        input_y |= coor_data[pos + 3];
        
                        input_w  = coor_data[pos + 4];
        
                        pos += 5;
        
                        goodix_touch_down(ts, idx, input_x, input_y, input_w);
                }
        }else{
                dprintk(DEBUG_X_Y_INFO,"Touch Release!");
                goodix_touch_up(ts);
        }

#if GTP_HAVE_TOUCH_KEY
        for (idx= 0; idx < GTP_MAX_KEY_NUM; idx++){
                input_report_key(ts->input_dev, touch_key_array[idx], key_value & (0x01<<idx));   
        }
#endif

        input_sync(ts->input_dev);

exit_work_func:
        return;
}

static u32 goodix_ts_irq_hanbler(struct goodix_ts_data *ts)
{
        dprintk(DEBUG_INT_INFO,"==========------TS Interrupt-----============\n");
        queue_work(goodix_wq, &ts->work);
        return 0;
}

static int goodix_ts_power(struct goodix_ts_data * ts, int on)
{
        s32 ret = -1;
        s32 success = 1;
        u8 i2c_control_buf1[3] = {0x0F,0xF2,0xc0};        //suspend cmd
        switch(on)
        {
        case 0:
                ret = i2c_write_bytes(ts->client, i2c_control_buf1, 3);
                i2c_end_cmd(ts);
                return ret;        
        case 1:             
                ctp_wakeup(config_info.wakeup_number, 0, 50);
                
#ifdef CONFIG_HAS_EARLYSUSPEND                
                if(STANDBY_WITH_POWER_OFF == standby_level){
                        ret = goodix_i2c_test(ts->client);
                        if(!ret){
        	                printk("Warnning: I2C connection might be something wrong!\n");
        	                ctp_wakeup(config_info.wakeup_number, 0, 50);
        	                ret = goodix_i2c_test(ts->client);
        	                if(!ret){
        	                        printk("retry fail!\n");
        	                        return -1;
        	                }
                        }
                        pr_info("===== goodix i2c test ok=======\n");
                }
#endif                
                ret = goodix_init_panel(ts);
                if( ret != 1){
                        printk("init panel fail!\n");
                        return -1;
                }
                return success;
        
         default:
                printk("%s: Cant't support this command.",f3x_ts_name );
                return -EINVAL;
        
        } 
	
}

static void goodix_resume_events (struct work_struct *work)
{
	int ret;
        struct goodix_ts_data *ts = i2c_get_clientdata(i2c_connect_client);
        
	if (ts->power) {
		ret = ts->power(ts, 1);
		if (ret < 0)
			dprintk(DEBUG_SUSPEND,"%s power on failed\n", f3x_ts_name);
	}
	sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1); 
}


static int goodix_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
        struct goodix_ts_data *ts = i2c_get_clientdata(client);

        dprintk(DEBUG_SUSPEND,"%s---start. \n",__func__); 
        cancel_work_sync(&goodix_resume_work);
  	flush_workqueue(goodix_resume_wq); 
  	
#ifndef CONFIG_HAS_EARLYSUSPEND
        is_suspend = true;
#endif  
 
        if(is_suspend == true){                                
                sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
                ret = cancel_work_sync(&ts->work);
                flush_workqueue(goodix_wq);
                
                if (ts->power) {
                	ret = ts->power(ts,0);
                	if (ret < 0)
                		dprintk(DEBUG_SUSPEND,"%s power off failed\n", f3x_ts_name);
                }
        }
        
        is_suspend = true;
        return 0 ;
}

//重新唤醒
static int goodix_ts_resume(struct i2c_client *client)
{
        dprintk(DEBUG_SUSPEND,"%s-----start. \n",__func__);
        queue_work(goodix_resume_wq, &goodix_resume_work);
	is_suspend = true; 
	return 0;
}
//停用设备
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
	int ret;
	struct goodix_ts_data *ts = container_of(h, struct goodix_ts_data, early_suspend);
	     
        dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND:%s-----tart. \n",__func__);  
        is_suspend = false;                            
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
        cancel_work_sync(&goodix_resume_work);
  	flush_workqueue(goodix_resume_wq);
        ret = cancel_work_sync(&ts->work);
        flush_workqueue(goodix_wq);
        
        if (ts->power) {
        	ret = ts->power(ts,0);
        	if (ret < 0)
        		dprintk(DEBUG_SUSPEND,"%s power off failed\n", f3x_ts_name);
        }
        
        return ;
}

//重新唤醒
static void goodix_ts_later_resume(struct early_suspend *h)
{
        struct goodix_ts_data *ts = container_of(h, struct goodix_ts_data, early_suspend);
        dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND:%s-----start. \n",__func__); 
        
        if(is_suspend == false){
              goodix_ts_resume(ts->client);
        }
	return ;
}
#endif

static void goodix_init_ts(struct i2c_client *client)
{
        int ret = -1;
        struct goodix_ts_data * ts = i2c_get_clientdata(client);
        dprintk(DEBUG_INIT,"%s----start!\n",__func__);
        
        ctp_wakeup(config_info.wakeup_number, 0, 100);
	ret = goodix_init_panel(ts);
	if(!ret) {
	        printk("init panel fail!\n");
		goto err_init_godix_ts;
	}else {
	        dprintk(DEBUG_INIT,"init panel succeed!\n");	
        }
	goodix_ts_version(ts);
	
	int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)goodix_ts_irq_hanbler,ts);
       	if (!int_handle) {
		pr_info( "goodix_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	ctp_set_int_port_rate(config_info.int_number, 1);
	ctp_set_int_port_deb(config_info.int_number, 0x07);
	dprintk(DEBUG_INIT,"reg clk: 0x%08x\n", readl(0xf1c20a18));
	
	dprintk(DEBUG_INIT,"%s----over!\n",__func__);	
	
	return ;
	
exit_irq_request_failed:
        sw_gpio_irq_free(int_handle);	
err_init_godix_ts:	
        return ;
        
}
static void goodix_init_events (struct work_struct *work)
{
        goodix_init_ts(i2c_connect_client);
	return;
}
/*******************************************************	
功能：
	触摸屏探测函数
	在注册驱动时调用（要求存在对应的client）；
	用于IO,中断等资源申请；设备注册；触摸屏初始化等工作
参数：
	client：待驱动的设备结构体
	id：设备ID
return：
	执行结果码，0表示正常执行
********************************************************/
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct goodix_ts_data *ts;
	dprintk(DEBUG_INIT,"=============GT82x Probe==================\n");
	
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	goodix_init_wq = create_singlethread_workqueue("goodix_init");
	if (goodix_init_wq == NULL) {
		printk("create goodix_init_wq fail!\n");
		return -ENOMEM;
	}
	goodix_resume_wq = create_singlethread_workqueue("goodix_resume");
	if (goodix_resume_wq == NULL) {
		printk("create goodix_resume_wq fail!\n");
		return -ENOMEM;
		
	}
	i2c_connect_client = client;				//used by Guitar Updating.
	ts->client = client;
	ts->power = goodix_ts_power;
	i2c_set_clientdata(client, ts);
	
	INIT_WORK(&ts->work, goodix_ts_work_func);
	goodix_wq = create_singlethread_workqueue("goodix_wq");
	if (!goodix_wq) {
		printk(KERN_ALERT "Creat %s workqueue failed.\n", f3x_ts_name);
		return -ENOMEM;
		
	}

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) 
	{
		ret = -ENOMEM;
		dev_dbg(&ts->client->dev,"Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	
#ifndef GOODIX_MULTI_TOUCH	
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
	input_set_abs_params(ts->input_dev, ABS_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);	
	
#else
	ts->input_dev->absbit[0] = BIT_MASK(ABS_MT_TRACKING_ID) |
	BIT_MASK(ABS_MT_TOUCH_MAJOR)| BIT_MASK(ABS_MT_WIDTH_MAJOR) |
  	BIT_MASK(ABS_MT_POSITION_X) | BIT_MASK(ABS_MT_POSITION_Y); 	// for android
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);	
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, MAX_FINGER_NUM, 0, 0);	
#endif	

#ifdef FOR_TSLIB_TEST
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
#endif

	sprintf(ts->phys, "input/goodix-ts");
	ts->input_dev->name = CTP_NAME;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0xDEAD;
	ts->input_dev->id.product = 0xBEEF;
	ts->input_dev->id.version = 0x1105;	

	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&ts->client->dev,"Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	queue_work(goodix_init_wq, &goodix_init_work);
		
	
#ifdef CONFIG_HAS_EARLYSUSPEND	
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;	
	ts->early_suspend.suspend = goodix_ts_early_suspend;
	ts->early_suspend.resume  = goodix_ts_later_resume;	
	register_early_suspend(&ts->early_suspend);
#endif
	
	dprintk(DEBUG_INIT,"========Probe Ok================\n");
	return 0;
	
err_input_register_device_failed:	
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:	
	i2c_set_clientdata(ts->client, NULL);
	kfree(ts);	
err_alloc_data_failed:	        
	return ret;
}


/*******************************************************	
功能：
	驱动资源释放
参数：
	client：设备结构体
return：
	执行结果码，0表示正常执行
********************************************************/
static int __devinit goodix_ts_remove(struct i2c_client *client)
{
	struct goodix_ts_data * ts = i2c_get_clientdata(client);
	printk("The driver is removing...\n");
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
        sw_gpio_irq_free(int_handle);
	flush_workqueue(goodix_wq);
	cancel_work_sync(&goodix_init_work);
  	cancel_work_sync(&goodix_resume_work);
	destroy_workqueue(goodix_wq);
  	destroy_workqueue(goodix_init_wq);
  	destroy_workqueue(goodix_resume_wq);
  	i2c_set_clientdata(ts->client, NULL);
	input_unregister_device(ts->input_dev);
	input_free_device(ts->input_dev);
	kfree(ts);
	
	return 0;
}

//可用于该驱动的 设备名—设备ID 列表
//only one client
static const struct i2c_device_id goodix_ts_id[] = {
	{ CTP_NAME, 0 },
	{ }
};

//设备驱动结构体
static struct i2c_driver goodix_ts_driver = {
	.class          = I2C_CLASS_HWMON,
	.probe		= goodix_ts_probe,
	.remove		= __devexit_p(goodix_ts_remove),
	.id_table	= goodix_ts_id,
	.suspend        =  goodix_ts_suspend,
	.resume         =  goodix_ts_resume,
	.driver = {
		.name	= CTP_NAME,
		.owner  = THIS_MODULE,
	},
	.address_list	= normal_i2c,
};

static int ctp_get_system_config(void)
{   
        char *name[] = {"ctp_para", "ctp_name"};
        
        get_str_para(name, &cfgname, 1);
        dprintk(DEBUG_INIT,"%s:cfgname:%s\n",__func__,cfgname);
        cfg_index = goodix_find_cfg_idx(cfgname);
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
//驱动加载函数
static int __devinit goodix_ts_init(void)
{
	int ret = -1;
        dprintk(DEBUG_INIT,"****************************************************************\n");
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
        
	ctp_wakeup(config_info.wakeup_number, 0, 20);
	goodix_ts_driver.detect = ctp_detect;
	ret = i2c_add_driver(&goodix_ts_driver);
	
        dprintk(DEBUG_INIT,"****************************************************************\n");
	return ret;
}

//驱动卸载函数
static void __exit goodix_ts_exit(void)
{
	printk("==goodix_ts_exit==\n");
	i2c_del_driver(&goodix_ts_driver);
	input_free_platform_resource(&(config_info.input_type));	
	return;
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("Goodix Touchscreen Driver");
MODULE_LICENSE("GPL v2");
