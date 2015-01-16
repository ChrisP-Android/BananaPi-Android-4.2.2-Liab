/*
 * drivers/i2c/busses/i2c-sun7i.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUN7I TWI Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <asm/irq.h>

#include <mach/gpio.h>
#include <mach/irqs-sun7i.h>
#include <mach/sys_config.h>
#include <mach/clock.h>
#include <mach/i2c.h>

#define SUN7I_I2C_DEBUG

#ifdef SUN7I_I2C_DEBUG
	#define I2C_DBG(x...)   printk(x)
#else
	#define I2C_DBG(x...)
#endif

#define I2C_ERR(x...)		printk(x)

#define SUN7I_I2C_OK      0
#define SUN7I_I2C_FAIL   -1
#define SUN7I_I2C_RETRY  -2
#define SUN7I_I2C_SFAIL  -3  /* start fail */
#define SUN7I_I2C_TFAIL  -4  /* stop  fail */

#define SYS_I2C_PIN

#ifndef SYS_I2C_PIN
#define _PIO_BASE_ADDRESS		(0x01c20800)
static void* __iomem gpio_addr = NULL;

/* gpio twi0 */
#define _Pn_CFG0(n) ( (n)*0x24 + 0x00 + gpio_addr )
#define _Pn_DRV0(n) ( (n)*0x24 + 0x14 + gpio_addr )
#define _Pn_PUL0(n) ( (n)*0x24 + 0x1C + gpio_addr )

/* gpio twi1 twi2 */
#define _Pn_CFG2(n) ( (n)*0x24 + 0x08 + gpio_addr )
#define _Pn_DRV1(n) ( (n)*0x24 + 0x18 + gpio_addr )
#define _Pn_PUL1(n) ( (n)*0x24 + 0x20 + gpio_addr )

#endif

static inline unsigned int twi_query_irq_status(void *base_addr);

/* I2C transfer status */
enum
{
	I2C_XFER_IDLE    = 0x1,
	I2C_XFER_START   = 0x2,
	I2C_XFER_RUNNING = 0x4,
};

struct sun7i_i2c {
	int 		    bus_num;
	unsigned int        status; /* start, running, idle */
	unsigned int        suspended:1;
	struct i2c_adapter  adap;

	spinlock_t          lock; /* syn */
	wait_queue_head_t   wait;

	struct i2c_msg      *msg;
	unsigned int	    msg_num;
	unsigned int	    msg_idx;
	unsigned int	    msg_ptr;

#ifndef CONFIG_AW_FPGA_PLATFORM
	struct clk	   *mclk;
	struct clk         *pclk;
	unsigned int       gpio_hdle;
#endif

	unsigned int       bus_freq;
	int		   irq;
	unsigned int 	   debug_state; /* log the twi machine state */

	void __iomem	   *base_addr;
	unsigned long	   iobase; /* for remove */
	unsigned long	   iosize; /* for remove */
};

#ifndef CONFIG_SUN7I_I2C_PRINT_TRANSFER_INFO
static int bus_transfer_dbg = -1;
#endif
/* clear the interrupt flag */
static inline void twi_clear_irq_flag(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val &= ~TWI_CTL_INTFLG;//0x 1111_0111
	writel(reg_val ,base_addr + TWI_CTL_REG);
	/* read two more times to make sure that interrupt flag does really be cleared */
	{
		unsigned int temp;
		temp = readl(base_addr + TWI_CTL_REG);
		temp |= readl(base_addr + TWI_CTL_REG);
	}
}

/* get data first, then clear flag */
static inline void twi_get_byte(void *base_addr, unsigned char  *buffer)
{
	*buffer = (unsigned char)( TWI_DATA_MASK & readl(base_addr + TWI_DATA_REG) );
	twi_clear_irq_flag(base_addr);
}

/* only get data, we will clear the flag when stop */
static inline void twi_get_last_byte(void *base_addr, unsigned char  *buffer)
{
	*buffer = (unsigned char)( TWI_DATA_MASK & readl(base_addr + TWI_DATA_REG) );
}

/* write data and clear irq flag to trigger send flow */
static inline void twi_put_byte(void *base_addr, const unsigned char *buffer)
{
	writel((unsigned int)*buffer, base_addr + TWI_DATA_REG);
	twi_clear_irq_flag(base_addr);
}

static inline void twi_enable_irq(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);

	/*
	 * 1 when enable irq for next operation, set intflag to 1 to prevent to clear it by a mistake
	 *   (intflag bit is write-0-to-clear bit)
	 * 2 Similarly, mask START bit and STOP bit to prevent to set it twice by a mistake
	 *   (START bit and STOP bit are self-clear-to-0 bits)
	 */
	reg_val |= (TWI_CTL_INTEN | TWI_CTL_INTFLG);
	reg_val &= ~(TWI_CTL_STA | TWI_CTL_STP);
	writel(reg_val, base_addr + TWI_CTL_REG);
}

static inline void twi_disable_irq(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val &= ~TWI_CTL_INTEN;
	writel(reg_val, base_addr + TWI_CTL_REG);
}

static inline void twi_disable_bus(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val &= ~TWI_CTL_BUSEN;
	writel(reg_val, base_addr + TWI_CTL_REG);
}

static inline void twi_enable_bus(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val |= TWI_CTL_BUSEN;
	writel(reg_val, base_addr + TWI_CTL_REG);
}

/* trigger start signal, the start bit will be cleared automatically */
static inline void twi_set_start(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val |= TWI_CTL_STA;
	writel(reg_val, base_addr + TWI_CTL_REG);
}

/* get start bit status, poll if start signal is sent */
static inline unsigned int twi_get_start(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val >>= 5;
	return reg_val & 1;
}

/* trigger stop signal, the stop bit will be cleared automatically */
static inline void twi_set_stop(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val |= TWI_CTL_STP;
	writel(reg_val, base_addr + TWI_CTL_REG);
}

/* get stop bit status, poll if stop signal is sent */
static inline unsigned int twi_get_stop(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val >>= 4;
	return reg_val & 1;
}

static inline void twi_disable_ack(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val &= ~TWI_CTL_ACK;
	writel(reg_val, base_addr + TWI_CTL_REG);
}

/* when sending ack or nack, it will send ack automatically */
static inline void twi_enable_ack(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	reg_val |= TWI_CTL_ACK;
	writel(reg_val, base_addr + TWI_CTL_REG);
}

/* get the interrupt flag */
static inline unsigned int twi_query_irq_flag(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CTL_REG);
	return (reg_val & TWI_CTL_INTFLG);//0x 0000_1000
}

/* get interrupt status */
static inline unsigned int twi_query_irq_status(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_STAT_REG);
	return (reg_val & TWI_STAT_MASK);
}

/* set twi clock
 *
 * clk_n: clock divider factor n
 * clk_m: clock divider factor m
 */
static inline void twi_clk_write_reg(unsigned int clk_n, unsigned int clk_m, void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CLK_REG);
	pr_debug("%s: clk_n = %d, clk_m = %d\n", __FUNCTION__, clk_n, clk_m);
	reg_val &= ~(TWI_CLK_DIV_M | TWI_CLK_DIV_N);
	reg_val |= ( clk_n |(clk_m << 3) );
	writel(reg_val, base_addr + TWI_CLK_REG);
}

/*
* Fin is APB CLOCK INPUT;
* Fsample = F0 = Fin/2^CLK_N;
* F1 = F0/(CLK_M+1);
* Foscl = F1/10 = Fin/(2^CLK_N * (CLK_M+1)*10);
* Foscl is clock SCL;100KHz or 400KHz
*
* clk_in: apb clk clock
* sclk_req: freqence to set in HZ
*/
static void twi_set_clock(unsigned int clk_in, unsigned int sclk_req, void *base_addr)
{
	unsigned int clk_m = 0;
	unsigned int clk_n = 0;
	unsigned int _2_pow_clk_n = 1;
	unsigned int src_clk      = clk_in/10;
	unsigned int divider      = src_clk/sclk_req;  // 400khz or 100khz
	unsigned int sclk_real    = 0;      // the real clock frequency

	if (divider == 0) {
		clk_m = 1;
		goto set_clk;
	}

	/* search clk_n and clk_m,from large to small value so that can quickly find suitable m & n. */
	while (clk_n < 8) { // 3bits max value is 8
		/* (m+1)*2^n = divider -->m = divider/2^n -1 */
		clk_m = (divider/_2_pow_clk_n) - 1;
		/* clk_m = (divider >> (_2_pow_clk_n>>1))-1 */
		while (clk_m < 16) { /* 4bits max value is 16 */
			sclk_real = src_clk/(clk_m + 1)/_2_pow_clk_n;  /* src_clk/((m+1)*2^n) */
			if (sclk_real <= sclk_req) {
				goto set_clk;
			}
			else {
				clk_m++;
			}
		}
		clk_n++;
		_2_pow_clk_n *= 2; /* mutilple by 2 */
	}

set_clk:
	twi_clk_write_reg(clk_n, clk_m, base_addr);

	return;
}

/* soft reset twi */
static inline void twi_soft_reset(void *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_SRST_REG);
	reg_val |= TWI_SRST_SRST;
	writel(reg_val, base_addr + TWI_SRST_REG);
}

/* Enhanced Feature Register */
static inline void twi_set_efr(void *base_addr, unsigned int efr)
{
	unsigned int reg_val = readl(base_addr + TWI_EFR_REG);

	reg_val &= ~TWI_EFR_MASK;
	efr     &= TWI_EFR_MASK;
	reg_val |= efr;
	writel(reg_val, base_addr + TWI_EFR_REG);
}

static int sun7i_i2c_xfer_complete(struct sun7i_i2c *i2c, int code);
static int sun7i_i2c_do_xfer(struct sun7i_i2c *i2c, struct i2c_msg *msgs, int num);

#ifndef CONFIG_AW_FPGA_PLATFORM
static void twi_set_gpio_sysconfig(struct sun7i_i2c *i2c)
{
	int cnt, i;
	char twi_para[16] = {0};
	char twi_used[16] = {0};
	script_item_u used, *list = NULL;
	script_item_value_type_e type;

	sprintf(twi_para, "twi%d_para", i2c->bus_num);
	sprintf(twi_used, "twi%d_used", i2c->bus_num);
	type = script_get_item(twi_para,twi_used, &used);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		I2C_ERR("[i2c%d] twi_used err!\n", i2c->bus_num);
		return;
	}

	if (1 == used.val) {
		/* 获取gpio list */
		cnt = script_get_pio_list(twi_para, &list);
		if (0 == cnt) {
			I2C_ERR("[i2c%d] get gpio list failed\n", i2c->bus_num);
			return;
		}

		/* 申请gpio */
		for (i = 0; i < cnt; i++)
			if (0 != gpio_request(list[i].gpio.gpio, NULL)){
			        I2C_ERR("[i2c%d] gpio_request i:%d, gpio:%d failed\n", i2c->bus_num, i, list[i].gpio.gpio);
				goto end;
			}

		/* 配置gpio list */
		if (0 != sw_gpio_setall_range(&list[0].gpio, cnt))
			I2C_ERR("[i2c%d] sw_gpio_setall_range failed\n", i2c->bus_num);
	}

	return;

end:
	/* 释放gpio */
	while (i--)
		gpio_free(list[i].gpio.gpio);
}

static int twi_request_gpio(struct sun7i_i2c *i2c)
{
#ifndef SYS_I2C_PIN
	if(i2c->bus_num == 0) {
	    /* configuration register */
	    unsigned int  reg_val = readl(_Pn_CFG0(1));
	    reg_val &= ~(0x77);/* PB0-PB1 TWI0 SCK,SDA */
	    reg_val |= (0x22);
	    writel(reg_val, _Pn_CFG0(1));

	    /* pull up resistor */
	    reg_val = readl(_Pn_PUL0(1));
	    reg_val &= ~(0xf);
	    reg_val |= 0x5;
	    writel(reg_val, _Pn_PUL0(1));

	    /* driving default */
	    reg_val = readl(_Pn_DRV0(1));
	    reg_val &= ~(0xf);
	    reg_val |= 0x5;
	    writel(reg_val, _Pn_DRV0(1));
	}
	else if(i2c->bus_num == 1) {
	    /* configuration register */
	    unsigned int  reg_val = readl(_Pn_CFG2(1));
	    reg_val &= ~(0x7700);/* PB18-PB19 TWI1 SCK,SDA */
	    reg_val |= (0x2200);
	    writel(reg_val, _Pn_CFG2(1));

	    /* pull up resistor */
	    reg_val = readl(_Pn_PUL1(1));
	    reg_val &= ~(0xf0);
	    reg_val |= 0x50;
	    writel(reg_val, _Pn_PUL1(1));

	    /* driving default */
	    reg_val = readl(_Pn_DRV1(1));
	    reg_val &= ~(0xf0);
	    reg_val |= 0x50;
	    writel(reg_val, _Pn_DRV1(1));
	}
	else if(i2c->bus_num == 2) {
	    /* configuration register */
	    unsigned int  reg_val = readl(_Pn_CFG2(1));
	    reg_val &= ~(0x770000);/* PB20-PB21 TWI2 SCK,SDA */
	    reg_val |= (0x220000);
	    writel(reg_val, _Pn_CFG2(1));

	    /* pull up resistor */
	    reg_val = readl(_Pn_PUL1(1));
	    reg_val &= ~(0xf00);
	    reg_val |= 0x500;
	    writel(reg_val, _Pn_PUL1(1));

	    /* driving default */
	    reg_val = readl(_Pn_DRV1(1));
	    reg_val &= ~(0xf00);
	    reg_val |= 0x500;
	    writel(reg_val, _Pn_DRV1(1));
	}
#else
        twi_set_gpio_sysconfig(i2c);
#endif

	return 0;
}

static void twi_release_gpio(struct sun7i_i2c *i2c)
{
#ifndef SYS_I2C_PIN
	if(i2c->bus_num == 0) {
	    /* config reigster */
	    unsigned int  reg_val = readl(_Pn_CFG0(1));
	    reg_val &= ~(0x77);/* PB0-PB1 TWI0 SCK,SDA */
	    writel(reg_val, _Pn_CFG0(1));

	    /* driving register */
	    reg_val = readl(_Pn_DRV0(1));
	    reg_val &= ~(0xf);
	    reg_val |= 0x5;
	    writel(reg_val, _Pn_DRV0(1));

	    /* pull up resistor */
	    reg_val = readl(_Pn_PUL0(1));
	    reg_val &= ~(0xf);
	    reg_val |= 0x5;
	    writel(reg_val, _Pn_PUL0(1));
	}
	else if(i2c->bus_num == 1){
	    /* config reigster */
	    unsigned int  reg_val = readl(_Pn_CFG2(1));
	    reg_val &= ~(0x7700);/* PB18-PB19 TWI1 SCK,SDA */
	    writel(reg_val, _Pn_CFG2(1));

	    /* driving register */
	    reg_val = readl(_Pn_DRV1(1));
	    reg_val &= ~(0xf0);
	    reg_val |= 0x50;
	    writel(reg_val, _Pn_DRV1(1));

	    /* pull up resistor */
	    reg_val = readl(_Pn_PUL1(1));
	    reg_val &= ~(0xf0);
	    reg_val |= 0x50;
	    writel(reg_val, _Pn_PUL1(1));
	}
	else if(i2c->bus_num == 2){
	    /* config reigster */
	    unsigned int  reg_val = readl(_Pn_CFG2(1));
	    reg_val &= ~(0x770000);/* PB20-PB21 TWI2 SCK,SDA */
	    writel(reg_val, _Pn_CFG2(1));

	    /* driving register */
		reg_val = readl(_Pn_DRV1(1));
	    reg_val &= ~(0xf00);
	    reg_val |= 0x500;
	    writel(reg_val, _Pn_DRV1(1));

	    /* pull up resistor */
	    reg_val = readl(_Pn_PUL1(1));
	    reg_val &= ~(0xf00);
	    reg_val |= 0x500;
	    writel(reg_val, _Pn_PUL1(1));
	}
#else
        int gpio_cnt, i;
	script_item_u *list = NULL;
	char twi_para[16] = {0};

	sprintf(twi_para, "twi%d_para", i2c->bus_num);
	gpio_cnt = script_get_pio_list(twi_para, &list);
	for(i = 0; i < gpio_cnt; i++)
		gpio_free(list[i].gpio.gpio);
#endif
}
#endif

/* function  */
static int twi_start(void *base_addr, int bus_num)
{
	unsigned int timeout = 0xff;

	twi_set_start(base_addr);
	while((1 == twi_get_start(base_addr))&&(--timeout));
	if(timeout == 0) {
		I2C_ERR("[i2c%d] START can't sendout!\n", bus_num);
		return SUN7I_I2C_FAIL;
	}

	return SUN7I_I2C_OK;
}

static int twi_restart(void  *base_addr, int bus_num)
{
	unsigned int timeout = 0xff;
	twi_set_start(base_addr);
	twi_clear_irq_flag(base_addr);
	while((1 == twi_get_start(base_addr))&&(--timeout));
	if(timeout == 0) {
		I2C_ERR("[i2c%d] Restart can't sendout!\n", bus_num);
		return SUN7I_I2C_FAIL;
	}
	return SUN7I_I2C_OK;
}

static int twi_stop(void *base_addr, int bus_num)
{
	unsigned int timeout = 0xff;

	twi_set_stop(base_addr);
	twi_clear_irq_flag(base_addr);

	twi_get_stop(base_addr);/* it must delay 1 nop to check stop bit */
	while(( 1 == twi_get_stop(base_addr))&& (--timeout));
	if(timeout == 0) {
		I2C_ERR("[i2c%d] STOP can't sendout!\n", bus_num);
		return SUN7I_I2C_TFAIL;
	}

	timeout = 0xffff;
	while((TWI_STAT_IDLE != readl(base_addr + TWI_STAT_REG))&&(--timeout));
	if(timeout == 0)
	{
		I2C_ERR("[i2c%d] i2c state isn't idle(0xf8)\n", bus_num);
		return SUN7I_I2C_TFAIL;
	}

	timeout = 0xff;
	while((TWI_LCR_IDLE_STATUS != readl(base_addr + TWI_LCR_REG))&&(--timeout));
	if(timeout == 0) {
		I2C_ERR("[i2c%d] i2c lcr isn't idle(0x3a)\n", bus_num);
		return SUN7I_I2C_TFAIL;
	}

	return SUN7I_I2C_OK;
}

/* get SDA state */
static unsigned int twi_get_sda(void *base_addr)
{
        unsigned int status = 0;
        status = TWI_LCR_SDA_STATE_MASK & readl(base_addr + TWI_LCR_REG);
        status >>= 4;
        return  (status&0x1);
}

/* set SCL level(high/low), only when SCL enable */
static void twi_set_scl(void *base_addr, unsigned int hi_lo)
{
        unsigned int reg_val = readl(base_addr + TWI_LCR_REG);
        reg_val &= ~TWI_LCR_SCL_CTL;
        hi_lo   &= 0x01;
        reg_val |= (hi_lo<<3);
        writel(reg_val, base_addr + TWI_LCR_REG);
}

/* enable SDA or SCL */
static void twi_enable_lcr(void *base_addr, unsigned int sda_scl)
{
        unsigned int reg_val = readl(base_addr + TWI_LCR_REG);
        sda_scl &= 0x01;
        
        if(sda_scl) {
                reg_val |= TWI_LCR_SCL_EN;/* enable scl line control */
        } else {
                reg_val |= TWI_LCR_SDA_EN;/* enable sda line control */
        }
        writel(reg_val, base_addr + TWI_LCR_REG);
}

/* disable SDA or SCL */
static void twi_disable_lcr(void *base_addr, unsigned int sda_scl)
{
        unsigned int reg_val = readl(base_addr + TWI_LCR_REG);
        sda_scl &= 0x01;
        
        if(sda_scl) {
                reg_val &= ~TWI_LCR_SCL_EN;/* disable scl line control */
        } else {
                reg_val &= ~TWI_LCR_SDA_EN;/* disable sda line control */
        }
        writel(reg_val, base_addr + TWI_LCR_REG);
}

/* send 9 clock to release sda */
static int twi_send_clk_9pulse(void *base_addr, int bus_num)
{
        int twi_scl = 1;
        int low = 0;
        int high = 1;
        int cycle = 0;

        /* enable scl control */
        twi_enable_lcr(base_addr, twi_scl);

        while(cycle < 10) {
                if( twi_get_sda(base_addr)
                && twi_get_sda(base_addr)
                && twi_get_sda(base_addr) ) {
                        break;
                }
                /* twi_scl -> low */
                twi_set_scl(base_addr, low);
                udelay(1000);

                /* twi_scl -> high */
                twi_set_scl(base_addr, high);
                udelay(1000);
                cycle++;
        }

        if(twi_get_sda(base_addr)){
                twi_disable_lcr(base_addr, twi_scl);
                return SUN7I_I2C_OK;
        } else {
        I2C_ERR("[i2c%d] SDA is still Stuck Low, failed. \n", bus_num);
                twi_disable_lcr(base_addr, twi_scl);
                return SUN7I_I2C_FAIL;
        }
}

/*
****************************************************************************************************
*
*  FunctionName:           sun7i_i2c_addr_byte
*
*  Description:
*            发送slave地址，7bit的全部信息，及10bit的第一部分地址。供外部接口调用，内部实现。
*         7bits addr: 7-1bits addr+0 bit r/w
*         10bits addr: 1111_11xx_xxxx_xxxx-->1111_0xx_rw,xxxx_xxxx
*         send the 7 bits addr,or the first part of 10 bits addr
*  Parameters:
*
*
*  Return value:
*           无
*  Notes:
*
****************************************************************************************************
*/
static void sun7i_i2c_addr_byte(struct sun7i_i2c *i2c)
{
	unsigned char addr = 0;
	unsigned char tmp  = 0;

	if(i2c->msg[i2c->msg_idx].flags & I2C_M_TEN) {
		/* 0111_10xx,ten bits address--9:8bits */
		tmp = 0x78 | ( ( (i2c->msg[i2c->msg_idx].addr)>>8 ) & 0x03);
		addr = tmp << 1;//1111_0xx0
		/* how about the second part of ten bits addr? Answer: deal at twi_core_process() */
	} else {
		/* 7-1bits addr, xxxx_xxx0 */
		addr = (i2c->msg[i2c->msg_idx].addr & 0x7f) << 1;
	}

	/* read, default value is write */
	if (i2c->msg[i2c->msg_idx].flags & I2C_M_RD) {
		addr |= 1;
	}

#ifdef CONFIG_SUN7I_I2C_PRINT_TRANSFER_INFO
	 if(i2c->bus_num == CONFIG_SUN7I_I2C_PRINT_TRANSFER_INFO_WITH_BUS_NUM){
		if(i2c->msg[i2c->msg_idx].flags & I2C_M_TEN) {
			I2C_DBG("[i2c%d] first part of 10bits = 0x%x\n", i2c->bus_num, addr);
		}
		I2C_DBG("[i2c%d] 7bits+r/w = 0x%x\n", i2c->bus_num, addr);
	}
#else
	if (unlikely(bus_transfer_dbg != -1)) {
		if (i2c->bus_num == bus_transfer_dbg) {
			if (i2c->msg[i2c->msg_idx].flags & I2C_M_TEN) {
				I2C_DBG("[i2c%d] first part of 10bits = 0x%x\n", i2c->bus_num, addr);
			}
			I2C_DBG("[i2c%d] 7bits+r/w = 0x%x\n", i2c->bus_num, addr);
		}
	}
#endif
	/* send 7bits+r/w or the first part of 10bits */
	twi_put_byte(i2c->base_addr, &addr);
}


static int sun7i_i2c_core_process(struct sun7i_i2c *i2c)
{
	void *base_addr = i2c->base_addr;
	int  ret        = SUN7I_I2C_OK;
	int  err_code   = 0;
	unsigned char  state = 0;
	unsigned char  tmp   = 0;

	state = twi_query_irq_status(base_addr);

#ifdef CONFIG_SUN7I_I2C_PRINT_TRANSFER_INFO
	if(i2c->bus_num == CONFIG_SUN7I_I2C_PRINT_TRANSFER_INFO_WITH_BUS_NUM){
		I2C_DBG("[i2c%d][slave address = (0x%x), state = (0x%x)]\n", i2c->bus_num, i2c->msg->addr, state);
	}
#else
	if (unlikely(bus_transfer_dbg != -1)) {
		if (i2c->bus_num == bus_transfer_dbg) {
			I2C_DBG("[i2c%d][slave address = (0x%x), state = (0x%x)]\n", i2c->bus_num, i2c->msg->addr, state);
		}
	}
#endif

    if(i2c->msg == NULL) {
        I2C_ERR("[i2c%d] i2c message is NULL, err_code = 0xfe\n", i2c->bus_num);
        err_code = 0xfe;
        goto msg_null;
    }

	switch(state) {
	case 0xf8: /* On reset or stop the bus is idle, use only at poll method */
		err_code = 0xf8;
		goto err_out;
	case 0x08: /* A START condition has been transmitted */
	case 0x10: /* A repeated start condition has been transmitted */
		sun7i_i2c_addr_byte(i2c);/* send slave address */
		break;
	case 0xd8: /* second addr has transmitted, ACK not received!    */
	case 0x20: /* SLA+W has been transmitted; NOT ACK has been received */
		err_code = 0x20;
		goto err_out;
	case 0x18: /* SLA+W has been transmitted; ACK has been received */
		/* if any, send second part of 10 bits addr */
		if(i2c->msg[i2c->msg_idx].flags & I2C_M_TEN) {
			tmp = i2c->msg[i2c->msg_idx].addr & 0xff;  /* the remaining 8 bits of address */
			twi_put_byte(base_addr, &tmp); /* case 0xd0: */
			break;
		}
		/* for 7 bit addr, then directly send data byte--case 0xd0:  */
	case 0xd0: /* second addr has transmitted,ACK received!     */
	case 0x28: /* Data byte in DATA REG has been transmitted; ACK has been received */
		/* after send register address then START send write data  */
		if(i2c->msg_ptr < i2c->msg[i2c->msg_idx].len) {
			twi_put_byte(base_addr, &(i2c->msg[i2c->msg_idx].buf[i2c->msg_ptr]));
			i2c->msg_ptr++;
			break;
		}

		i2c->msg_idx++; /* the other msg */
		i2c->msg_ptr = 0;
		
		if (i2c->msg_idx == i2c->msg_num) {
			err_code = SUN7I_I2C_OK;/* Success,wakeup */
			goto ok_out;
		} else if(i2c->msg_idx < i2c->msg_num) {/* for restart pattern */
			ret = twi_restart(base_addr, i2c->bus_num);/* read spec, two msgs */
			
			if(ret == SUN7I_I2C_FAIL) {
				err_code = SUN7I_I2C_SFAIL;
				goto err_out;/* START can't sendout */
			}
		} else {
			err_code = SUN7I_I2C_FAIL;
			goto err_out;
		}
		break;
	case 0x30: /* Data byte in I2CDAT has been transmitted; NOT ACK has been received */
		err_code = 0x30;//err,wakeup the thread
		goto err_out;
	case 0x38: /* Arbitration lost during SLA+W, SLA+R or data bytes */
		err_code = 0x38;//err,wakeup the thread
		goto err_out;
	case 0x40: /* SLA+R has been transmitted; ACK has been received */
		/* with Restart,needn't to send second part of 10 bits addr,refer-"I2C-SPEC v2.1" */
		/* enable A_ACK need it(receive data len) more than 1. */
		if(i2c->msg[i2c->msg_idx].len > 1) {
			/* send register addr complete,then enable the A_ACK and get ready for receiving data */
			twi_enable_ack(base_addr);
			twi_clear_irq_flag(base_addr);/* jump to case 0x50 */
		} else if(i2c->msg[i2c->msg_idx].len == 1) {
			twi_clear_irq_flag(base_addr);/* jump to case 0x58 */
		}
		break;
	case 0x48: /* SLA+R has been transmitted; NOT ACK has been received */
		err_code = 0x48;//err,wakeup the thread
		goto err_out;
	case 0x50: /* Data bytes has been received; ACK has been transmitted */
		/* receive first data byte */
		if (i2c->msg_ptr < i2c->msg[i2c->msg_idx].len) {
			/* more than 2 bytes, the last byte need not to send ACK */
			if( (i2c->msg_ptr + 2) == i2c->msg[i2c->msg_idx].len ) {
				twi_disable_ack(base_addr);/* last byte no ACK */
			}
			/* get data then clear flag,then next data comming */
			twi_get_byte(base_addr, &i2c->msg[i2c->msg_idx].buf[i2c->msg_ptr]);
			i2c->msg_ptr++;
			break;
		}
		/* err process, the last byte should be @case 0x58 */
		err_code = SUN7I_I2C_FAIL;/* err, wakeup */
		goto err_out;
	case 0x58: /* Data byte has been received; NOT ACK has been transmitted */
		/* received the last byte  */
		if ( i2c->msg_ptr == i2c->msg[i2c->msg_idx].len - 1 ) {
			twi_get_last_byte(base_addr, &i2c->msg[i2c->msg_idx].buf[i2c->msg_ptr]);
			i2c->msg_idx++;
			i2c->msg_ptr = 0;
			if (i2c->msg_idx == i2c->msg_num) {
				err_code = SUN7I_I2C_OK; // succeed,wakeup the thread
				goto ok_out;
			} else if(i2c->msg_idx < i2c->msg_num) {
				/* repeat start */
				ret = twi_restart(base_addr, i2c->bus_num);
				if(ret == SUN7I_I2C_FAIL) {/* START fail */
					err_code = SUN7I_I2C_SFAIL;
					goto err_out;
				}
				break;
			}
		} else {
			err_code = 0x58;
			goto err_out;
		}
	case 0x00: /* Bus error during master or slave mode due to illegal level condition */
		err_code = 0xff;
		goto err_out;
	default:
		err_code = state;
		goto err_out;
	}
	i2c->debug_state = state;/* just for debug */
	return ret;

ok_out:
err_out:
	if(SUN7I_I2C_TFAIL == twi_stop(base_addr, i2c->bus_num)) {
		I2C_ERR("[i2c%d] STOP failed!\n", i2c->bus_num);
	}

msg_null:
	ret = sun7i_i2c_xfer_complete(i2c, err_code);/* wake up */
	i2c->debug_state = state;/* just for debug */
	return ret;
}

static irqreturn_t sun7i_i2c_handler(int this_irq, void * dev_id)
{
	struct sun7i_i2c *i2c = (struct sun7i_i2c *)dev_id;

	if(!twi_query_irq_flag(i2c->base_addr)) {
		I2C_ERR("unknown interrupt!\n");
		return IRQ_NONE;
	}

	/* disable irq */
	twi_disable_irq(i2c->base_addr);

	/* twi core process */
	sun7i_i2c_core_process(i2c);

	/* enable irq only when twi is transfering, otherwise disable irq */
	if(i2c->status != I2C_XFER_IDLE) {
		twi_enable_irq(i2c->base_addr);
	}

	return IRQ_HANDLED;
}

static int sun7i_i2c_xfer_complete(struct sun7i_i2c *i2c, int code)
{
	int ret = SUN7I_I2C_OK;

	i2c->msg     = NULL;
	i2c->msg_num = 0;
	i2c->msg_ptr = 0;
	i2c->status  = I2C_XFER_IDLE;

	/* i2c->msg_idx  store the information */
	if(code == SUN7I_I2C_FAIL) {
		I2C_ERR("[i2c%d] Maybe Logic Error, debug it!\n", i2c->bus_num);
		i2c->msg_idx = code;
		ret = SUN7I_I2C_FAIL;
	}
	else if(code != SUN7I_I2C_OK) {
		i2c->msg_idx = code;
		ret = SUN7I_I2C_FAIL;
	}

	wake_up(&i2c->wait);

	return ret;
}

static int sun7i_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct sun7i_i2c *i2c = (struct sun7i_i2c *)adap->algo_data;
	int ret = SUN7I_I2C_FAIL;
	int i   = 0;

	if(i2c->suspended) {
		I2C_ERR("[i2c%d] has already suspend, dev addr:0x%x!\n", i2c->adap.nr, msgs->addr);
		return -ENODEV;
	}

	for(i = adap->retries; i >= 0; i--) {
		ret = sun7i_i2c_do_xfer(i2c, msgs, num);

		if(ret != SUN7I_I2C_RETRY) {
			goto out;
		}

		I2C_DBG("[i2c%d] Retrying transmission %d\n", i2c->adap.nr, i);
		udelay(100);
	}

	ret = -EREMOTEIO;
out:
	return ret;
}

static int sun7i_i2c_do_xfer(struct sun7i_i2c *i2c, struct i2c_msg *msgs, int num)
{
	unsigned long timeout = 0;
	int ret = SUN7I_I2C_FAIL;
	//int i = 0, j =0;

	twi_soft_reset(i2c->base_addr);
	udelay(100);

	/* test the bus is free,already protect by the semaphore at DEV layer */
	while( TWI_STAT_IDLE != twi_query_irq_status(i2c->base_addr)&&
	       TWI_STAT_BUS_ERR != twi_query_irq_status(i2c->base_addr) &&
	       TWI_STAT_ARBLOST_SLAR_ACK != twi_query_irq_status(i2c->base_addr) ) {
		I2C_DBG("[i2c%d] bus is busy, status = %x\n", i2c->bus_num, twi_query_irq_status(i2c->base_addr));
                if( SUN7I_I2C_OK == twi_send_clk_9pulse(i2c->base_addr, i2c->bus_num) ) {
                        break;
                } else {
                        ret = SUN7I_I2C_RETRY;
                        goto out;
                }
	}

	/* may conflict with xfer_complete */
	spin_lock_irq(&i2c->lock);
	i2c->msg     = msgs;
	i2c->msg_num = num;
	i2c->msg_ptr = 0;
	i2c->msg_idx = 0;
	i2c->status  = I2C_XFER_START;
        twi_enable_irq(i2c->base_addr);  /* enable irq */
	twi_disable_ack(i2c->base_addr); /* disabe ACK */
	twi_set_efr(i2c->base_addr, 0);  /* set the special function register,default:0. */
	spin_unlock_irq(&i2c->lock);


//	for(i =0 ; i < num; i++){
//		for(j = 0; j < msgs->len; j++){
//			I2C_DBG("baddr = 0x%x \n",msgs->addr);
//			I2C_DBG("data = 0x%x \n", msgs->buf[j]);
//		}
//		I2C_DBG("\n\n");
//	}

	/* START signal, needn't clear int flag */
	ret = twi_start(i2c->base_addr, i2c->bus_num);
	if(ret == SUN7I_I2C_FAIL) {
		twi_soft_reset(i2c->base_addr);
		twi_disable_irq(i2c->base_addr);  /* disable irq */
		i2c->status  = I2C_XFER_IDLE;
		ret = SUN7I_I2C_RETRY;
		goto out;
	}

	i2c->status  = I2C_XFER_RUNNING;
	/* sleep and wait, do the transfer at interrupt handler ,timeout = 5*HZ */
	timeout = wait_event_timeout(i2c->wait, i2c->msg_num == 0, i2c->adap.timeout);
	/* return code,if(msg_idx == num) succeed */
	ret = i2c->msg_idx;

	if (timeout == 0) {
		I2C_ERR("[i2c%d] xfer timeout (dev addr:0x%x)\n", i2c->bus_num, msgs->addr);
		ret = -ETIME;
	} else if (ret != num) {
		I2C_ERR("[i2c%d] incomplete xfer (status: 0x%x, dev addr: 0x%x)\n", i2c->bus_num, ret, msgs->addr);
		ret = -ECOMM;
	}
out:
	return ret;
}

static unsigned int sun7i_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C|I2C_FUNC_10BIT_ADDR|I2C_FUNC_SMBUS_EMUL;
}


static const struct i2c_algorithm sun7i_i2c_algorithm = {
	.master_xfer	  = sun7i_i2c_xfer,
	.functionality	  = sun7i_i2c_functionality,
};

#ifndef CONFIG_AW_FPGA_PLATFORM
static int sun7i_i2c_clk_init(struct sun7i_i2c *i2c)
{
	unsigned int apb_clk = 0;

	/* enable APB clk */
	if (clk_enable(i2c->pclk)) {
		I2C_ERR("[i2c%d] enable apb_twi clock failed!\n", i2c->bus_num);
		return -1;
	}
	
	if (clk_enable(i2c->mclk)) {
		I2C_ERR("[i2c%d] enable mod_twi clock failed!\n", i2c->bus_num);
		return -1;
	}

	if (clk_reset(i2c->mclk, AW_CCU_CLK_NRESET)) {
		I2C_ERR("[i2c%d] NRESET module clock failed!\n",i2c->bus_num);
		return -1;
	}

	/* enable twi bus */
	twi_enable_bus(i2c->base_addr);

	/* set twi module clock */
	apb_clk  =  clk_get_rate(i2c->mclk);
	if (apb_clk == 0) {
		I2C_ERR("[i2c%d] get i2c source clock frequency failed!\n", i2c->bus_num);
		return -1;
	}

	twi_set_clock(apb_clk, i2c->bus_freq, i2c->base_addr);

	return 0;
}

static int sun7i_i2c_clk_exit(struct sun7i_i2c *i2c)
{
	/* disable twi bus */
	twi_disable_bus(i2c->base_addr);

	/* disable clk */
	if (!i2c->mclk || IS_ERR(i2c->mclk)) {
		I2C_ERR("[i2c%d] i2c mclk handle is invalid, just return!\n", i2c->bus_num);
		return -1;
	} else {
		if (clk_reset( i2c->mclk, AW_CCU_CLK_RESET)) {
			printk("[i2c%d] RESET module clock failed!\n", i2c->bus_num);
			return -1;
		}
		clk_disable(i2c->mclk);
	}

	if (!i2c->pclk || IS_ERR(i2c->pclk)) {
		I2C_ERR("[i2c%d] i2c pclk handle is invalid, just return!\n", i2c->bus_num);
		return -1;
	} else {
		clk_disable(i2c->pclk);
	}

	return 0;
}


static int sun7i_i2c_hw_init(struct sun7i_i2c *i2c)
{
	int ret = 0;

	ret = twi_request_gpio(i2c);
	if(ret == -1){
		I2C_ERR("[i2c%d] request i2c gpio failed!\n", i2c->bus_num);
		return -1;
	}

	if(sun7i_i2c_clk_init(i2c)) {
		I2C_ERR("[i2c%d] init i2c clock failed!\n", i2c->bus_num);
		return -1;
	}

	twi_soft_reset(i2c->base_addr);

	return ret;
}

static void sun7i_i2c_hw_exit(struct sun7i_i2c *i2c)
{
	if(sun7i_i2c_clk_exit(i2c)) {
		I2C_ERR("[i2c%d] exit i2c clock failed!\n", i2c->bus_num);
		return;
	}
	twi_release_gpio(i2c);
}
#else
static int sun7i_i2c_hw_init(struct sun7i_i2c *i2c)
{
	twi_enable_bus(i2c->base_addr);
	twi_set_clock(24000000, i2c->bus_freq, i2c->base_addr);
	twi_soft_reset(i2c->base_addr);
	return 0;
}

static void sun7i_i2c_hw_exit(struct sun7i_i2c *i2c)
{
	twi_disable_bus(i2c->base_addr);
}
#endif

#ifndef CONFIG_SUN7I_I2C_PRINT_TRANSFER_INFO
static ssize_t transfer_debug_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sun7i_i2c *i2c = platform_get_drvdata(pdev);
	char value;

    if(strlen(buf) != 2)
        return -EINVAL;
    if(buf[0] < '0' || buf[0] > '1')
		return -EINVAL;
    value = buf[0];
    switch(value)
    {
        case '1':
            bus_transfer_dbg = i2c->bus_num;
            break;
        case '0':
            bus_transfer_dbg = -1;
            break;
        default:
            return -EINVAL;
    }
	return count;
}

static struct device_attribute transfer_debug_attrs[] = {
	__ATTR(transfer_debug, 0200, NULL, transfer_debug_store),
};
#endif
static int sun7i_i2c_probe(struct platform_device *pdev)
{
	struct sun7i_i2c *i2c = NULL;
	struct resource *res = NULL;
	struct sun7i_i2c_platform_data *pdata = NULL;
#ifndef CONFIG_AW_FPGA_PLATFORM
	char *i2c_mclk[] ={CLK_MOD_TWI0, CLK_MOD_TWI1, CLK_MOD_TWI2, CLK_MOD_TWI3,CLK_MOD_TWI4};
	char *i2c_pclk[] ={CLK_APB_TWI0, CLK_APB_TWI1, CLK_APB_TWI2, CLK_APB_TWI3,CLK_APB_TWI4};
#endif
	int ret, irq;
#ifndef CONFIG_SUN7I_I2C_PRINT_TRANSFER_INFO
	int i;
#endif

	pdata = pdev->dev.platform_data;
	if(pdata == NULL) {
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if (res == NULL || irq < 0) {
		return -ENODEV;
	}

	if (!request_mem_region(res->start, resource_size(res), res->name)) {
		return -ENOMEM;
	}

	i2c = kzalloc(sizeof(struct sun7i_i2c), GFP_KERNEL);
	if (!i2c) {
		ret = -ENOMEM;
		goto emalloc;
	}

	strlcpy(i2c->adap.name, "sun7i-i2c", sizeof(i2c->adap.name));
	i2c->adap.owner   = THIS_MODULE;
	i2c->adap.nr      = pdata->bus_num;
	i2c->adap.retries = 2;
	i2c->adap.timeout = 5*HZ;
	i2c->adap.class   = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	i2c->bus_freq     = pdata->frequency;
	i2c->irq 	  = irq;
	i2c->bus_num      = pdata->bus_num;
	i2c->status       = I2C_XFER_IDLE;
	i2c->suspended    = 0;
	
	spin_lock_init(&i2c->lock);
	init_waitqueue_head(&i2c->wait);

#ifndef CONFIG_AW_FPGA_PLATFORM
	i2c->pclk = clk_get(NULL, i2c_pclk[i2c->adap.nr]);
	if (!i2c->pclk || IS_ERR(i2c->pclk)) {
		I2C_DBG("[i2c%d] request apb_i2c clock failed\n", i2c->bus_num);
		ret = -EIO;
		goto eremap;
	}

	i2c->mclk = clk_get(NULL, i2c_mclk[i2c->adap.nr]);
	if (!i2c->mclk || IS_ERR(i2c->mclk)) {
		I2C_DBG("[i2c%d] request i2c clock failed\n", i2c->bus_num);
		ret = -EIO;
		goto eremap;
	}
#endif

	snprintf(i2c->adap.name, sizeof(i2c->adap.name), "sun7i-i2c.%u", i2c->adap.nr);
	i2c->base_addr = ioremap(res->start, resource_size(res));
	if (!i2c->base_addr) {
		ret = -EIO;
		goto eremap;
	}

#ifndef SYS_I2C_PIN
	gpio_addr = ioremap(_PIO_BASE_ADDRESS, 0x400);
	if(!gpio_addr) {
	    ret = -EIO;
	    goto ereqirq;
	}

#endif

	i2c->adap.algo = &sun7i_i2c_algorithm;
	ret = request_irq(irq, sun7i_i2c_handler, IRQF_DISABLED, i2c->adap.name, i2c);
	if (ret) {
		I2C_DBG("[i2c%d] requeset irq failed!\n", i2c->bus_num);
		goto ereqirq;
	}

	if(-1 == sun7i_i2c_hw_init(i2c)){
		ret = -EIO;
		goto ehwinit;
	}

	i2c->adap.algo_data  = i2c;
	i2c->adap.dev.parent = &pdev->dev;

	i2c->iobase = res->start;
	i2c->iosize = resource_size(res);

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		I2C_ERR( "[i2c%d] failed to add adapter\n", i2c->bus_num);
		goto eadapt;
	}

	platform_set_drvdata(pdev, i2c);

#ifndef CONFIG_SUN7I_I2C_PRINT_TRANSFER_INFO
	for (i = 0; i < ARRAY_SIZE(transfer_debug_attrs); i++) {
		ret = device_create_file(&pdev->dev, &transfer_debug_attrs[i]);
		if (ret)
			goto eadapt;
	}
#endif
	I2C_DBG(KERN_INFO "I2C: %s: sun7i I2C adapter\n", dev_name(&i2c->adap.dev));

	I2C_DBG("**********start************\n");
	I2C_DBG("0x%x \n",readl(i2c->base_addr + 0x0c));
	I2C_DBG("0x%x \n",readl(i2c->base_addr + 0x10));
	I2C_DBG("0x%x \n",readl(i2c->base_addr + 0x14));
	I2C_DBG("0x%x \n",readl(i2c->base_addr + 0x18));
	I2C_DBG("0x%x \n",readl(i2c->base_addr + 0x1c));
	I2C_DBG("**********end************\n");

	return 0;

eadapt:
#ifndef CONFIG_AW_FPGA_PLATFORM
	if (i2c->mclk)
		clk_disable(i2c->mclk);
	if (i2c->pclk)
		clk_disable(i2c->pclk);
#endif

ehwinit:
	free_irq(irq, i2c);

ereqirq:
#ifndef SYS_I2C_PIN
    if(gpio_addr){
        iounmap(gpio_addr);
    }
#endif
	iounmap(i2c->base_addr);
eremap:
#ifndef CONFIG_AW_FPGA_PLATFORM
	if (i2c->mclk) {
		clk_put(i2c->mclk);
		i2c->mclk = NULL;
	}
	if (i2c->pclk) {
		clk_put(i2c->pclk);
		i2c->pclk = NULL;
	}
#endif
	kfree(i2c);

emalloc:
	release_mem_region(i2c->iobase, i2c->iosize);

	return ret;
}


static int __exit sun7i_i2c_remove(struct platform_device *pdev)
{
	struct sun7i_i2c *i2c = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&i2c->adap);
	free_irq(i2c->irq, i2c);

	/* disable clock and release gpio */
	sun7i_i2c_hw_exit(i2c);
#ifndef CONFIG_AW_FPGA_PLATFORM
	if (!i2c->mclk || IS_ERR(i2c->mclk)) {
		I2C_ERR("i2c mclk handle is invalid, just return!\n");
		return -1;
	} else {
		clk_put(i2c->mclk);
		i2c->mclk = NULL;
	}
	if (!i2c->pclk || IS_ERR(i2c->pclk)) {
		I2C_ERR("i2c pclk handle is invalid, just return!\n");
		return -1;
	} else {
		clk_put(i2c->pclk);
		i2c->pclk = NULL;
	}
#endif

	iounmap(i2c->base_addr);
	release_mem_region(i2c->iobase, i2c->iosize);
	kfree(i2c);

	return 0;
}

#ifdef CONFIG_PM
static int sun7i_i2c_suspend(struct device *dev)
{
#ifndef CONFIG_AW_FPGA_PLATFORM
	struct platform_device *pdev = to_platform_device(dev);
	struct sun7i_i2c *i2c = platform_get_drvdata(pdev);
	int count = 10;
	
	i2c->suspended = 1;
	/*
	 * twi0 is for power, it will be accessed by axp driver
	 * before twi resume, so, don't suspend twi0
	 */
	if(0 == i2c->bus_num) {
		i2c->suspended = 0;
		return 0;
	}

	while ((i2c->status != I2C_XFER_IDLE) && (count-- > 0)) {
		I2C_ERR("[i2c%d] suspend while xfer,dev addr = 0x%x\n",
			i2c->adap.nr, i2c->msg? i2c->msg->addr : 0xff);
		msleep(100);
	}

	
	if(sun7i_i2c_clk_exit(i2c)) {
		I2C_ERR("[i2c%d] suspend failed.. \n", i2c->bus_num);
		i2c->suspended = 0;
		return -1;
	}

	I2C_DBG("[i2c%d] suspend okay.. \n", i2c->bus_num);
#endif
	return 0;
}

static int sun7i_i2c_resume(struct device *dev)
{
#ifndef CONFIG_AW_FPGA_PLATFORM
	struct platform_device *pdev = to_platform_device(dev);
	struct sun7i_i2c *i2c = platform_get_drvdata(pdev);
	
	if(0 == i2c->bus_num) {
		return 0;
	}

	i2c->suspended = 0;

	if(sun7i_i2c_clk_init(i2c)) {
		I2C_ERR("[i2c%d] resume failed.. \n", i2c->bus_num);
		return -1;
	}

	twi_soft_reset(i2c->base_addr);

	I2C_DBG("[i2c%d] resume okay.. \n", i2c->bus_num);
#endif
	return 0;
}

static const struct dev_pm_ops sun7i_i2c_dev_pm_ops = {
	.suspend = sun7i_i2c_suspend,
	.resume = sun7i_i2c_resume,
};

#define SUN7I_I2C_DEV_PM_OPS (&sun7i_i2c_dev_pm_ops)
#else
#define SUN7I_I2C_DEV_PM_OPS NULL
#endif

static struct platform_driver sun7i_i2c_driver = {
	.probe		= sun7i_i2c_probe,
	.remove		= __exit_p(sun7i_i2c_remove),
	.driver		= {
		.name	= "sun7i-i2c",
		.owner	= THIS_MODULE,
		.pm		= SUN7I_I2C_DEV_PM_OPS,
	},
};


/* twi0 */
static struct resource sun7i_twi0_resources[] = {
	{
		.start	= TWI0_BASE_ADDR_START,
		.end	= TWI0_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= AW_IRQ_TWI0,
		.end	= AW_IRQ_TWI0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct sun7i_i2c_platform_data sun7i_twi0_pdata[] = {
	{
		.bus_num   = 0,
		.frequency = TWI0_TRANSFER_SPEED,
	},
};

struct platform_device sun7i_twi0_device = {
	.name		= "sun7i-i2c",
	.id		= 0,
	.resource	= sun7i_twi0_resources,
	.num_resources	= ARRAY_SIZE(sun7i_twi0_resources),
	.dev = {
		.platform_data = sun7i_twi0_pdata,
	},
};

/* twi1 */
static struct resource sun7i_twi1_resources[] = {
	{
		.start	= TWI1_BASE_ADDR_START,
		.end	= TWI1_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	}, 
	{
		.start	= AW_IRQ_TWI1,
		.end	= AW_IRQ_TWI1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct sun7i_i2c_platform_data sun7i_twi1_pdata[] = {
	{
		.bus_num   = 1,
    	        .frequency = TWI1_TRANSFER_SPEED,
	},
};

struct platform_device sun7i_twi1_device = {
	.name		= "sun7i-i2c",
	.id             = 1,
	.resource	= sun7i_twi1_resources,
	.num_resources	= ARRAY_SIZE(sun7i_twi1_resources),
	.dev = {
		.platform_data = sun7i_twi1_pdata,
	},
};

/* twi2 */
static struct resource sun7i_twi2_resources[] = {
	{
		.start	= TWI2_BASE_ADDR_START,
		.end	= TWI2_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	},
        {
		 .start	= AW_IRQ_TWI2,
		 .end	= AW_IRQ_TWI2,
		 .flags	= IORESOURCE_IRQ,
	 },
};

static struct sun7i_i2c_platform_data sun7i_twi2_pdata[] = {
	{
		.bus_num   = 2,
    	        .frequency = TWI2_TRANSFER_SPEED,
	},
};

struct platform_device sun7i_twi2_device = {
	.name		= "sun7i-i2c",
	.id             = 2,
	.resource	= sun7i_twi2_resources,
	.num_resources	= ARRAY_SIZE(sun7i_twi2_resources),
	.dev = {
		.platform_data = sun7i_twi2_pdata,
	},
};

/* twi3 */
static struct resource sun7i_twi3_resources[] = {
	{
		.start	= TWI3_BASE_ADDR_START,
		.end	= TWI3_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	},
        {
		 .start	= AW_IRQ_TWI3,
		 .end	= AW_IRQ_TWI3,
		 .flags	= IORESOURCE_IRQ,
	 },
};

static struct sun7i_i2c_platform_data sun7i_twi3_pdata[] = {
	{
		.bus_num   = 3,
    	        .frequency = TWI3_TRANSFER_SPEED,
	},
};

struct platform_device sun7i_twi3_device = {
	.name		= "sun7i-i2c",
	.id             = 3,
	.resource	= sun7i_twi3_resources,
	.num_resources	= ARRAY_SIZE(sun7i_twi3_resources),
	.dev = {
		.platform_data = sun7i_twi3_pdata,
	},
};

/* twi4 */
static struct resource sun7i_twi4_resources[] = {
	{
		.start	= TWI4_BASE_ADDR_START,
		.end	= TWI4_BASE_ADDR_END,
		.flags	= IORESOURCE_MEM,
	},
        {
		 .start	= AW_IRQ_TWI4,
		 .end	= AW_IRQ_TWI4,
		 .flags	= IORESOURCE_IRQ,
	 },
};

static struct sun7i_i2c_platform_data sun7i_twi4_pdata[] = {
	{
		.bus_num   = 4,
    	        .frequency = TWI4_TRANSFER_SPEED,
	},
};

struct platform_device sun7i_twi4_device = {
	.name		= "sun7i-i2c",
	.id             = 4,
	.resource	= sun7i_twi4_resources,
	.num_resources	= ARRAY_SIZE(sun7i_twi4_resources),
	.dev = {
		.platform_data = sun7i_twi4_pdata,
	},
};

#ifndef CONFIG_AW_FPGA_PLATFORM
#define TWI0_USED_MASK 0x1
#define TWI1_USED_MASK 0x2
#define TWI2_USED_MASK 0x4
#define TWI3_USED_MASK 0x8
#define TWI4_USED_MASK 0x10
static int twi_used_mask = 0;

static int __init sun7i_i2c_adap_init(void)
{
        script_item_u used;
        script_item_value_type_e type;
        int i = 0;
        char twi_para[16] = {0};
        char twi_used[16] = {0};

        for (i=0; i < TWI_MODULE_NUM; i++) {
                sprintf(twi_para, "twi%d_para", i);
                sprintf(twi_used, "twi%d_used", i);
		type = script_get_item(twi_para, twi_used, &used);
		        
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		        I2C_ERR("[i2c%d] fetch para from sysconfig failed\n", i);
			continue;
		}
                
                if (used.val)
                        twi_used_mask |= 1 << i;
        }

        if (twi_used_mask & TWI0_USED_MASK)
                platform_device_register(&sun7i_twi0_device);

        if (twi_used_mask & TWI1_USED_MASK)
                platform_device_register(&sun7i_twi1_device);

        if (twi_used_mask & TWI2_USED_MASK)
                platform_device_register(&sun7i_twi2_device);

        if (twi_used_mask & TWI3_USED_MASK)
                platform_device_register(&sun7i_twi3_device);
                
        if (twi_used_mask & TWI4_USED_MASK)
                platform_device_register(&sun7i_twi4_device);

        if (twi_used_mask)
                return platform_driver_register(&sun7i_i2c_driver);

        I2C_DBG("cannot find any using configuration for all twi controllers!\n");

	return 0;
}

static void __exit sun7i_i2c_adap_exit(void)
{
	if (twi_used_mask)
		platform_driver_unregister(&sun7i_i2c_driver);
}
#else
static int __init sun7i_i2c_adap_init(void)
{
	platform_device_register(&sun7i_twi0_device);
	return platform_driver_register(&sun7i_i2c_driver);
}

static void __exit sun7i_i2c_adap_exit(void)
{
	platform_driver_unregister(&sun7i_i2c_driver);
}
#endif


subsys_initcall(sun7i_i2c_adap_init);
module_exit(sun7i_i2c_adap_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sun7i-i2c");
MODULE_DESCRIPTION("SUN7I I2C Bus Driver");
MODULE_AUTHOR("pannan");

