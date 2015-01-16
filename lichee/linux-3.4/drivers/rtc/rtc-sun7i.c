/*
 * drivers\rtc\rtc-sun7i.c
 * (C) Copyright 2007-2011
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sunxi rtc driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/i2c.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <mach/includes.h>

#include "rtc-sun7i-regs.h"

#define A20_ALARM
#define SW_INT_IRQNO_ALARM   AW_IRQ_TIMER2

static rtc_alarm_reg *rtc_reg_list;
static int sunxi_rtc_alarmno = SW_INT_IRQNO_ALARM;

#include "rtc-sun7i-base.c"

static int sunxi_rtc_open(struct device *dev)
{
	pr_info("sunxi_rtc_open \n");
	return 0;
}

static void sunxi_rtc_release(struct device *dev)
{
}

static const struct rtc_class_ops sunxi_rtcops = {
	.open			= sunxi_rtc_open,
	.release		= sunxi_rtc_release,
	.read_time		= sunxi_rtc_gettime,
	.set_time		= sunxi_rtc_settime,
#ifdef A20_ALARM
	.read_alarm		= sunxi_rtc_getalarm,
	.set_alarm		= sunxi_rtc_setalarm,
	.alarm_irq_enable 	= sunxi_rtc_alarm_irq_enable,
#endif
};

static int __devexit sunxi_rtc_remove(struct platform_device *pdev)
{
	struct rtc_device *rtc = platform_get_drvdata(pdev);

#ifdef A20_ALARM
	free_irq(sunxi_rtc_alarmno, rtc);
#endif
	rtc_device_unregister(rtc);
	platform_set_drvdata(pdev, NULL);
#ifdef A20_ALARM
	sunxi_rtc_setaie(0);
#endif
	return 0;
}

static int __devinit sunxi_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	int ret;

	rtc_reg_list = (rtc_alarm_reg *)(SW_VA_TIMERC_IO_BASE + LOSC_CTRL_REG_OFF);

	/* init rtc hardware */
	ret = rtc_hw_init();
	if(ret) {
		pr_err("%s(%d) err: rtc_probe_init failed\n", __func__, __LINE__);
		return ret;
	}

	device_init_wakeup(&pdev->dev, 1);

	/* register rtc device */
	rtc = rtc_device_register("rtc", &pdev->dev, &sunxi_rtcops, THIS_MODULE);
	if(IS_ERR(rtc)) {
		dev_err(&pdev->dev, "err: cannot attach rtc\n");
		ret = PTR_ERR(rtc);
		return ret;
	}

#ifdef A20_ALARM
	/* register alarm irq */
	ret = request_irq(sunxi_rtc_alarmno, sunxi_rtc_alarmirq,
			IRQF_DISABLED,  "sunxi-rtc alarm", rtc);
	if(ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", sunxi_rtc_alarmno, ret);
		rtc_device_unregister(rtc);
		return ret;
	}
#endif
	platform_set_drvdata(pdev, rtc);
	return 0;
}

static int sunxi_rtc_suspend(struct platform_device *dev, pm_message_t state)
{
    rtc_reg_list->a_config.alarm_wakeup = 1;
	return 0;
}

static int sunxi_rtc_resume(struct platform_device *dev)
{
    rtc_reg_list->a_config.alarm_wakeup = 0;
	return 0;
}

/* share the irq no. with timer2 */
static struct resource sunxi_rtc_resource[] = {
	[0] = {
		.start = SW_INT_IRQNO_ALARM,
		.end   = SW_INT_IRQNO_ALARM,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device sunxi_device_rtc = {
	.name		= "sunxi-rtc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(sunxi_rtc_resource),
	.resource	= sunxi_rtc_resource,
};
static struct platform_driver sunxi_rtc_driver = {
	.probe		= sunxi_rtc_probe,
	.remove		= __devexit_p(sunxi_rtc_remove),
	.driver		= {
		.name	= "sunxi-rtc",
		.owner	= THIS_MODULE,
	},
	.suspend        =  sunxi_rtc_suspend,
	.resume         =  sunxi_rtc_resume,
};

static int i2c_write_reg(struct i2c_adapter *adapter, u8 adr,
			 u8 reg, u8 data)
{
	u8 m[2] = {reg, data};
	struct i2c_msg msg = {.addr = adr, .flags = 0, .buf = m, .len = 2};

	if (i2c_transfer(adapter, &msg, 1) != 1) {
		printk(KERN_ERR "Failed to write to I2C register %02x@%02x!\n",
		       reg, adr);
		return -1;
	}
	return 0;
}

static int i2c_read_reg(struct i2c_adapter *adapter, u8 adr,
			u8 reg, u8 *val)
{
	struct i2c_msg msgs[2] = {{.addr = adr, .flags = 0,
				   .buf = &reg, .len = 1},
				  {.addr = adr, .flags = I2C_M_RD,
				   .buf = val, .len = 1} };

	if (i2c_transfer(adapter, msgs, 2) != 2) {
		printk(KERN_ERR "error in i2c_read_reg\n");
		return -1;
	}
	return 0;
}


script_item_u rtc_used;

static int __init sunxi_rtc_init(void)
{
	struct i2c_adapter *adap = NULL;
	u8 addr = 0x34;
	u8 val = 0;
	u8 data = (1<<7)|(1<<5)|2;
	script_item_value_type_e type;

	pr_info("rtc init\n");

	rtc_used.val = 0;
	type = script_get_item("pmu_para", "pmu_backupen", &rtc_used);
	if (SCIRPT_ITEM_VALUE_TYPE_INT == type) {
		if (rtc_used.val == 1) {
			adap = i2c_get_adapter(0);
			if (adap != NULL) {
				i2c_read_reg(adap, addr, 0x35, &val);
				pr_debug("var: %x\n", val);
				pr_info("rtc: enable pmu backup\n");
				i2c_write_reg(adap, addr, 0x35, data);
				i2c_read_reg(adap, addr, 0x35, &val);
				pr_debug("var: %x\n", val);

				pr_debug("release i2c adapter0\n");
				i2c_put_adapter(adap);
			}
		}
	}

	platform_device_register(&sunxi_device_rtc);
	return platform_driver_register(&sunxi_rtc_driver);
}

static void __exit sunxi_rtc_exit(void)
{
	platform_driver_unregister(&sunxi_rtc_driver);
}

module_init(sunxi_rtc_init);
module_exit(sunxi_rtc_exit);

MODULE_DESCRIPTION("sunxi rtc Driver");
MODULE_AUTHOR("liugang");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-rtc");
