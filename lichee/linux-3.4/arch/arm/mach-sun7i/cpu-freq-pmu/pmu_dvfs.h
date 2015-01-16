/*
 * arch/arm/mach-sun7i/cpu-freq-pmu/pmu_dvfs.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __PMU_DVFS_H__
#define __PMU_DVFS_H__

#include <linux/init.h>
#include <linux/types.h>
#include <mach/includes.h>


#if (0)
    #define DVFS_DBG(format,args...)   printk("[pmu_dvfs] DBG:"format,##args)
#else
    #define DVFS_DBG(format,args...)   do{}while(0)
#endif

#define DVFS_ERR(format,args...)   printk("[pmu_dvfs] ERR:"format,##args)

#define CCU_PLL6            (SW_VA_CCM_IO_BASE + 0x28)  /* speed factor dectector start when set bit31 to 1 */
#define CCU_CPU_AHB_APB     (SW_VA_CCM_IO_BASE + 0x54)  /* select cpu clock source */

extern int __init pmu_dvfs_hw_init(void);
extern int pmu_dvfs_volt_chg_err(void);
extern void pmu_dvfs_start_to_finish(void);
extern void pmu_dvfs_clear_pending(void);
extern int pmu_dvfs_suspend(void);
extern int pmu_dvfs_resume(void);

#endif /* __PMU_DVFS_H__ */