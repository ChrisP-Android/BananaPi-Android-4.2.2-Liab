/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
*
* Copyright (c) 2011
*
* ChangeLog
*
*
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h> 
#include <mach/irqs-sun7i.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
    #include <linux/pm.h>
    #include <linux/earlysuspend.h>
#endif


#define SW_INT_IRQNO_LRADC       AW_IRQ_LRADC


#define INPUT_DEV_NAME	        ("sw-keyboard")

#define  KEY_MAX_CNT  		(13)
 
#define  KEY_BASSADDRESS	(0xf1c22800)
#define  LRADC_CTRL		(0x00)
#define  LRADC_INTC		(0x04)
#define  LRADC_INT_STA 		(0x08)
#define  LRADC_DATA0		(0x0c)
#define  LRADC_DATA1		(0x10)

#define  FIRST_CONCERT_DLY	(2<<24)
#define  CHAN			(0x3)
#define  ADC_CHAN_SELECT	(CHAN<<22)
#define  LRADC_KEY_MODE		(0)
#define  KEY_MODE_SELECT	(LRADC_KEY_MODE<<12)
#define  LEVELB_VOL		(0<<4)

#define  LRADC_HOLD_EN		(1<<6)

#define  LRADC_SAMPLE_32HZ	(3<<2)
#define  LRADC_SAMPLE_62HZ	(2<<2)
#define  LRADC_SAMPLE_125HZ	(1<<2)
#define  LRADC_SAMPLE_250HZ	(0<<2)


#define  LRADC_EN		(1<<0)

#define  LRADC_ADC1_UP_EN	(1<<12)
#define  LRADC_ADC1_DOWN_EN	(1<<9)
#define  LRADC_ADC1_DATA_EN	(1<<8)

#define  LRADC_ADC0_UP_EN	(1<<4)
#define  LRADC_ADC0_DOWN_EN	(1<<1)
#define  LRADC_ADC0_DATA_EN	(1<<0)

#define  LRADC_ADC1_UPPEND	(1<<12)
#define  LRADC_ADC1_DOWNPEND	(1<<9)
#define  LRADC_ADC1_DATAPEND	(1<<8)


#define  LRADC_ADC0_UPPEND 	(1<<4)
#define  LRADC_ADC0_DOWNPEND	(1<<1)
#define  LRADC_ADC0_DATAPEND	(1<<0)

#define EVB
//#define CUSTUM
#define ONE_CHANNEL
#define MODE_0V2
//#define MODE_0V15
//#define TWO_CHANNEL
#ifdef MODE_0V2
//standard of key maping
//0.2V mode	 

#define REPORT_START_NUM			(2)
#define REPORT_KEY_LOW_LIMIT_COUNT		(1)
#define MAX_CYCLE_COUNTER			(100)
//#define REPORT_REPEAT_KEY_BY_INPUT_CORE
//#define REPORT_REPEAT_KEY_FROM_HW
#define INITIAL_VALUE				(0Xff)

static unsigned char keypad_mapindex[64] =
{
    0,0,0,0,0,0,0,0,               //key 1, 8???? 0-7
    1,1,1,1,1,1,1,                 //key 2, 7???? 8-14
    2,2,2,2,2,2,2,                 //key 3, 7???? 15-21
    3,3,3,3,3,3,                   //key 4, 6???? 22-27
    4,4,4,4,4,4,                   //key 5, 6???? 28-33
    5,5,5,5,5,5,                   //key 6, 6???? 34-39
    6,6,6,6,6,6,6,6,6,6,           //key 7, 10????40-49
    7,7,7,7,7,7,7,7,7,7,7,7,7,7    //key 8, 17????50-63
};
#endif
                        
#ifdef MODE_0V15
//0.15V mode
static unsigned char keypad_mapindex[64] =
{
	0,0,0,                      //key1
	1,1,1,1,1,                  //key2
	2,2,2,2,2,
	3,3,3,3,
	4,4,4,4,4,
	5,5,5,5,5,
	6,6,6,6,6,
	7,7,7,7,
	8,8,8,8,8,
	9,9,9,9,9,
	10,10,10,10,
	11,11,11,11,
	12,12,12,12,12,12,12,12,12,12 //key13
};
#endif

#ifdef EVB
static unsigned int sw_scankeycodes[KEY_MAX_CNT]=
{
	[0 ] = KEY_VOLUMEUP,       
	[1 ] = KEY_VOLUMEDOWN,      
	[2 ] = KEY_MENU,         
	[3 ] = KEY_SEARCH,       
	[4 ] = KEY_HOME,   
	[5 ] = KEY_ESC, 
	[6 ] = KEY_ENTER,        
	[7 ] = KEY_RESERVED,
	[8 ] = KEY_RESERVED,
	[9 ] = KEY_RESERVED,
	[10] = KEY_RESERVED,
	[11] = KEY_RESERVED,
	[12] = KEY_RESERVED,
};
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND 	
struct sw_keyboard_data {
    struct early_suspend early_suspend;
};
static struct sw_keyboard_data *keyboard_data;
#elif CONFIG_PM
static struct dev_pm_domain keyboard_pm_domain;
#endif

static volatile unsigned int key_val;
static struct input_dev *swkbd_dev;
static unsigned char scancode;
static unsigned char key_cnt = 0;
static unsigned char cycle_buffer[REPORT_START_NUM] = {0};
static unsigned char transfer_code = INITIAL_VALUE;

enum {
	DEBUG_INIT      = 1U << 0,
	DEBUG_INT       = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND   = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);



#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
static void _suspend(void)
{
        if (NORMAL_STANDBY == standby_type) {

		writel(0,KEY_BASSADDRESS + LRADC_CTRL);
	/*process for super standby*/	
	} else if(SUPER_STANDBY == standby_type) {
		;
	}
        
}

static void _resume(void)
{
	if (NORMAL_STANDBY == standby_type) {
			writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT|LRADC_SAMPLE_125HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
	/*process for super standby*/	
	} else if(SUPER_STANDBY == standby_type) {
		#ifdef ONE_CHANNEL
			writel(LRADC_ADC0_DOWN_EN|LRADC_ADC0_UP_EN|LRADC_ADC0_DATA_EN,KEY_BASSADDRESS + LRADC_INTC);	
			writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT|LRADC_SAMPLE_125HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
			//writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT|LRADC_SAMPLE_125HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
		#else
		#endif
	}        
}
#endif

//ͣ???豸
#ifdef CONFIG_HAS_EARLYSUSPEND
static void sw_keyboard_suspend(struct early_suspend *h)
{
	dprintk(DEBUG_SUSPEND,"EARLYSUSPEND:[%s] enter standby state: %d. \n",
	         __FUNCTION__, (int)standby_type);
	         
	_suspend();         

	return ;
}

//???»???
static void sw_keyboard_resume(struct early_suspend *h)
{

	dprintk(DEBUG_SUSPEND,"EARLYSUSPEND:[%s] return from standby state: %d. \n",
	         __FUNCTION__, (int)standby_type);

	_resume();

	return ; 
}
#elif CONFIG_PM
static int sw_keyboard_suspend(struct device *dev)
{
	dprintk(DEBUG_SUSPEND, "COMFIG_PM:[%s] enter standby state: %d. \n",
	         __FUNCTION__, (int)standby_type);
    
        _suspend();
	
	return 0;
}

/* ???»??? */
static int sw_keyboard_resume(struct device *dev)
{
	
	dprintk(DEBUG_SUSPEND, "COMFIG_PM:[%s] return from standby state: %d. \n", 
	        __FUNCTION__, (int)standby_type); 

        _resume();
	
	return 0; 
}
#endif


static irqreturn_t sw_isr_key(int irq, void *dummy)
{
	unsigned int  reg_val;
	int judge_flag = 0;
	int loop = 0;
	

	dprintk(DEBUG_INT, "Key Interrupt\n");
	reg_val  = readl(KEY_BASSADDRESS + LRADC_INT_STA);
        dprintk(DEBUG_INT, "reg_val: 0x%x \n", reg_val);

	if(reg_val&LRADC_ADC0_DOWNPEND){
                dprintk(DEBUG_DATA_INFO, "key down\n");
	}
	
	if(reg_val&LRADC_ADC0_DATAPEND){
		key_val = readl(KEY_BASSADDRESS+LRADC_DATA0);
                dprintk(DEBUG_DATA_INFO, "key_val: 0x%x \n", key_val);
		
		if(key_val < 0x3f) {
		        cycle_buffer[key_cnt%REPORT_START_NUM] = key_val & 0x3f;

		        if((key_cnt + 1) < REPORT_START_NUM){

		                //do not report key message
                        }else {

			        if(cycle_buffer[(key_cnt - REPORT_START_NUM + 1)%REPORT_START_NUM] 
			        == cycle_buffer[(key_cnt - REPORT_START_NUM + 2)%REPORT_START_NUM]) {
			                key_val = cycle_buffer[(key_cnt - REPORT_START_NUM + 1)%REPORT_START_NUM];
			                scancode = keypad_mapindex[key_val&0x3f];
			                judge_flag = 1;
			        }  
			        
			        dprintk(DEBUG_DATA_INFO, "cycle_buffer[(key_cnt - 1)] = 0x%x,cycle_buffer[(key_cnt -  2)]) = 0x%x\n",
			                cycle_buffer[(key_cnt -  1)],cycle_buffer[(key_cnt -  2)]); 

			        if(1 == judge_flag) {

				        dprintk(DEBUG_DATA_INFO,"report data: key_val :%8d transfer_code: %8d , scancode: %8d\n",\
				                key_val, transfer_code, scancode);


				        if(transfer_code == scancode){
                                                #ifdef REPORT_REPEAT_KEY_FROM_HW
				                input_report_key(swkbd_dev, sw_scankeycodes[scancode], 0);
				                input_sync(swkbd_dev);
				                input_report_key(swkbd_dev, sw_scankeycodes[scancode], 1);
				                input_sync(swkbd_dev);
                                                #else
				                //do not report key value
                                                #endif
				        }else if(INITIAL_VALUE != transfer_code) {                               
				                //report previous key value up signal + report current key value down
				                input_report_key(swkbd_dev, sw_scankeycodes[transfer_code], 0);
				                input_sync(swkbd_dev);
				                input_report_key(swkbd_dev, sw_scankeycodes[scancode], 1);
				                input_sync(swkbd_dev);
				                transfer_code = scancode;

				        }else {
				                //INITIAL_VALUE == transfer_code, first time to report key event
				                input_report_key(swkbd_dev, sw_scankeycodes[scancode], 1);
				                input_sync(swkbd_dev);
				                transfer_code = scancode;
				        }

			        }

			}
			
			key_cnt++;
			if(key_cnt > 2 * MAX_CYCLE_COUNTER ) {
			        key_cnt -= MAX_CYCLE_COUNTER;
			}

		}
	}
        
	if(reg_val&LRADC_ADC0_UPPEND) {
		if(key_cnt > REPORT_START_NUM) {
			if(INITIAL_VALUE != transfer_code) {
                                dprintk(DEBUG_DATA_INFO, "report data: key_val :%8d transfer_code: %8d \n",
                                        key_val, transfer_code);
			        input_report_key(swkbd_dev, sw_scankeycodes[transfer_code], 0);
			        input_sync(swkbd_dev);
			}

		}else if(key_cnt >= REPORT_KEY_LOW_LIMIT_COUNT){   
			//rely on hardware first_delay work, need to be verified!
			if(cycle_buffer[0] == cycle_buffer[1]){
				key_val = cycle_buffer[0];
				scancode = keypad_mapindex[key_val&0x3f];
				dprintk(DEBUG_DATA_INFO, "report data: key_val :%8d scancode: %8d \n",
				        key_val, scancode);
				input_report_key(swkbd_dev, sw_scankeycodes[scancode], 1);
				input_sync(swkbd_dev);   
				input_report_key(swkbd_dev, sw_scankeycodes[scancode], 0);
				input_sync(swkbd_dev);  
			}

		}

		dprintk(DEBUG_DATA_INFO,"key up \n");
		key_cnt = 0;
		judge_flag = 0;
		transfer_code = INITIAL_VALUE;
		
		for(loop = 0; loop < REPORT_START_NUM; loop++){
			cycle_buffer[loop] = 0; 
		}

	}
	
	writel(reg_val,KEY_BASSADDRESS + LRADC_INT_STA);
	return IRQ_HANDLED;
}

static int __init swkbd_init(void)
{
	int i;
	int err =0;	

	dprintk(DEBUG_INIT, "sun4ikbd_init \n");

	swkbd_dev = input_allocate_device();
	if (!swkbd_dev) {
		printk(KERN_ERR "sun4ikbd: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	swkbd_dev->name = INPUT_DEV_NAME;  
	swkbd_dev->phys = "swkbd/input0"; 
	swkbd_dev->id.bustype = BUS_HOST;      
	swkbd_dev->id.vendor = 0x0001;
	swkbd_dev->id.product = 0x0001;
	swkbd_dev->id.version = 0x0100;

#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	swkbd_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	dprintk(DEBUG_INIT, 
	        "REPORT_REPEAT_KEY_BY_INPUT_CORE is defined, support report repeat key value. \n");
#else
	swkbd_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif

	for (i = 0; i < KEY_MAX_CNT; i++)
		set_bit(sw_scankeycodes[i], swkbd_dev->keybit);
	
#ifdef ONE_CHANNEL
	writel(LRADC_ADC0_DOWN_EN|LRADC_ADC0_UP_EN|LRADC_ADC0_DATA_EN,KEY_BASSADDRESS + LRADC_INTC);	
	writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT|LRADC_SAMPLE_125HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
	//writel(FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|ADC_CHAN_SELECT|LRADC_SAMPLE_62HZ|LRADC_EN,KEY_BASSADDRESS + LRADC_CTRL);
#else
#endif

	if (request_irq(SW_INT_IRQNO_LRADC, sw_isr_key, 0, "swkbd", NULL)){
		err = -EBUSY;
		printk("request irq failure. \n");
		goto fail2;
	}

	err = input_register_device(swkbd_dev);
	if (err)
		goto fail3;

#ifdef CONFIG_HAS_EARLYSUSPEND	
	dprintk(DEBUG_INIT, "==register_early_suspend =\n");
	keyboard_data = kzalloc(sizeof(*keyboard_data), GFP_KERNEL);
	
	if (keyboard_data == NULL) {
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}

	keyboard_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 3;	
	keyboard_data->early_suspend.suspend = sw_keyboard_suspend;
	keyboard_data->early_suspend.resume = sw_keyboard_resume;
	register_early_suspend(&keyboard_data->early_suspend);
#elif CONFIG_PM
        keyboard_pm_domain.ops.suspend = sw_keyboard_suspend;
	keyboard_pm_domain.ops.resume = sw_keyboard_resume;
	swkbd_dev->dev.pm_domain = &keyboard_pm_domain;	
#endif

	return 0;
#ifdef CONFIG_HAS_EARLYSUSPEND
 err_alloc_data_failed:
#endif
 fail3:	
	free_irq(SW_INT_IRQNO_LRADC, sw_isr_key);
 fail2:	
	input_free_device(swkbd_dev);
 fail1:
	dprintk(DEBUG_INIT, "swkbd_init failed. \n");
 return err;
}

static void __exit swkbd_exit(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND	
	 unregister_early_suspend(&keyboard_data->early_suspend);	
#endif
	free_irq(SW_INT_IRQNO_LRADC, sw_isr_key);
	input_unregister_device(swkbd_dev);
}

module_init(swkbd_init);
module_exit(swkbd_exit);


MODULE_AUTHOR(" <@>");
MODULE_DESCRIPTION("sw-keyboard driver");
MODULE_LICENSE("GPL");



