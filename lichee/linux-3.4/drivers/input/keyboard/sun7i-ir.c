/*
 * Sun7i ir driver
 *
 * Copyright (C) 2011 newbie Co.Ltd
 * Author: liugang <liugang@newbietech.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <mach/clock.h>
#include <linux/gpio.h>
#include <mach/sys_config.h>

#include <mach/irqs-sun7i.h>
#include <mach/hardware.h>
#include <linux/clk.h>
#include <mach/gpio.h>
#include <linux/init-input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <mach/platform.h>

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif

#include "ir-keymap.h"

#define REPORT_REPEAT_KEY_VALUE

//#define SUN7I_IR_FPGA
#define CLK_CFG_NEW_20120806
//#define SUN7I_SYS_GPIO_CFG_EN
//#define SUN7I_SYS_CLK_CFG_EN
#define SUN7I_PRINT_SUSPEND_INFO



static struct gpio_hdle {
	script_item_u	val;
	script_item_value_type_e  type;		
}ir_gpio_hdle;

//Registers
#define IR_BASE			SW_VA_IR0_IO_BASE
#define IR_IRQNO			AW_IRQ_IR0
#define CCM_BASE		SW_VA_CCM_IO_BASE
#define PI_BASE			SW_VA_PORTC_IO_BASE

#define IR_REG(x) 		(x)
#define IR_CTRL_REG		IR_REG(0x00) 	//IR Control
#define IR_RXCFG_REG		IR_REG(0x10) 	//Rx Config
#define IR_RXDAT_REG		IR_REG(0x20) 	//Rx Data
#define IR_RXINTE_REG		IR_REG(0x2C) 	//Rx Interrupt Enable
#define IR_RXINTS_REG		IR_REG(0x30) 	//Rx Interrupt Status
#define IR_SPLCFG_REG		IR_REG(0x34) 	//IR Sample Config

//Bit Definition of IR_RXINTS_REG Register
#define IR_RXINTS_RXOF		(0x1<<0)	//Rx FIFO Overflow
#define IR_RXINTS_RXPE		(0x1<<1)	//Rx Packet End
#define IR_RXINTS_RXDA		(0x1<<4)	//Rx FIFO Data Available

#ifdef CONFIG_AW_FPGA_PLATFORM
#define IR_RXFILT_VAL		(16)   		//Filter Threshold = 8*42.7 = ~341us < 500us 
#define IR_RXIDLE_VAL  		(5)    		//Idle Threshold = (2+1)*128*42.7 = ~16.4ms > 9ms

#define IR_L1_MIN   		(160)  		//80*42.7 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN 
#define IR_L0_MIN  		(80)  		//40*42.7 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN 
#define IR_PMAX    		(52)  		//26*42.7 = ~1109us ~= 561*2, Pluse < IR_PMAX 
#define IR_DMID    		(52)  		//26*42.7 = ~1109us ~= 561*2, D1 > IR_DMID, D0 =< IR_DMID
#define IR_DMAX    		(106)  		//53*42.7 = ~2263us ~= 561*4, D < IR_DMAX
#else
//Frequency of Sample Clock = 23437.5Hz, Cycle is 42.7us
//Pulse of NEC Remote >560us
#define IR_RXFILT_VAL		(8)		//Filter Threshold = 8*42.7 = ~341us	< 500us
#define IR_RXIDLE_VAL		(2)   		//Idle Threshold = (2+1)*128*42.7 = ~16.4ms > 9ms

#define IR_L1_MIN		(80)		//80*42.7 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN
#define IR_L0_MIN		(40)		//40*42.7 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN
#define IR_PMAX			(26)		//26*42.7 = ~1109us ~= 561*2, Pluse < IR_PMAX
#define IR_DMID			(26)		//26*42.7 = ~1109us ~= 561*2, D1 > IR_DMID, D0 =< IR_DMID
#define IR_DMAX			(53)		//53*42.7 = ~2263us ~= 561*4, D < IR_DMAX
#endif /* CONFIG_AW_FPGA_PLATFORM */

#define IR_FIFO_SIZE		(16)    	//16Bytes

#define IR_ERROR_CODE		(0xffffffff)
#define IR_REPEAT_CODE		(0x00000000)
#define DRV_VERSION		"1.00"

static struct clk *apb_ir_clk;
static struct clk *ir_clk;


#ifdef CONFIG_HAS_EARLYSUSPEND
struct sun7i_ir_data {
	struct early_suspend early_suspend;
};
#endif

struct ir_raw_buffer {
	unsigned long dcnt;                  		/*Packet Count*/
	#define	IR_RAW_BUF_SIZE		128
	unsigned char buf[IR_RAW_BUF_SIZE];	
};

static unsigned int ir_cnt = 0;
static struct input_dev *ir_dev;
static struct timer_list *s_timer; 
static unsigned long ir_code=0;
static int timer_used=0;
static struct ir_raw_buffer	ir_rawbuf;
static int ir_wakeup = 0; 
static unsigned long power_key = 0; 
static unsigned long ir_addr_code = 0; 

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct sun7i_ir_data *ir_data;
#endif

static struct ir_config_info ir_info = {
	.input_type = IR_TYPE,
	.rate = 0,
};
static u32 debug_mask = 0x00;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk("***IR***" KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static inline void ir_reset_rawbuffer(void)
{
	ir_rawbuf.dcnt = 0;
}

static inline void ir_write_rawbuffer(unsigned char data)
{
	if (ir_rawbuf.dcnt < IR_RAW_BUF_SIZE) 
		ir_rawbuf.buf[ir_rawbuf.dcnt++] = data;
	else
		printk("ir_write_rawbuffer: IR Rx Buffer Full!!\n");
}

static inline unsigned char ir_read_rawbuffer(void)
{
	unsigned char data = 0x00;

	if(ir_rawbuf.dcnt > 0)
		data = ir_rawbuf.buf[--ir_rawbuf.dcnt];

	return data;
}

static inline int ir_rawbuffer_empty(void)
{
	return (ir_rawbuf.dcnt == 0);
}

static inline int ir_rawbuffer_full(void)
{
	return (ir_rawbuf.dcnt >= IR_RAW_BUF_SIZE);
}

static void ir_clk_cfg(void)
{
#ifndef CONFIG_AW_FPGA_PLATFORM
	unsigned long rate = 3000000; //3Mhz

	apb_ir_clk = clk_get(NULL, CLK_APB_IR0);
	if(!apb_ir_clk || IS_ERR(apb_ir_clk)) {
		printk("%s err: try to get apb_ir0 clock failed! line %d\n", __func__, __LINE__);
		return;
	}

	ir_clk = clk_get(NULL, CLK_MOD_IR0);
	if(!ir_clk || IS_ERR(ir_clk)) {
		printk("%s err: try to get ir0 clock failed! line %d\n", __func__, __LINE__);
		return;
	}

	if(clk_set_rate(ir_clk, rate))
		printk("%s err: set ir0 clock freq to 3M failed! line %d\n", __func__, __LINE__);

	if(clk_enable(apb_ir_clk))
		printk("%s err: try to enable apb_ir_clk failed! line %d\n", __func__, __LINE__);

	if(clk_enable(ir_clk))
		printk("%s err: try to enable apb_ir_clk failed! line %d\n", __func__, __LINE__);

#else

#ifdef CLK_CFG_NEW_20120806
	unsigned long tmp = 0;

	//Enable APB Clock for IR
	tmp = readl(CCM_BASE + 0x68);
	tmp |= 0x1 << 6;  //IR
	writel(tmp, CCM_BASE + 0x68);

	//config Special Clock for IR	(24/8=3MHz)
	tmp = readl(CCM_BASE + 0xB0);
	tmp &= ~(0x3 << 24);	//Select 24MHz
	tmp |=  (0x1 << 31);   	//Open Clock
	tmp &= ~(0x3 << 16);
	tmp |=  (0x3 << 16);	//Divisor = 8
	writel(tmp, CCM_BASE + 0xB0);
#else
	unsigned long tmp = 0;

	//Enable APB Clock for IR
	tmp = readl(CCM_BASE + 0x10);
	tmp |= 0x1<<10;  //IR
	writel(tmp, CCM_BASE + 0x10);

	//config Special Clock for IR	(24/8=3MHz)
	tmp = readl(CCM_BASE + 0x34);
	tmp &= ~(0x3<<8);
	tmp |= (0x1<<8);   	//Select 24MHz
	tmp |= (0x1<<7);   	//Open Clock
	tmp &= ~(0x3f<<0);
	tmp |= (7<<0);	 	//Divisor = 8
	writel(tmp, CCM_BASE + 0x34);
#endif /* CLK_CFG_NEW_20120806 */

#endif /* CONFIG_AW_FPGA_PLATFORM */
}

static void ir_clk_uncfg(void)
{
#ifndef CONFIG_AW_FPGA_PLATFORM
	clk_put(apb_ir_clk);
	clk_put(ir_clk);
#else	
#endif
	return;
}
static void ir_sys_cfg(void)
{
        int ret = -1;
        if (input_fetch_sysconfig_para(&(ir_info.input_type))) {
		printk("%s: ir_fetch_sysconfig_para err.\n", __func__);
		return ;
	}
	
	if (ir_info.ir_used == 0) {
		printk("*** ir_used set to 0 !\n");
		return;
	}
#ifndef CONFIG_AW_FPGA_PLATFORM
	ret = input_init_platform_resource(&(ir_info.input_type));
        if(0 != ret) {
        	printk("%s:ctp_ops.init_platform_resource err. \n", __func__);
        	goto end;
        }
#else
	unsigned long tmp;

	//config IO: PIOB4 to IR_Rx
	tmp = readl(PI_BASE + 0x24); //PIOB_CFG0_REG
	tmp &= ~(0xf<<16);
	tmp |= (0x2<<16);
	writel(tmp, PI_BASE + 0x24);
#endif

	ir_clk_cfg();
#ifndef CONFIG_AW_FPGA_PLATFORM
end:
	input_free_platform_resource(&(ir_info.input_type));
	return;
#endif
}

static void ir_sys_uncfg(void)
{
	//unsigned long tmp;
#ifndef CONFIG_AW_FPGA_PLATFORM
	input_free_platform_resource(&(ir_info.input_type));
#endif
	ir_clk_uncfg();

	return;	
}

static void ir_reg_cfg(void)
{
	unsigned long tmp = 0;

	/*Enable IR Mode*/
	tmp = 0x3<<4;
	writel(tmp, IR_BASE+IR_CTRL_REG);

	/*Config IR Smaple Register*/
#ifdef CONFIG_AW_FPGA_PLATFORM
	tmp = 0x3<<0; 	//Fsample = 24MHz/512, and IR_RXFILT_VAL ~ IR_DMAX doubled
#else
	tmp = 0x1<<0; 	//Fsample = 3MHz/128 = 23437.5Hz (42.7us)
#endif /* CONFIG_AW_FPGA_PLATFORM */
	tmp |= (IR_RXFILT_VAL&0x3f)<<2;	//Set Filter Threshold
	tmp |= (IR_RXIDLE_VAL&0xff)<<8; //Set Idle Threshold
	writel(tmp, IR_BASE+IR_SPLCFG_REG);

	/*Invert Input Signal*/
	writel(0x1<<2, IR_BASE+IR_RXCFG_REG);

	/*Clear All Rx Interrupt Status*/
	writel(0xff, IR_BASE+IR_RXINTS_REG);

	/*Set Rx Interrupt Enable*/
	tmp = (0x1<<4)|0x3;
	tmp |= ((IR_FIFO_SIZE>>1)-1)<<8; //Rx FIFO Threshold = FIFOsz/2;
	writel(tmp, IR_BASE+IR_RXINTE_REG);

	/*Enable IR Module*/
	tmp = readl(IR_BASE+IR_CTRL_REG);
	tmp |= 0x3;
	writel(tmp, IR_BASE+IR_CTRL_REG);

	return;
}

static void ir_setup(void)
{	
	dprintk(DEBUG_INIT, "ir_setup: ir setup start!!\n");

	ir_code = 0;
	timer_used = 0;
	ir_reset_rawbuffer();	
	ir_sys_cfg();	
	ir_reg_cfg();
	
	dprintk(DEBUG_INIT, "ir_setup: ir setup end!!\n");

	return;
}

static inline unsigned char ir_get_data(void)
{
	return (unsigned char)(readl(IR_BASE + IR_RXDAT_REG));
}

static inline unsigned long ir_get_intsta(void)
{
	return (readl(IR_BASE + IR_RXINTS_REG));
}

static inline void ir_clr_intsta(unsigned long bitmap)
{
	unsigned long tmp = readl(IR_BASE + IR_RXINTS_REG);

	tmp &= ~0xff;
	tmp |= bitmap&0xff;
	writel(tmp, IR_BASE + IR_RXINTS_REG);
}

static unsigned long ir_packet_handler(unsigned char *buf, unsigned long dcnt)
{
	unsigned long len;
	unsigned char val = 0x00;
	unsigned char last = 0x00;
	unsigned long code = 0;
	int bitCnt = 0;
	unsigned long i=0;

	//print_hex_dump_bytes("--- ", DUMP_PREFIX_NONE, buf, dcnt);

	dprintk(DEBUG_DATA_INFO, "dcnt = %d \n", (int)dcnt);
	
	/* Find Lead '1' */
	len = 0;
	for (i=0; i<dcnt; i++) {
		val = buf[i];
		if (val & 0x80) {
			len += val & 0x7f;
		} else {
			if (len > IR_L1_MIN)
				break;
			
			len = 0;
		}
	}

	if ((val&0x80) || (len<=IR_L1_MIN))
		return IR_ERROR_CODE; /* Invalid Code */
		
	/* Find Lead '0' */
	len = 0;
	for (; i<dcnt; i++) {
		val = buf[i];		
		if (val & 0x80) {
			if(len > IR_L0_MIN)
				break;
			
			len = 0;
		} else {
			len += val & 0x7f;
		}		
	}
	
	if ((!(val&0x80)) || (len<=IR_L0_MIN))
		return IR_ERROR_CODE; /* Invalid Code */
	
	/* go decoding */
	code = 0;  /* 0 for Repeat Code */
	bitCnt = 0;
	last = 1;
	len = 0;
	for (; i<dcnt; i++) {
		val = buf[i];		
		if (last) {
			if (val & 0x80) {
				len += val & 0x7f;
			} else {
				if (len > IR_PMAX) {		/* Error Pulse */
					return IR_ERROR_CODE;
				}
				last = 0;
				len = val & 0x7f;
			}
		} else {
			if (val & 0x80) {
				if (len > IR_DMAX){		/* Error Distant */
					return IR_ERROR_CODE;
				} else {
					if (len > IR_DMID)  {
						/* data '1'*/
						code |= 1<<bitCnt;
					}
					bitCnt ++;
					if (bitCnt == 32)
						break;  /* decode over */
				}	
				last = 1;
				len = val & 0x7f;
			} else {
				len += val & 0x7f;
			}
		}
	}
	
	return code;
}

static int ir_code_valid(unsigned long code)
{
	unsigned long tmp1, tmp2;

#ifdef IR_CHECK_ADDR_CODE
	/* Check Address Value */
	if ((code&0xffff) != (ir_addr_code&0xffff))
		return 0; /* Address Error */
	
	tmp1 = code & 0x00ff0000;
	tmp2 = (code & 0xff000000)>>8;
	
	return ((tmp1^tmp2)==0x00ff0000);  /* Check User Code */
#else	
	/* Do Not Check Address Value */
	tmp1 = code & 0x00ff00ff;
	tmp2 = (code & 0xff00ff00)>>8;
	
	//return ((tmp1^tmp2)==0x00ff00ff);
	return (((tmp1^tmp2) & 0x00ff0000)==0x00ff0000 );
#endif /* #ifdef IR_CHECK_ADDR_CODE */
}

static irqreturn_t ir_irq_service(int irqno, void *dev_id)
{
	unsigned long intsta = ir_get_intsta();
	unsigned long rxint = readl(IR_BASE + IR_RXINTE_REG);
	unsigned long tmp;
	if(ir_wakeup){
		tmp = intsta >> 8; 
		//printk("=======fe3o4=========rxint:%x  intsta:%x intsta >> 8:%x,  intsta >> 8 & 0xf:%x\n",rxint , intsta, tmp, tmp & 0xf );
		if((intsta & 0xff) == 0x12 && (tmp & 0xf) < 0x8 ){ //  
		
			printk("==========fe3o4=====intsta:%x this is a wakeup interrupt !!! \n", intsta);
			input_report_key(ir_dev, ir_keycodes[power_key], 1);
			input_sync(ir_dev);
		}
	}
	
	dprintk(DEBUG_INT, "IR IRQ Serve\n");

	ir_clr_intsta(intsta);

	//if(intsta & (IR_RXINTS_RXDA|IR_RXINTS_RXPE))  /*FIFO Data Valid*/
	/*Read Data Every Time Enter this Routine*/
	{
		unsigned long dcnt =  (ir_get_intsta()>>8) & 0x1f;
		unsigned long i = 0;
		
		/* Read FIFO */
		for (i=0; i<dcnt; i++) {
			if (ir_rawbuffer_full()) {
			
				dprintk(DEBUG_INT, "ir_irq_service: Raw Buffer Full!!\n");
				
				break;
			} else {
				ir_write_rawbuffer(ir_get_data());
			}
		}
	}
	
	if (intsta & IR_RXINTS_RXPE) {	 /* Packet End */
		unsigned long code;
		int code_valid;
		
		code = ir_packet_handler(ir_rawbuf.buf, ir_rawbuf.dcnt);
		ir_rawbuf.dcnt = 0;
		code_valid = ir_code_valid(code);

		dprintk(DEBUG_INT, "%s: code 0x%08x, code_valid %d\n", __FUNCTION__, (u32)code, code_valid);


		if(timer_used) {
			if(code_valid) {  //the pre-key is released
			
				input_report_key(ir_dev, ir_keycodes[(ir_code>>16)&0xff], 0);
				input_sync(ir_dev);
				dprintk(DEBUG_INT,"%s: IR KEY UP\n", __FUNCTION__);
				ir_cnt = 0;
			}
			
			if((code==IR_REPEAT_CODE)||(code_valid)) {  //Error, may interfere from other source
				mod_timer(s_timer, jiffies + (HZ/5));
			}
		}else {
			if(code_valid) {
				if (!timer_pending(s_timer)) {
				        s_timer->expires = jiffies + (HZ/5);  //200ms timeout
				        add_timer(s_timer);
				} else
					mod_timer(s_timer, jiffies + (HZ/5));
					
				timer_used = 1;	
			}
		}

		if(timer_used) {
			ir_cnt++;
			if(ir_cnt == 1) {
				if(code_valid)
					ir_code = code;  /*update saved code with a new valid code*/

				dprintk(DEBUG_INT, "%s: IR RAW CODE : %lu\n", __FUNCTION__, (ir_code>>16)&0xff);
				input_report_key(ir_dev, ir_keycodes[(ir_code>>16)&0xff], 1);
				dprintk(DEBUG_INT, "%s: IR CODE : %d\n", __FUNCTION__, ir_keycodes[(ir_code>>16)&0xff]);
				input_sync(ir_dev);
				
				dprintk(DEBUG_INT, "IR KEY VALE %d\n",ir_keycodes[(ir_code>>16)&0xff]);
				
			}

			
		}
		
		dprintk(DEBUG_INT, "ir_irq_service: Rx Packet End, code=0x%x, ir_code=0x%x, timer_used=%d \n", (int)code, (int)ir_code, timer_used);
	}	
	
	if (intsta & IR_RXINTS_RXOF) {  /* FIFO Overflow */
		/* flush raw buffer */
		ir_reset_rawbuffer();		

		dprintk(DEBUG_INT, "ir_irq_service: Rx FIFO Overflow!!\n");

	}	
	
	return IRQ_HANDLED;
}

static void ir_timer_handle(unsigned long arg)
{
	del_timer(s_timer);
	timer_used = 0;

	/*Time Out, means that the key is up*/
	input_report_key(ir_dev, ir_keycodes[(ir_code>>16)&0xff], 0);
	input_sync(ir_dev);

	dprintk(DEBUG_INT, "IR KEY TIMER OUT UP\n");

	ir_cnt = 0;

	dprintk(DEBUG_INT, "ir_timer_handle: timeout \n");
}

//ͣ???豸
#ifdef CONFIG_HAS_EARLYSUSPEND
static void sun7i_ir_suspend(struct early_suspend *h)
{
/*	unsigned long tmp = 0;
	int ret;
	struct sun7i_ir_data *ts = container_of(h, struct sun7i_ir_data, early_suspend);

	tmp = readl(IR_BASE+IR_CTRL_REG);
	tmp &= 0xfffffffc;
	writel(tmp, IR_BASE+IR_CTRL_REG);
*/
	dprintk(DEBUG_SUSPEND, "EARLYSUSPEND:enter earlysuspend: sun7i_ir_suspend. \n");
        if(NULL == ir_clk || IS_ERR(ir_clk)) {
		printk("ir_clk handle is invalid, just return!\n");
		return;
	} else {	
		clk_disable(ir_clk);
	}
	
	if(NULL == apb_ir_clk || IS_ERR(ir_clk)) {
		printk("ir_clk handle is invalid, just return!\n");
		return;
	} else {	
		clk_disable(apb_ir_clk);
	}
	
}

//???»???
static void sun7i_ir_resume(struct early_suspend *h)
{

	dprintk(DEBUG_INIT, "EARLYSUSPEND:enter laterresume: sun7i_ir_resume. \n");
	
	ir_code = 0;
	timer_used = 0;
	ir_reset_rawbuffer();
	ir_clk_cfg();
	ir_reg_cfg();
}
#else
#endif

static int __init ir_init(void)
{
	int i,ret;
	int err =0;
        dprintk(DEBUG_INIT, "++++++++++++++++++++++\n");
	ir_dev = input_allocate_device();
	if (!ir_dev) {
		printk(KERN_ERR "ir_dev: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	ir_dev->name = "sun7i-ir";
	ir_dev->phys = "RemoteIR/input1";
	ir_dev->id.bustype = BUS_HOST;
	ir_dev->id.vendor = 0x0001;
	ir_dev->id.product = 0x0001;
	ir_dev->id.version = 0x0100;

#ifdef REPORT_REPEAT_KEY_VALUE
	ir_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP) ;
#else
	ir_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif
	//add by fe3o4 <<<

	 if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ir_para", "ir_wakeup", &ir_wakeup)){
			 pr_err("%s: ir_wakeup script_get_item error. \n",__func__ );
			ir_wakeup = 0;
	 }

	 if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ir_para", "power_key", &power_key)){
			 pr_err("%s: power_key script_get_item error. \n",__func__ );
			power_key = 0x57;
	 }

	 if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("ir_para", "ir_addr_code", &ir_addr_code)){
			 pr_err("%s: ir_addr_code script_get_item error. \n",__func__ );
			ir_addr_code = 0x9f00;
	 }

	printk("sun7i-ir script_get_item,ir_wakeup:%d, power_key:%x,ir_addr_code:%x\n", ir_wakeup, power_key, ir_addr_code);
	//add end >>>

	for (i = 0; i < 256; i++)
		set_bit(ir_keycodes[i], ir_dev->keybit);

	if (request_irq(IR_IRQNO, ir_irq_service, 0, "RemoteIR", ir_dev)) {
		err = -EBUSY;
		goto fail2;
	}

	ir_setup();

	s_timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
	if(!s_timer) {
		ret =  - ENOMEM;
		printk(" IR FAIL TO  Request Time\n");
		goto fail3;
	}
	init_timer(s_timer);
	s_timer->function = &ir_timer_handle;

	err = input_register_device(ir_dev);
	if (err)
		goto fail4;
	dprintk(DEBUG_INIT, "IR Initial OK\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	dprintk(DEBUG_INIT, "==register_early_suspend =\n");
	ir_data = kzalloc(sizeof(*ir_data), GFP_KERNEL);
	if (ir_data == NULL) {
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}

	ir_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	// closed by fe3o4 <<<
	if(!ir_wakeup){
		ir_data->early_suspend.suspend = sun7i_ir_suspend;
		ir_data->early_suspend.resume	= sun7i_ir_resume;
		register_early_suspend(&ir_data->early_suspend);
	}
	
	
#endif

	dprintk(DEBUG_INIT, "ir_init end\n");
        dprintk(DEBUG_INIT,"+++++++++++++++++++\n");
	return 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
err_alloc_data_failed:
#endif

fail4:
        printk("fail4\n");
	kfree(s_timer);
fail3:
        printk("fail3\n");
	free_irq(IR_IRQNO, ir_dev);
fail2:
        printk("fail2\n");
	input_free_device(ir_dev);
fail1:
        printk("fail1\n");
	return err;
	
}

static void __exit ir_exit(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ir_data->early_suspend);
#endif

	free_irq(IR_IRQNO, ir_dev);
	input_unregister_device(ir_dev);
	ir_sys_uncfg();
	kfree(s_timer);
}

module_init(ir_init);
module_exit(ir_exit);

MODULE_DESCRIPTION("Remote IR driver");
MODULE_AUTHOR("liugang");
MODULE_LICENSE("GPL");

