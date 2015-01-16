/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : pm.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-27 14:08
* Descript: power manager
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __AW_PM_H__
#define __AW_PM_H__


/**max device number of pmu*/
#define PMU_MAX_DEVS        2

/**
*@name PMU command
*@{
*/
#define AW_PMU_SET          0x10
#define AW_PMU_VALID        0x20
/**
*@}
*/

/*
* define event source for wakeup system when suspended
*/
#define SUSPEND_WAKEUP_SRC_EXINT    (1<<0)  /* external interrupt, pmu event for ex.    */
#define SUSPEND_WAKEUP_SRC_USB      (1<<1)  /* usb connection event */
#define SUSPEND_WAKEUP_SRC_KEY      (1<<2)  /* key event    */
#define SUSPEND_WAKEUP_SRC_IR       (1<<3)  /* ir event */
#define SUSPEND_WAKEUP_SRC_ALARM    (1<<4)  /* alarm event  */
#define SUSPEND_WAKEUP_SRC_TIMEOFF  (1<<5)  /* set time to power off event  */
#define SUSPEND_WAKEUP_SRC_PIO      (1<<6)  /* gpio event  */


#define WAKEUP_GPIO_PH(num)     (1 << (num))
#define WAKEUP_GPIO_PI(num)     (1 << (num + 21))
//#define WAKEUP_GPIO_AXP(num)    (1 << (num + 24))

/**
*@brief struct of pmu device arg
*/
struct aw_pmu_arg{
    unsigned int  twi_port;     /**<twi port for pmu chip   */
    unsigned char dev_addr;     /**<address of pmu device   */
};

extern int pm_wakeup_src_enable(int src);

#endif /* __AW_PM_H__ */

