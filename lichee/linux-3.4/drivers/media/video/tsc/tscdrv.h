/*
 * drivers/media/video/tsc/tscdrv.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * csjamesdeng <csjamesdeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __TSC_DRV_H__
#define __TSC_DRV_H__

#include "dvb_drv_sun7i.h"

#define DRV_VERSION                 "0.01alpha"      //version 
#define TS_IRQ_NO                   (AW_IRQ_TS)             //interrupt number, 

#ifndef TSCDEV_MAJOR
#define TSCDEV_MAJOR                (225)
#endif

#ifndef TSCDEV_MINOR
#define TSCDEV_MINOR                (0)
#endif

struct tsc_dev {
    struct cdev cdev;               /* char device struct */
    struct device *dev;             /* ptr to class device struct */
    struct class *class;            /* class for auto create device node */
    struct semaphore sem;           /* mutual exclusion semaphore */
    spinlock_t lock;                /* spinlock to protect ioclt access */
    wait_queue_head_t  wq;          /* wait queue for poll ops */

    struct resource *regs;          /* registers resource */
    char *regsaddr ;                /* registers address */

    unsigned int irq;               /* tsc driver irq number */
    unsigned int irq_flag;          /* flag of tsc driver irq generated */

    int     ts_dev_major;
    int     ts_dev_minor;

    struct clk *pll5_clk;
    struct clk *sdram_clk;          /* sdram clock */
    struct clk *tsc_clk;            /* ts  clock */
    struct clk *ahb_clk;            /* ahb clock */

    char *pmem;                     /* memory address */
    unsigned int gsize;             /* memory size */

    struct intrstatus intstatus;    /* save interrupt status */
};

#define TSC_DEBUG_LEVEL 3

#if (TSC_DEBUG_LEVEL == 1)
    #define TSC_DBG(format,args...)     do {} while (0)
    #define TSC_INF(format,args...)     do {} while (0)
    #define TSC_ERR(format,args...)     printk(KERN_ERR "[tsc-err] "format,##args)
#elif (TSC_DEBUG_LEVEL == 2)
    #define TSC_DBG(format,args...)     do {} while (0)
    #define TSC_INF(format,args...)     printk(KERN_INFO"[tsc-inf] "format,##args)
    #define TSC_ERR(format,args...)     printk(KERN_ERR "[tsc-err] "format,##args)
#elif (TSC_DEBUG_LEVEL == 3)
    #define TSC_DBG(format,args...)     printk(KERN_INFO"[tsc-dbg] "format,##args)
    #define TSC_INF(format,args...)     printk(KERN_INFO"[tsc-inf] "format,##args)
    #define TSC_ERR(format,args...)     printk(KERN_ERR "[tsc-err] "format,##args)
#endif

#endif
