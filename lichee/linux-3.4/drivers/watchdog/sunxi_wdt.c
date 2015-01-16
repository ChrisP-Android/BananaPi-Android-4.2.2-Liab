/*
 * drivers/watchdog/sunxi_wdt.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * abai <abai@reuuimllatech.com>
 *
 * sun7i watchdog driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/bug.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <mach/includes.h>
#include <linux/pm.h>

#define DRV_NAME	"sunxi_wdt"
#define DRV_VERSION	"1.0"
#define PFX		DRV_NAME ": "
#define MAX_TIMEOUT 	16 /* max 16 seconds */

static struct platform_device *platform_device;
static bool is_active, expect_release;

struct sunxi_watchdog_interv{
	u32 	timeout;
	u32 	interv;
};

static const struct sunxi_watchdog_interv g_timeout_interv[] = {
	{.timeout = 0.5, .interv = 0b0000},
	{.timeout = 1  , .interv = 0b0001},
	{.timeout = 2  , .interv = 0b0010},
	{.timeout = 3  , .interv = 0b0011},
	{.timeout = 4  , .interv = 0b0100},
	{.timeout = 5  , .interv = 0b0101},
	{.timeout = 6  , .interv = 0b0110},
	{.timeout = 8  , .interv = 0b0111},
	{.timeout = 10 , .interv = 0b1000},
	{.timeout = 12 , .interv = 0b1001},
	{.timeout = 14 , .interv = 0b1010},
	{.timeout = 16 , .interv = 0b1011},
};

/* watchdog timeout in second */
static unsigned int g_timeout = MAX_TIMEOUT;
module_param(g_timeout, uint, S_IRUGO);
MODULE_PARM_DESC(g_timeout, "watchdog g_timeout in seconds "
	"(default=" __MODULE_STRING(MAX_TIMEOUT) ")");

/* watchdog noway out flag */
static bool g_nowayout = WATCHDOG_NOWAYOUT;
module_param(g_nowayout, bool, S_IRUGO);
MODULE_PARM_DESC(g_nowayout, "watchdog cannot be stopped once started "
	"(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

/* reg define */
static struct resource sunxi_wdt_res[] = {
	{
		.start	= SW_PA_TIMERC_IO_BASE + WDOG_CTRL_REG_OFF,
		.end	= SW_PA_TIMERC_IO_BASE + WDOG_CTRL_REG_OFF + 0x10,
		.flags	= IORESOURCE_MEM,
	},
};

static void wdt_irq_en(bool enable)
{
	u32 val = readl(TMR_IRQ_EN_REG);

	pr_info("%s, enable %d\n", __func__, enable);
	/* bit8: watcdog irq enable */
	val &= ~(1<<8);
	val |= (enable ? (1<<8) : (0<<8));
	writel(val, TMR_IRQ_EN_REG);
}

static void wdt_clr_irq_pend(void)
{
	u32 val = readl(TMR_IRQ_STA_REG);

	pr_info("%s\n", __func__);
	/* bit8: watcdog irq pending */
	if(val & (1<<8))
		writel(1<<8, TMR_IRQ_STA_REG);
}

static int timeout_to_interv(int timeout_in_sec)
{
	int 	temp;
	int 	array_sz;

	array_sz = ARRAY_SIZE(g_timeout_interv);
	if(timeout_in_sec > g_timeout_interv[array_sz - 1].timeout)
		return g_timeout_interv[array_sz - 1].interv;
	else if(timeout_in_sec < g_timeout_interv[0].timeout)
		return g_timeout_interv[0].interv;
	else {
		for(temp = 0; temp < array_sz; temp++) {
			if(timeout_in_sec > g_timeout_interv[temp].timeout)
				continue;
			else
				return g_timeout_interv[temp].interv;
		}
		pr_info("%s, line %d\n", __func__, __LINE__);
		return g_timeout_interv[array_sz - 1].interv; /* the largest one */
	}
}

static int interv_to_timeout(int interv)
{
	int 	temp;
	int 	array_sz;

	array_sz = ARRAY_SIZE(g_timeout_interv);
	if(interv > g_timeout_interv[array_sz - 1].interv)
		return g_timeout_interv[array_sz - 1].timeout;
	else if(array_sz < g_timeout_interv[0].interv)
		return g_timeout_interv[0].timeout;
	else {
		for(temp = 0; temp < array_sz; temp++) {
			if(interv > g_timeout_interv[temp].interv)
				continue;
			else
				return g_timeout_interv[temp].timeout;
		}
		pr_info("%s, line %d\n", __func__, __LINE__);
		return g_timeout_interv[array_sz - 1].timeout; /* the largest one */
	}
}

static void wdt_restart(void)
{
	pr_info("%s\n", __func__);
	/* bit0: watcdog restart */
	writel(1, WDOG_CTRL_REG);
}

/**
 * wdt_set_tmout - set watchdog time out value
 * @timeout_in_sec:	time out to set in second
 *
 * Returns <0 if failed, otherwise return the actually timeout set in seconds.
 */
static int wdt_set_tmout(int timeout_in_sec)
{
	int 	temp = 0, temp2 = 0;
	int 	interv = 0;

	interv = timeout_to_interv(timeout_in_sec);
	pr_info("%s, timeout_in_sec %d, timeout_to_interv return interv 0x%08x\n",
		__func__, timeout_in_sec, interv);
	if(interv < 0)
		return interv;
	temp = readl(WDOG_MODE_REG);
	temp &= ~(0b1111 << 3);
	temp |= (interv << 3);
	writel(temp, WDOG_MODE_REG);

	temp2 = interv_to_timeout(interv);
	pr_info("%s, write 0x%08x to modreg 0x%08x, interv_to_timeout return timeout %d\n",
		__func__, temp, (u32)WDOG_MODE_REG, temp2);
	return temp2;
}

static void wdt_enable(bool enable)
{
	int 	temp = readl(WDOG_MODE_REG);

	pr_info("%s %d\n", __func__, enable);
	/* bit0: watcdog enable */
	temp &= ~(1<<0);
	if(enable)
		temp |= 1<<0;
	writel(temp, WDOG_MODE_REG);
}

static void wdt_reset_enable(bool enable)
{
	int 	temp = readl(WDOG_MODE_REG);

	pr_info("%s %d\n", __func__, enable);
	/* bit1: watcdog reset enable */
	temp &= ~(1<<1);
	if(enable)
		temp |= 1<<1;
	writel(temp, WDOG_MODE_REG);
}

static void watchdog_start(void)
{
	int temp = wdt_set_tmout(g_timeout);

	if(temp < 0)
		return;
	g_timeout = temp;
	wdt_reset_enable(true); /* enable reset default */
	wdt_enable(true);
}

static void watchdog_stop(void)
{
	wdt_enable(false);
}

static void watchdog_kick(void)
{
	wdt_restart();
}

/**
 * watchdog_set_timeout - set watchdog time out value
 * @timeout:	time out to set in second
 *
 * Returns <0 if failed, 0 if success
 */
static int watchdog_set_timeout(int timeout)
{
	int temp = wdt_set_tmout(timeout);

	if(temp < 0)
		return temp;
	g_timeout = temp;
	return 0;
}

static int watchdog_probe_init(void)
{
	int 	temp = -1;

	/* disable watchdog */
	wdt_enable(false);
	/* disable irq */
	wdt_irq_en(false);
	/* clear irq pending */
	wdt_clr_irq_pend();
	/* set max timeout */
	temp = wdt_set_tmout(g_timeout);
	if(temp < 0)
		return temp;
	g_timeout = temp;
	return 0;
}

static int sunxi_wdt_open(struct inode *inode, struct file *file)
{
	/* /dev/watchdog can only be opened once */
	if(xchg(&is_active, true))
		return -EBUSY;

	watchdog_start();
	return nonseekable_open(inode, file);
}

static int sunxi_wdt_release(struct inode *inode, struct file *file)
{
	if(expect_release)
		watchdog_stop();
	else {
		pr_info("%s: unexpected close, not stopping watchdog!\n", __func__);
		watchdog_kick(); /* why here? abai */
	}

	is_active = false;
	expect_release = false;
	return 0;
}

static ssize_t sunxi_wdt_write(struct file *file, const char __user *data,
			size_t len, loff_t *ppos)
{
	/* See if we got the magic character 'V' and reload the timer */
	if(len) {
		if(!g_nowayout) {
			size_t i;

			/* in case it was set long ago */
			expect_release = false;

			/* scan to see whether or not we got the magic character */
			for(i = 0; i != len; i++) {
				char c;
				if(get_user(c, data + i))
					return -EFAULT;
				if(c == 'V')
					expect_release = true;
			}
		}

		/* someone wrote to us, we should reload the timer */
		watchdog_kick();
	}
	return len;
}

static long sunxi_wdt_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	int new_options;
	int new_timeout;
	int __user *argp = (void __user *)arg;
	static const struct watchdog_info ident = {
		.options 		= WDIOF_SETTIMEOUT 
					| WDIOF_MAGICCLOSE
					| WDIOF_KEEPALIVEPING,
		.firmware_version 	= 0,
		.identity 		= DRV_NAME,
	};

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		return copy_to_user(argp, &ident, sizeof(ident)) ? -EFAULT : 0;

	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		return put_user(0, argp);

	case WDIOC_SETOPTIONS:
		if(get_user(new_options, argp))
			return -EFAULT;
		if(new_options & WDIOS_DISABLECARD)
			watchdog_stop();
		if(new_options & WDIOS_ENABLECARD)
			watchdog_start();
		return 0;

	case WDIOC_KEEPALIVE:
		//watchdog_set_timeout(g_timeout); /* abai,2013-1-25 */
		watchdog_kick();
		return 0;

	case WDIOC_SETTIMEOUT:
		if(get_user(new_timeout, argp))
			return -EFAULT;
		if(!new_timeout || new_timeout > MAX_TIMEOUT) {
			printk("%s err, line %d\n", __func__, __LINE__);
			return -EINVAL;
		}
		watchdog_set_timeout(new_timeout);
		/* fall through */
	case WDIOC_GETTIMEOUT:
		return put_user(g_timeout, argp);
	}

	return -ENOTTY;
}

static const struct file_operations sunxi_wdt_fops = {
	.owner =		THIS_MODULE,
	.llseek =		no_llseek,
	.write =		sunxi_wdt_write,
	.unlocked_ioctl =	sunxi_wdt_ioctl,
	.open =			sunxi_wdt_open,
	.release =		sunxi_wdt_release,
};

static struct miscdevice sunxi_wdt_miscdev = {
	.minor =	WATCHDOG_MINOR,
	.name =		"watchdog",
	.fops =		&sunxi_wdt_fops,
};

static int __devinit sunxi_wdt_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	if(!g_timeout || g_timeout > MAX_TIMEOUT) {
		g_timeout = MAX_TIMEOUT;
		pr_info(PFX "timeout value invalid, using %d\n", g_timeout);
	}

	/*
	 * As this driver only covers the global watchdog case, reject
	 * any attempts to register per-CPU watchdogs.
	 */
	if(pdev->id != -1)
		return -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(unlikely(!res)) {
		ret = -EINVAL;
		goto err_get_resource;
	}

/*	if(!devm_request_mem_region(&pdev->dev, res->start,
				resource_size(res), DRV_NAME)) {
		ret = -EBUSY;
		goto err_request_mem_region;
	}
	wdt_reg = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if(!wdt_reg) {
		ret = -ENXIO;
		goto err_ioremap;
	}
	pr_info("%s: devm_ioremap return wdt_reg 0x%08x, res->start 0x%08x, res->end 0x%08x",
		__func__, (u32)wdt_reg, (u32)res->start, (u32)res->end);
*/
	ret = misc_register(&sunxi_wdt_miscdev);
	if(ret) {
		pr_err(PFX "cannot register miscdev on minor=%d (%d)\n", WATCHDOG_MINOR, ret);
		goto err_misc_register;
	}
	pr_info(PFX "initialized (g_timeout=%ds, g_nowayout=%d)\n", g_timeout, g_nowayout);

	watchdog_probe_init();
	return ret;

err_misc_register:
//	devm_iounmap(&pdev->dev, wdt_reg);
//err_ioremap:
//	devm_release_mem_region(&pdev->dev, res->start, resource_size(res));
err_get_resource:
	return ret;
}

static int __devexit sunxi_wdt_remove(struct platform_device *pdev)
{
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	/* Stop the timer before we leave */
	watchdog_stop();

	misc_deregister(&sunxi_wdt_miscdev);
	//devm_iounmap(&pdev->dev, wdt_reg);
	devm_release_mem_region(&pdev->dev, res->start, resource_size(res));
	return 0;
}

static void sunxi_wdt_shutdown(struct platform_device *pdev)
{
	watchdog_stop();
}

static int sunxi_wdt_suspend(struct platform_device *dev, pm_message_t state)
{
	pr_info("%s(%d): enter %s\n", __func__, __LINE__, (NORMAL_STANDBY == standby_type) ?
		"normal standby" : "super standby");
	if(is_active)
		watchdog_stop();
	return 0;
}

static int sunxi_wdt_resume(struct platform_device *dev)
{
	pr_info("%s(%d): resume from %s\n", __func__, __LINE__, (NORMAL_STANDBY == standby_type) ?
		"normal standby" : "super standby");
	if(is_active)
		watchdog_start();
	return 0;
}

static struct platform_driver sunxi_wdt_driver = {
	.probe          = sunxi_wdt_probe,
	.remove         = __devexit_p(sunxi_wdt_remove),
	.shutdown       = sunxi_wdt_shutdown,
	.suspend        = sunxi_wdt_suspend,
	.resume         = sunxi_wdt_resume,
	.driver         = {
		.owner  = THIS_MODULE,
		.name   = DRV_NAME,
	},
};

static int __init sunxi_wdt_init_module(void)
{
	int err;

	pr_info(PFX "sunxi watchdog timer driver v%s\n", DRV_VERSION);
	err = platform_driver_register(&sunxi_wdt_driver);
	if(err)
		goto err_driver_register;

	platform_device = platform_device_register_simple(DRV_NAME, -1, sunxi_wdt_res, ARRAY_SIZE(sunxi_wdt_res));
	if(IS_ERR(platform_device)) {
		err = PTR_ERR(platform_device);
		goto err_platform_device;
	}
	return err;

err_platform_device:
	platform_driver_unregister(&sunxi_wdt_driver);
err_driver_register:
	return err;
}

static void __exit sunxi_wdt_cleanup_module(void)
{
	platform_device_unregister(platform_device);
	platform_driver_unregister(&sunxi_wdt_driver);
	pr_info(PFX "module unloaded\n");
}

module_init(sunxi_wdt_init_module);
module_exit(sunxi_wdt_cleanup_module);

MODULE_AUTHOR("abai <abai@reuuimllatech.com>");
MODULE_DESCRIPTION("sunxi wtchdog timer driver");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
