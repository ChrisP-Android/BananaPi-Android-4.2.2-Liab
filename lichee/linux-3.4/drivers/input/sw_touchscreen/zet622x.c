/* drivers/input/touchscreen/zet6221_i2c.c
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * ZEITEC Semiconductor Co., Ltd
 * Tel: +886-3-579-0045
 * Fax: +886-3-579-9960
 * http://www.zeitecsemi.com
 */
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/pm.h>
#include <linux/earlysuspend.h>
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
#include <asm/uaccess.h>

#include <mach/gpio.h> 
#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/input/mt.h> //Henry++

#include <linux/init-input.h>
#include <linux/gpio.h>
#include "zet622x_fw.h"

static struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
};

#define CTP_IRQ_NUMBER                  (config_info.int_number)
#define CTP_IRQ_MODE		        (TRIG_EDGE_NEGATIVE)

static const unsigned short normal_i2c[2] = {0x76,I2C_CLIENT_END};
static __u32 twi_id = 0;
static int screen_max_x = 0;
static int screen_max_y = 0;
static u32 int_handle = 0;
/* -------------- global variable definition -----------*/
#define _MACH_MSM_TOUCH_H_

#define ZET_TS_ID_NAME          "zet622x"
#define MJ5_TS_NAME	        ZET_TS_ID_NAME 

#define RSTPIN_ENABLE
#define TPINFO	                1
#define Y_MAX                   (screen_max_y)
#define X_MAX                   (screen_max_x)

#define MT_TYPE_B
#define TP_AA_X_MAX	        480
#define TP_AA_Y_MAX	        600
#define FINGER_NUMBER           5
#define KEY_NUMBER              0	//please assign correct default key number 0/8 
 
#define DEBOUNCE_NUMBER	        1
#define P_MAX	                255
#define D_POLLING_TIME	        25000
#define U_POLLING_TIME	        25000
#define S_POLLING_TIME          100
#define REPORT_POLLING_TIME     5

#define MAX_KEY_NUMBER      	8
#define MAX_FINGER_NUMBER	16
#define TRUE 		        1
#define FALSE 		        0


static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("[CTP]:"fmt, ## arg)
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);

#define TOPRIGHT 	        0
#define TOPLEFT  	        1
#define BOTTOMRIGHT	        2
#define BOTTOMLEFT	        3
#define ORIGIN		        BOTTOMRIGHT


struct msm_ts_platform_data {
	unsigned int x_max;
	unsigned int y_max;
	unsigned int pressure_max;
};

struct zet6221_tsdrv {
	struct i2c_client *i2c_ts;
	struct work_struct work1;
	struct work_struct work2; //  write_cmd
	struct workqueue_struct *ts_workqueue; // 
	struct workqueue_struct *ts_workqueue1; //write_cmd
	struct input_dev *input;
	struct timer_list polling_timer;
	struct early_suspend early_suspend;
	unsigned int gpio; /* GPIO used for interrupt of TS1*/
	unsigned int irq;
	unsigned int x_max;
	unsigned int y_max;
	unsigned int pressure_max;
};

#ifdef ZET_TIMER
static u16 polling_time = S_POLLING_TIME;
#endif

#ifdef ZET_DOWNLOADER
static unsigned char zeitec_zet622x_page[131] __initdata;
static unsigned char zeitec_zet622x_page_in[131] __initdata;
#endif

static u8  ChargeChange = 0;//discharge
static u8  key_menu_pressed = 0x0; //0x1;
static u8  key_back_pressed = 0x0; //0x1;
static u8  key_home_pressed = 0x0; //0x1;
static u8  key_search_pressed = 0x0; //0x1;
static u8  inChargerMode=0;
static u8  xyExchange=0;
static u8  pc[8];
static u8  ic_model = 0;	// 0:6221 / 1:6223
static u16 ResolutionX=0;
static u16 ResolutionY=0;
static u16 FingerNum=0;
static u16 KeyNum=0;

static u16 fb[8] = {0x3DF1, 0x3DF4, 0x3DF7, 0x3DFA, 0x3EF6, 0x3EF9, 0x3EFC, 0x3EFF};
static u16 fb21[8] = {0x3DF1, 0x3DF4, 0x3DF7, 0x3DFA, 0x3EF6, 0x3EF9, 0x3EFC, 0x3EFF}; 
static u16 fb23[8] = {0x7BFC, 0x7BFD, 0x7BFE, 0x7BFF, 0x7C04, 0x7C05, 0x7C06, 0x7C07};

static struct i2c_client *this_client;
static int resetCount = 0;          //albert++ 20120807
static int bufLength=0;	
static int f_up_cnt=0;
static int enable_cmd; 

/**
 * ctp_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;
    
	if(twi_id == adapter->nr){
                dprintk(DEBUG_INIT,"%s: addr = %x\n", __func__, client->addr);
                ret = i2c_test(client);
                if(!ret){
        		printk("%s:I2C connection might be something wrong \n", __func__);
        		return -ENODEV;
        	} else {           	    
            	        strlcpy(info->type, ZET_TS_ID_NAME, I2C_NAME_SIZE);
    		    return 0;	
	        }

	}else{
		return -ENODEV;
	}
}

/***********************************************************************
    [function]: 
		        callback: read data by i2c interface;
    [parameters]:
			    client[in]:  struct i2c_client ??represent an I2C slave device;
			    data [out]:  data buffer to read;
			    length[in]:  data length to read;
    [return]:
			    Returns negative errno, else the number of messages executed;
************************************************************************/
static s32 zet6221_i2c_read_tsdata(struct i2c_client *client, u8 *data, u8 length)
{
	struct i2c_msg msg;
	msg.addr = client->addr;
	msg.flags = I2C_M_RD;
	msg.len = length;
	msg.buf = data;
	return i2c_transfer(client->adapter,&msg, 1);
}

/***********************************************************************
    [function]: 
		        callback: write data by i2c interface;
    [parameters]:
			    client[in]:  struct i2c_client ??represent an I2C slave device;
			    data [out]:  data buffer to write;
			    length[in]:  data length to write;
    [return]:
			    Returns negative errno, else the number of messages executed;
************************************************************************/
static s32 zet6221_i2c_write_tsdata(struct i2c_client *client, u8 *data, u8 length)
{
	struct i2c_msg msg;
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = length;
	msg.buf = data;
	return i2c_transfer(client->adapter,&msg, 1);
}

void ts_write_charge_enable_cmd(void)
{	
        int ret=0;
        u8 ts_write_charge_cmd[1] = {0xb5}; 
        dprintk(DEBUG_INIT, "%s is running ==========",__FUNCTION__);
        ret=zet6221_i2c_write_tsdata(this_client, ts_write_charge_cmd, 1);
}
EXPORT_SYMBOL_GPL(ts_write_charge_enable_cmd);

void ts_write_charge_disable_cmd(void)
{
	u8 ts_write_cmd[1] = {0xb6}; 
	int ret=0;
	dprintk(DEBUG_INIT, "%s is running ==========",__FUNCTION__);
	ret=zet6221_i2c_write_tsdata(this_client, ts_write_cmd, 1);
}
EXPORT_SYMBOL_GPL(ts_write_charge_disable_cmd);

/***********************************************************************
    [function]: 
		        callback: Timer Function if there is no interrupt fuction;
    [parameters]:
			    arg[in]:  arguments;
    [return]:
			    NULL;
************************************************************************/
#ifdef ZET_TIMER

static void polling_timer_func(unsigned long arg)
{

	struct zet6221_tsdrv *ts_drv = (struct zet6221_tsdrv *)arg;
	queue_work(ts_drv->ts_workqueue1, &ts_drv->work2);
	mod_timer(&ts_drv->polling_timer,jiffies + msecs_to_jiffies(polling_time));	
}

#endif

static void write_cmd_work(struct work_struct *_work)
{
	if(enable_cmd != ChargeChange)
	{	
		if(enable_cmd == 1) {
			ts_write_charge_enable_cmd();
			
		}else if(enable_cmd == 0)
		{
			ts_write_charge_disable_cmd();
		}
		ChargeChange = enable_cmd;
	}

}

/***********************************************************************
    [function]: 
		        callback: coordinate traslating;
    [parameters]:
			    px[out]:  value of X axis;
			    py[out]:  value of Y axis;
				p [in]:   pressed of released status of fingers;
    [return]:
			    NULL;
************************************************************************/
void touch_coordinate_traslating(u32 *px, u32 *py, u8 p)
{
	int i;
	u8 pressure;

	#if ORIGIN == TOPRIGHT
	for(i=0;i<MAX_FINGER_NUMBER;i++){
		pressure = (p >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		if(pressure)
		{
			px[i] = X_MAX - px[i];
		}
	}
	#elif ORIGIN == BOTTOMRIGHT
	for(i=0;i<MAX_FINGER_NUMBER;i++){
		pressure = (p >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		if(pressure)
		{
			px[i] = X_MAX - px[i];
			py[i] = Y_MAX - py[i];
		}
	}
	#elif ORIGIN == BOTTOMLEFT
	for(i=0;i<MAX_FINGER_NUMBER;i++){
		pressure = (p >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		if(pressure)
		{
			py[i] = Y_MAX - py[i];
		}
	}
	#endif
}

/***********************************************************************
    [function]: 
		        callback: read finger information from TP;
    [parameters]:
    			client[in]:  struct i2c_client ??represent an I2C slave device;
			    x[out]:  values of X axis;
			    y[out]:  values of Y axis;
			    z[out]:  values of Z axis;
				pr[out]:  pressed of released status of fingers;
				ky[out]:  pressed of released status of keys;
    [return]:
			    Packet ID;
************************************************************************/
static u8 zet6221_ts_get_xy_from_panel(struct i2c_client *client, u32 *x, u32 *y, u32 *z, u32 *pr, u32 *ky)
{
	u8  ts_data[70];
	int ret;
	int i;
	
	memset(ts_data, 0, 70);

	ret=zet6221_i2c_read_tsdata(client, ts_data, bufLength);
	
	*pr = ts_data[1];
	*pr = (*pr << 8) | ts_data[2];
		
	for(i = 0; i < FingerNum; i++) {
		x[i] = (u8)((ts_data[3+4*i])>>4)*256 + (u8)ts_data[(3+4*i)+1];
		y[i] = (u8)((ts_data[3+4*i]) & 0x0f)*256 + (u8)ts_data[(3+4*i)+2];
		z[i] = (u8)((ts_data[(3+4*i)+3]) & 0x0f);
	}
		
	//if key enable
	if(KeyNum > 0)
		*ky = ts_data[3+4*FingerNum];

	return ts_data[0];
}

static u8 zet6221_ts_get_report_mode_t(struct i2c_client *client)
{
	u8 ts_report_cmd[1] = {178};
//	u8 ts_reset_cmd[1] = {176};
	u8 ts_in_data[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int ret;
	int i;
	
	ret=zet6221_i2c_write_tsdata(client, ts_report_cmd, 1);

	if (ret > 0) {
	        msleep(10);
	        dprintk ( DEBUG_INIT, "=============== zet6221_ts_get_report_mode_t ===============\n");
	        ret=zet6221_i2c_read_tsdata(client, ts_in_data, 17);
			
		if(ret > 0) {
		        for(i=0;i<8;i++)
			        pc[i]=ts_in_data[i] & 0xff;
		
        		xyExchange = (ts_in_data[16] & 0x8) >> 3;
        		if(xyExchange == 1) {
        			ResolutionY = ts_in_data[9] & 0xff;
        			ResolutionY = (ResolutionY << 8)|(ts_in_data[8] & 0xff);
        			ResolutionX = ts_in_data[11] & 0xff;
        			ResolutionX = (ResolutionX << 8) | (ts_in_data[10] & 0xff);
        		} else {
        			ResolutionX = ts_in_data[9] & 0xff;
        			ResolutionX = (ResolutionX << 8)|(ts_in_data[8] & 0xff);
        			ResolutionY = ts_in_data[11] & 0xff;
        			ResolutionY = (ResolutionY << 8) | (ts_in_data[10] & 0xff);
        		}
        			
        		FingerNum = (ts_in_data[15] & 0x7f);
        		KeyNum = (ts_in_data[15] & 0x80);
        		inChargerMode = (ts_in_data[16] & 0x2) >> 1;
        
        		if(KeyNum==0)
        			bufLength  = 3 + 4*FingerNum;
        		else
        			bufLength  = 3 + 4*FingerNum + 1;
				
		} else {
		        printk ("=============== zet6221_ts_get_report_mode_t READ ERROR ===============\n");
			return ret;
		}
							
	} else {
		return ret;
	}
	
	return 1;
}


/***********************************************************************
    [function]: 
		        callback: interrupt function;
    [parameters]:
    			irq[in]:  irq value;
    			dev_id[in]: dev_id;

    [return]:
			    NULL;
************************************************************************/
static u32 zet6221_ts_interrupt(struct zet6221_tsdrv *ts_drv)
{
        dprintk(DEBUG_INT_INFO, "==========------zet6221_ts TS Interrupt-----============\n"); 
	queue_work(ts_drv->ts_workqueue, &ts_drv->work1);

	return 0;
}

/***********************************************************************
    [function]: 
		        callback: touch information handler;
    [parameters]:
    			_work[in]:  struct work_struct;

    [return]:
			    NULL;
************************************************************************/
static void zet6221_ts_work(struct work_struct *_work)
{
	struct zet6221_tsdrv *ts = container_of(_work, struct zet6221_tsdrv, work1);
	struct i2c_client *tsclient1 = ts->i2c_ts;
	u32 x[MAX_FINGER_NUMBER], y[MAX_FINGER_NUMBER], z[MAX_FINGER_NUMBER], pr, ky = 0, points;
	u32 px,py,pz;
	u8 ret;
	u8 pressure;
	int i;
	

	if (bufLength == 0)
		return;
	
	
	if(resetCount == 1) {
		resetCount = 0;
		return;
	}

#if 0	
	// be sure to get coordinate data when INT=LOW
	if( gpio_read_one_pin_value(gpio_int_hdle, NULL) != 0)
		return;
	
#endif
	ret = zet6221_ts_get_xy_from_panel(tsclient1, x, y, z, &pr, &ky);
	write_cmd_work(_work);
	
	if(ret == 0x3C) {
		points = pr;
		
		#if defined(TRANSLATE_ENABLE)
		        touch_coordinate_traslating(x, y, points);
		#endif
		
		if(points == 0) {
			f_up_cnt++;
			if(f_up_cnt>=DEBOUNCE_NUMBER) {
				f_up_cnt = 0;
#ifdef MT_TYPE_B
				for(i=0;i<FingerNum;i++){
					input_mt_slot(ts->input, i);
					input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,false);
					input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
				}
				input_mt_report_pointer_emulation(ts->input, true);
#else
				input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 0);
				input_mt_sync(ts->input);
#endif
			}
			goto no_finger;
		}
		
#if defined(VIRTUAL_KEY)
        KeyNum = 4;
	if(points == 0x8000) { // only finger 1 enable. 

		if(y[0] > TP_AA_Y_MAX) {
			if( x[0]>=33 && x[0]<=122 && y[0]>=897 && y[0]<=1019 )
				ky=0x1;
		        else if(x[0]>=184 && x[0]<=273 && y[0]>=897 && y[0]<=1019 ) 
				ky=0x2;
			else if(x[0]>=363 && x[0]<=451 && y[0]>=897 && y[0]<=1019 ) 
				ky=0x4;
			else if(x[0]>=527 && x[0]<=615 && y[0]>=897 && y[0]<=1019 ) 
				ky=0x8;
			goto Key_finger;
		}
	}
#endif

        f_up_cnt = 0;
	for(i=0;i<FingerNum;i++) {
	        pressure = (points >> (MAX_FINGER_NUMBER-i-1)) & 0x1;
		dprintk(DEBUG_X_Y_INFO, "valid=%04X pressure[%d]= %d x= %d y= %d\n",
		        points, i,pressure,x[i], y[i]);
		if(pressure) {
                        px = x[i];
		        py = y[i];
                        pz = z[i];
				
#if defined(VIRTUAL_KEY)
                        if(py > TP_AA_Y_MAX) 
			        py = TP_AA_Y_MAX;
#endif

#ifdef MT_TYPE_B
                        input_mt_slot(ts->input, i);
			input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,true);
#endif
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, i);
	    		input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pz);
	    		input_report_abs(ts->input, ABS_MT_POSITION_X, px);
	    		input_report_abs(ts->input, ABS_MT_POSITION_Y, py);
#ifndef MT_TYPE_B	    		
	    		input_mt_sync(ts->input);
#endif

		} else {
#ifdef MT_TYPE_B
		        input_mt_slot(ts->input, i);
			input_mt_report_slot_state(ts->input, MT_TOOL_FINGER,false);
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
#endif				
		}
	}
#ifdef MT_TYPE_B
		input_mt_report_pointer_emulation(ts->input, true);
#endif

//Key_finger:
no_finger:
                if(KeyNum > 0) {
                        for(i=0;i<MAX_KEY_NUMBER;i++) {			
        		        pressure = ky & ( 0x01 << i );
                                switch(i) {
        			case 0:
        			        if(pressure) {
        				        if(!key_search_pressed) {					
                                                        input_report_key(ts->input, KEY_SEARCH, 1);
                                                        key_search_pressed = 0x1;
        					}
        				} else {
        				        if(key_search_pressed) {
                                                        input_report_key(ts->input, KEY_SEARCH, 0);
                                                        key_search_pressed = 0x0;
        					}
        				}
        						
        				break;
        			case 1:
        			        if(pressure) {
        				        if(!key_back_pressed) {							
                                                        input_report_key(ts->input, KEY_BACK, 1);
                                                        key_back_pressed = 0x1;
        					}
        				} else {
                                                if(key_back_pressed) {
                                                        input_report_key(ts->input, KEY_BACK, 0);
                                                        key_back_pressed = 0x0;
        					}
        				}
                                        break;
        			case 2:
        			        if(pressure) {
        				        if(!key_home_pressed) {
                                                        input_report_key(ts->input, KEY_HOME, 1);
                                                        key_home_pressed = 0x1;
        					}
        				} else {
        				        if(key_home_pressed) {
                                                        input_report_key(ts->input, KEY_HOME, 0);
                                                         key_home_pressed = 0x0;
        					}
        				}
        				break;
        			case 3:
        			        if(pressure) {
        				        if(!key_menu_pressed) {
                                                        input_report_key(ts->input, KEY_MENU, 1);
                                                        key_menu_pressed = 0x1;
        					}
        				} else {
        				        if(key_menu_pressed) {
                                                        input_report_key(ts->input, KEY_MENU, 0);
                                                        key_menu_pressed = 0x0;
        					}
        				}
        				break;
        			case 4:
        				break;
        			case 5:
        				break;
        			case 6:
        				break;
        			case 7:
        				break;
        			}
        
        		}
        	}

		input_sync(ts->input);		
	}

}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ts_early_suspend(struct early_suspend *h)
{
	//Sleep Mode
#if 1
	//struct zet6221_tsdrv *zet6221_ts;
	u8 ts_sleep_cmd[1] = {0xb1}; 
	int ret=0;
	ret=zet6221_i2c_write_tsdata(this_client, ts_sleep_cmd, 1);
#endif
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);

	return;	        
}

static void ts_late_resume(struct early_suspend *h)
{
#if 1
	resetCount = 1;
	ctp_wakeup(config_info.wakeup_number, 0, 10);
	msleep(100);
	
#else
	//struct zet6221_tsdrv *zet6221_ts;
	ctp_wakeup(config_info.wakeup_number, 0, 10);
	msleep(100);
	u8 ts_wakeup_cmd[1] = {0xb4};
	zet6221_i2c_write_tsdata(this_client, ts_wakeup_cmd, 1);
	
#endif
        sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
	ChargeChange = 0;

	return;
}
#endif

/*************************6221_FW*******************************/

u8 zet622x_ts_sndpwd(struct i2c_client *client)
{
	u8 ts_sndpwd_cmd[3] = {0x20,0xC5,0x9D};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_sndpwd_cmd, 3);

	return ret;
}

u8 zet622x_ts_option(struct i2c_client *client)
{

	u8 ts_cmd[1] = {0x27};
//	u8 ts_cmd_erase[1] = {0x28};
	u8 ts_in_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//	u8 ts_out_data[18] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int ret;
	u16 model;
	int i;
	
	printk("\noption write : "); 

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	msleep(1);
	
	printk("%02x ",ts_cmd[0]); 
	
	printk("\nread : "); 

	ret=zet6221_i2c_read_tsdata(client, ts_in_data, 16);

	msleep(1);

	for(i=0;i<16;i++)
	{
		printk("%02x ",ts_in_data[i]); 
	}
	printk("\n"); 

	model = 0x0;
	model = ts_in_data[7];
	model = (model << 8) | ts_in_data[6];
	
	switch(model) { 
        case 0xFFFF: 
        	ret = 1;
            ic_model = 0;
			for(i=0;i<8;i++)
			{
				fb[i]=fb21[i];
			}
			
#if defined(High_Impendence_Mode)

			if(ts_in_data[2] != 0xf1)
			{
			
				if(zet622x_ts_sfr(client)==0)
				{
					return 0;
				}
			
				ret=zet6221_i2c_write_tsdata(client, ts_cmd_erase, 1);
				msleep(1);
			
				for(i=2;i<18;i++)
				{
					ts_out_data[i]=ts_in_data[i-2];
				}
				ts_out_data[0] = 0x21;
				ts_out_data[1] = 0xc5;
				ts_out_data[4] = 0xf1;
			
				ret=zet6221_i2c_write_tsdata(client, ts_out_data, 18);
				msleep(1);
			
				//
				ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

				msleep(1);
	
				printk("%02x ",ts_cmd[0]); 
	
				printk("\n(2)read : "); 

				ret=zet6221_i2c_read_tsdata(client, ts_in_data, 16);

				msleep(1);

				for(i=0;i<16;i++)
				{
					printk("%02x ",ts_in_data[i]); 
				}
				printk("\n"); 
				
			}
									
#endif					
            break; 
        case 0x6223:
        	ret = 1;
			ic_model = 1;
			for(i=0;i<8;i++)
			{
				fb[i]=fb23[i];
			}
            break; 
        default: 
            ret=0; 
    } 

	return ret;
}

u8 zet622x_ts_sfr(struct i2c_client *client)
{

	u8 ts_cmd[1] = {0x2C};
	u8 ts_in_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_cmd17[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int ret;
	int i;
	
	printk("\nwrite : "); 

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	msleep(1);
	
	printk("%02x ",ts_cmd[0]); 
	
	printk("\nread : "); 

	ret=zet6221_i2c_read_tsdata(client, ts_in_data, 16);

	msleep(1);

	for(i=0;i<16;i++)
	{
		ts_cmd17[i+1]=ts_in_data[i];
		printk("%02x ",ts_in_data[i]); 
	}
	printk("\n"); 

#if 1
	if(ts_in_data[14]!=0x3D && ts_in_data[14]!=0x7D)
	{
		return 0;
	}
#endif

	if(ts_in_data[14]!=0x3D)
	{
		ts_cmd17[15]=0x3D;
		
		ts_cmd17[0]=0x2B;	
		
		ret=zet6221_i2c_write_tsdata(client, ts_cmd17, 17);
	}
	
	return 1;
}

u8 zet622x_ts_masserase(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x24};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet622x_ts_pageerase(struct i2c_client *client,int npage)
{
	u8 ts_cmd[3] = {0x23,0x00,0x00};
	u8 len=0;
	int ret;

	switch(ic_model)
	{
		case 0: //6221
			ts_cmd[1]=npage;
			len=2;
			break;
		case 1: //6223
			ts_cmd[1]=npage & 0xff;
			ts_cmd[2]=npage >> 8;
			len=3;
			break;
	}

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, len);

	return 1;
}

u8 zet622x_ts_resetmcu(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x29};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet622x_ts_hwcmd(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0xB9};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

#ifdef ZET_DOWNLOADER
static u8 zet622x_ts_version(void)
{	
	int i;
		
	printk("pc: ");
	for(i=0;i<8;i++)
		printk("%02x ",pc[i]);
	printk("\n");
	
	printk("src: ");
	for(i=0;i<8;i++)
		printk("%02x ",zeitec_zet622x_firmware[fb[i]]);
	printk("\n");
	
	for(i=0;i<8;i++)
		if(pc[i]!=zeitec_zet622x_firmware[fb[i]])
			return 0;
			
	return 1;
}

//support compatible
static int __init zet622x_downloader( struct i2c_client *client )
{
	int BufLen = 0;
	int BufPage = 0;
	int BufIndex = 0;
	int ret;
	int i;
	
	int nowBufLen = 0;
	int nowBufPage = 0;
	int nowBufIndex = 0;
	int retryCount = 0;
	
	int i2cLength = 0;
	int bufOffset = 0;
	
begin_download:	
	ctp_wakeup(config_info.wakeup_number, 0, 0);
	msleep(20);

	//send password
	ret=zet622x_ts_sndpwd(client);
	msleep(200);
	
	if(ret<=0)
		return ret;
		
	ret=zet622x_ts_option(client);
	msleep(200);
	
	if(ret<=0)
		return ret;

/*****compare version*******/

	//0~3
	memset(zeitec_zet622x_page_in, 0x00, 131);
	
	switch(ic_model) {
	case 0: //6221
        	zeitec_zet622x_page_in[0] = 0x25;
        	zeitec_zet622x_page_in[1] = (fb[0] >> 7);//(fb[0]/128);
        	
        	i2cLength=2;
        	break;
	case 1: //6223
        	zeitec_zet622x_page_in[0] = 0x25;
        	zeitec_zet622x_page_in[1] = (fb[0] >> 7) & 0xff; //(fb[0]/128);
        	zeitec_zet622x_page_in[2] = (fb[0] >> 7) >> 8; //(fb[0]/128);
        	i2cLength=3;
		break;
	}
	
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet622x_page_in, i2cLength);

	if(ret<=0)
		return ret;
	
	zeitec_zet622x_page_in[0] = 0x0;
	zeitec_zet622x_page_in[1] = 0x0;
	zeitec_zet622x_page_in[2] = 0x0;

	ret=zet6221_i2c_read_tsdata(client, zeitec_zet622x_page_in, 128);


	if(ret<=0)
		return ret;
	
	printk("page=%d ",(fb[0] >> 7));//(fb[0]/128));
	for(i=0; i<4; i++) {
		pc[i] = zeitec_zet622x_page_in[(fb[i] & 0x7f)];//[(fb[i]%128)];
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));//(fb[i]%128));
	}
	printk("\n");
	
	/*
	printk("page=%d ",(fb[0] >> 7));
	for(i=0;i<4;i++)
	{
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));
	}
	printk("\n");
	*/
	
	//4~7
	memset(zeitec_zet622x_page_in, 0x00, 131);
	
	switch(ic_model) {
	case 0: //6221
		zeitec_zet622x_page_in[0] = 0x25;
		zeitec_zet622x_page_in[1] = (fb[4] >> 7);//(fb[4]/128);
		
		i2cLength=2;
		break;
	case 1: //6223
		zeitec_zet622x_page_in[0] = 0x25;
		zeitec_zet622x_page_in[1] = (fb[4] >> 7) & 0xff; //(fb[4]/128);
		zeitec_zet622x_page_in[2] = (fb[4] >> 7) >> 8; //(fb[4]/128);
		
		i2cLength=3;
		break;
	}
	
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet622x_page_in, i2cLength);

	if(ret<=0)
		return ret;
	
	zeitec_zet622x_page_in[0] = 0x0;
	zeitec_zet622x_page_in[1] = 0x0;
	zeitec_zet622x_page_in[2] = 0x0;

	ret=zet6221_i2c_read_tsdata(client, zeitec_zet622x_page_in, 128);

	printk("page=%d ",(fb[4] >> 7)); //(fb[4]/128));
	for(i=4;i<8;i++) {
		pc[i]=zeitec_zet622x_page_in[(fb[i] & 0x7f)]; //[(fb[i]%128)];
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));  //(fb[i]%128));
	}
	printk("\n");
	
	if(ret<=0)
		return ret;
	
	if(zet622x_ts_version()!=0)
		goto exit_download;
		
/*****compare version*******/
//proc_sfr:
	//sfr
	if(zet622x_ts_sfr(client)==0) {
		//comment out to disable sfr checking loop
		return 0;
#if 0
                ctp_wakeup(config_info.wakeup_number, 0, 20);
		msleep(20);
		goto begin_download;
		
#endif

	}
	msleep(20);
	
	//comment out to enable download procedure
	//return 1;
	
	//erase
	if(BufLen==0) {
		//mass erase
		//DPRINTK( "mass erase\n");
		zet622x_ts_masserase(client);
		msleep(200);

		BufLen=sizeof(zeitec_zet622x_firmware)/sizeof(char);
	} else {
		zet622x_ts_pageerase(client,BufPage);
		msleep(200);
	}
	
	while(BufLen>0) {
download_page:
		memset(zeitec_zet622x_page,0x00,131);
		dprintk(DEBUG_INIT, "Start: write page%d\n",BufPage);
		nowBufIndex=BufIndex;
		nowBufLen=BufLen;
		nowBufPage=BufPage;
		
		switch(ic_model) {
		case 0: //6221
			bufOffset = 2;
			i2cLength = 130;
			
			zeitec_zet622x_page[0] = 0x22;
			zeitec_zet622x_page[1] = BufPage;				
			break;
		case 1: //6223
			bufOffset = 3;
			i2cLength=131;
			
			zeitec_zet622x_page[0] = 0x22;
			zeitec_zet622x_page[1] = BufPage & 0xff;
			zeitec_zet622x_page[2] = BufPage >> 8;
			break;
		}
		
		if(BufLen > 128) {
			for(i=0; i<128; i++) {
				zeitec_zet622x_page[i+bufOffset] = zeitec_zet622x_firmware[BufIndex];
				BufIndex += 1;
			}

			BufLen -= 128;
		} else {
			for(i=0; i<BufLen; i++) {
				zeitec_zet622x_page[i+bufOffset] = zeitec_zet622x_firmware[BufIndex];
				BufIndex += 1;
			}

			BufLen = 0;
		}

		ret=zet6221_i2c_write_tsdata(client, zeitec_zet622x_page, i2cLength);
		msleep(50);
		
#if 1

		memset(zeitec_zet622x_page_in, 0x00, 131);
		switch(ic_model) {
		case 0: //6221
			zeitec_zet622x_page_in[0] = 0x25;
			zeitec_zet622x_page_in[1] = BufPage;
			i2cLength=2;
			break;
		case 1: //6223
			zeitec_zet622x_page_in[0] = 0x25;
			zeitec_zet622x_page_in[1] = BufPage & 0xff;
			zeitec_zet622x_page_in[2] = BufPage >> 8;
			i2cLength=3;
			break;
		}		
		
		ret=zet6221_i2c_write_tsdata(client, zeitec_zet622x_page_in, i2cLength);

		zeitec_zet622x_page_in[0] = 0x0;
		zeitec_zet622x_page_in[1] = 0x0;
		zeitec_zet622x_page_in[2] = 0x0;
		
		ret=zet6221_i2c_read_tsdata(client, zeitec_zet622x_page_in, 128);

		
		for(i=0; i<128; i++) {
			if(i < nowBufLen) {
				if(zeitec_zet622x_page[i+bufOffset]!=zeitec_zet622x_page_in[i]) {
					BufIndex = nowBufIndex;
					BufLen = nowBufLen;
					BufPage = nowBufPage;
				
					if(retryCount < 5) {
						retryCount++;
						goto download_page;
					} else {
						retryCount = 0;
						ctp_wakeup(config_info.wakeup_number, 0, 5);
						msleep(20);
						goto begin_download;
					}

				}
			}
		}
		
#endif
		retryCount=0;
		BufPage+=1;
	}

exit_download:

	zet622x_ts_resetmcu(client);
	msleep(100);
        ctp_wakeup(config_info.wakeup_number, 1, 0);
	msleep(200);
	return 1;

}
#endif
/*************************6221_FW*******************************/


static int __devinit zet6221_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct input_dev *input_dev;
	struct zet6221_tsdrv *zet6221_ts;
	int err = -1;
	
	dprintk(DEBUG_INIT, " =====zet6221_ts_probe=====\n");
	
	zet6221_ts = kzalloc(sizeof(struct zet6221_tsdrv), GFP_KERNEL);
	if (!zet6221_ts)	{
		err = -ENOMEM;
		printk("alloc_data_failed\n");
		goto exit_alloc_data_failed;
	}
	
	zet6221_ts->i2c_ts = client;
	this_client = client;
	i2c_set_clientdata(client, zet6221_ts);

	INIT_WORK(&zet6221_ts->work1, zet6221_ts_work);
	zet6221_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev)); //  workqueue
	if (!zet6221_ts->ts_workqueue) {
		printk("ts_workqueue ts_probe error ==========\n");
		return -ENOMEM;
	}
	
	/*   charger detect : write_cmd */
	INIT_WORK(&zet6221_ts->work2, write_cmd_work);
	zet6221_ts->ts_workqueue1 = create_singlethread_workqueue(dev_name(&client->dev)); //  workqueue
	if (!zet6221_ts->ts_workqueue1) {
		printk("ts_workqueue1 ts_probe error ==========\n");
		return -ENOMEM;
	}
	/*   charger detect : write_cmd */

	input_dev = input_allocate_device();
	if (!input_dev || !zet6221_ts) {
		err = -ENOMEM;
		printk("input_allocate_device fail!");
		goto fail_alloc_mem;
	}
	
	i2c_set_clientdata(client, zet6221_ts);

	input_dev->name = MJ5_TS_NAME;
	input_dev->phys = "zet6221_touch/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0100;

	ic_model = 0; // 0:6221 1:6223
	
#ifdef ZET_DOWNLOADER	
	if(zet622x_downloader(client) <= 0)
	        printk("ZET_DOWNLOADER fail!");	
#endif	

#if defined(TPINFO)
        ctp_wakeup(config_info.wakeup_number, 0, 5);  
		
        if(zet6221_ts_get_report_mode_t(client)<=0) {
	        goto exit_check_functionality_failed;
	}
		
        if(pc[3]!=0x8)  // not zeitec ic
	        goto exit_check_functionality_failed;
	
#else
	ResolutionX = X_MAX;
	ResolutionY = Y_MAX;
	FingerNum = FINGER_NUMBER;
	KeyNum = KEY_NUMBER;   
	if(KeyNum==0)
		bufLength  = 3+4*FingerNum;
	else
		bufLength  = 3+4*FingerNum+1;
#endif

	dprintk(DEBUG_INIT, "ResolutionX=%d ResolutionY=%d FingerNum=%d KeyNum=%d\n",
	        ResolutionX,ResolutionY,FingerNum,KeyNum);

#ifdef MT_TYPE_B
	input_mt_init_slots(input_dev, FingerNum);	
#endif
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);	
	

        set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit); 
        set_bit(ABS_MT_POSITION_X, input_dev->absbit); 
         set_bit(ABS_MT_POSITION_Y, input_dev->absbit); 
        set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit); 
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, P_MAX, 0, 0);
#if defined(VIRTUAL_KEY)
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, TP_AA_X_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, TP_AA_Y_MAX, 0, 0);
#else
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ResolutionX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ResolutionY, 0, 0);
#endif
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_MENU, input_dev->keybit);
	set_bit(KEY_HOME, input_dev->keybit);
	set_bit(KEY_SEARCH, input_dev->keybit);

	input_dev->evbit[0] = BIT(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);


	err = input_register_device(input_dev);
	if (err) {
	        printk("input_register_device fail!");
		goto fail_ip_reg;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	pr_info("==register_early_suspend =\n");
	zet6221_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	zet6221_ts->early_suspend.suspend = ts_early_suspend;
	zet6221_ts->early_suspend.resume = ts_late_resume;
	register_early_suspend(&zet6221_ts->early_suspend);
#endif

	zet6221_ts->input = input_dev;
	input_set_drvdata(zet6221_ts->input, zet6221_ts);

   	int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)zet6221_ts_interrupt,zet6221_ts);
	if (!int_handle) {
		printk("zet6221_ts_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	

	return 0;


exit_irq_request_failed:
        sw_gpio_irq_free(int_handle);
fail_ip_reg:  
        input_free_device(input_dev);      
exit_check_functionality_failed:        
fail_alloc_mem:
        cancel_work_sync(&zet6221_ts->work1);
	destroy_workqueue(zet6221_ts->ts_workqueue);
        cancel_work_sync(&zet6221_ts->work2);
	destroy_workqueue(zet6221_ts->ts_workqueue1);
        i2c_set_clientdata(client, NULL);
	kfree(zet6221_ts);
exit_alloc_data_failed:        
	return err;
		
}

static int __devexit zet6221_ts_remove(struct i2c_client *client)
{
	struct zet6221_tsdrv *zet6221_ts = i2c_get_clientdata(client);
	
	//del_timer_sync(&write_timer);
	printk("==zet6221_ts_remove=\n");
	sw_gpio_irq_free(int_handle);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&zet6221_ts->early_suspend);
#endif
        cancel_work_sync(&zet6221_ts->work1);
	destroy_workqueue(zet6221_ts->ts_workqueue);
        cancel_work_sync(&zet6221_ts->work2);
	destroy_workqueue(zet6221_ts->ts_workqueue1);
	input_unregister_device(zet6221_ts->input);
	input_free_device(zet6221_ts->input);
	kfree(zet6221_ts);
	
        i2c_set_clientdata(this_client, NULL);

	return 0;
}


static int ctp_get_system_config(void)
{   
        twi_id = config_info.twi_id;
        screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        return 1;
}

//Touch Screen
static const struct i2c_device_id zet6221_ts_idtable[] = {
       { ZET_TS_ID_NAME, 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, zet6221_ts_idtable);

static struct i2c_driver zet6221_ts_driver = {
	.class = I2C_CLASS_HWMON,//
	.probe	  = zet6221_ts_probe,
	.remove		= __devexit_p(zet6221_ts_remove),
	.id_table = zet6221_ts_idtable,	
	.driver = {
		.owner = THIS_MODULE,
		.name  = ZET_TS_ID_NAME,
	},
	.address_list	=normal_i2c, // 
};

static int __init zet6221_ts_init(void)
{

	int ret = -1;      
        dprintk(DEBUG_INIT, "****************************************************************\n");
        
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
        
	ctp_wakeup(config_info.wakeup_number, 0, 1); 
	msleep(10); 
		
	zet6221_ts_driver.detect = ctp_detect;

	ret = i2c_add_driver(&zet6221_ts_driver);
	dprintk(DEBUG_INIT, "****************************************************************\n");
	return ret;
	
}
module_init(zet6221_ts_init);

static void __exit zet6221_ts_exit(void)
{
    i2c_del_driver(&zet6221_ts_driver);
    input_free_platform_resource(&(config_info.input_type));
}
module_exit(zet6221_ts_exit);

void zet6221_set_ts_mode(u8 mode)
{
//	DPRINTK( "[Touch Screen]ts mode = %d \n", mode);
}
EXPORT_SYMBOL_GPL(zet6221_set_ts_mode);


MODULE_DESCRIPTION("ZET6221 I2C Touch Screen driver");
MODULE_LICENSE("GPL v2");


