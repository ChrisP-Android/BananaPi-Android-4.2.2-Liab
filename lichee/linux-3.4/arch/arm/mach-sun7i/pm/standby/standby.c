/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        newbie Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 18:34
* Descript: platform standby fucntion.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "standby_i.h"


extern unsigned int save_sp(void);
extern void restore_sp(unsigned int sp);
extern void mem_flush_tlb(void);
extern void mem_preload_tlb(void);
extern char *__bss_start;
extern char *__bss_end;
extern char *__standby_start;
extern char *__standby_end;

static __u32 sp_backup;
static __u32 dram_self_refresh = 1;
static __u32 cpu_clk_src_pll = 0;
static void standby(void);
static __u32 dcdc2, dcdc3, dcdc4;
static struct pll_factor_t orig_pll;

static struct sun4i_clk_div_t  clk_div;
static struct sun4i_clk_div_t  tmp_clk_div;

/* parameter for standby, it will be transfered from sys_pwm module */
struct aw_pm_info  pm_info;

#define DRAM_BASE_ADDR      0xc0000000
static __u8 dram_traning_area_back[DRAM_TRANING_SIZE];
struct standby_ir_buffer ir_buffer; //add by fe3o4

/*
*********************************************************************************************************
*                                   STANDBY MAIN PROCESS ENTRY
*
* Description: standby main process entry.
*
* Arguments  : arg  pointer to the parameter that transfered from sys_pwm module.
*
* Returns    : none
*
* Note       :
*********************************************************************************************************
*/
int main(struct aw_pm_info *arg)
{
    char    *tmpPtr;

    tmpPtr = (char *)&__bss_start;
    dram_suspend_flag = 0;
    printk("normal standby start!\n");

//    printk("__bss_start:%x!\n",&__bss_start);
    if(!arg){
        /* standby parameter is invalid */
        return -1;
    }
    dram_self_refresh = 1;
    cpu_clk_src_pll = 0;

    /* flush data and instruction tlb, there is 32 items of data tlb and 32 items of instruction tlb,
       The TLB is normally allocated on a rotating basis. The oldest entry is always the next allocated */
    mem_flush_tlb();
    /* preload tlb for standby */
    mem_preload_tlb();

    /* clear bss segment */
    do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);

    /* copy standby parameter from dram */
    standby_memcpy(&pm_info, arg, sizeof(pm_info));
    pm_info.standby_para.event = 0;
    /* copy standby code & data to load tlb */
    //standby_memcpy((char *)&__standby_end, (char *)&__standby_start, (char *)&__bss_end - (char *)&__bss_start);
    /* backup dram traning area */
    standby_memcpy((char *)dram_traning_area_back, (char *)DRAM_BASE_ADDR, DRAM_TRANING_SIZE);

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    /* init module before dram enter selfrefresh */
    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    /* initialise standby modules */
    standby_clk_init();
    standby_clk_apbinit();
    mem_int_init();
    standby_tmr_init();
    standby_power_init(pm_info.standby_para.axp_src);
    /* init some system wake source */
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_EXINT){
        mem_enable_int(INT_SOURCE_EXTNMI);
    }
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_KEY){
        standby_key_init();
        mem_enable_int(INT_SOURCE_LRADC);
    }
	printk("fe3o4 test print here pm_info.standby_para.event_enable:%x\n", pm_info.standby_para.event_enable);
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_IR){
        standby_ir_init();
        mem_enable_int(INT_SOURCE_IR0);
        mem_enable_int(INT_SOURCE_IR1);
    }
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_ALARM){
        //standby_alarm_init();???
        mem_enable_int(INT_SOURCE_ALARM);
    }
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_USB){
        standby_usb_init();
        mem_enable_int(INT_SOURCE_USB0);
        mem_enable_int(INT_SOURCE_USB1);
        mem_enable_int(INT_SOURCE_USB2);
        mem_enable_int(INT_SOURCE_USB3);
        mem_enable_int(INT_SOURCE_USB4);
        dram_self_refresh = 0;
        cpu_clk_src_pll = 1;
    }
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_TIMEOFF){
        /* set timer for power off */
        if(pm_info.standby_para.time_off) {
            standby_tmr_set(pm_info.standby_para.time_off);
            mem_enable_int(INT_SOURCE_TIMER0);
        }
    }
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_PIO){
        mem_enable_int(INT_SOURCE_GPIO);
    }

    /* save stack pointer registger, switch stack to sram */
    sp_backup = save_sp();
    /* enable dram enter into self-refresh */
    if (dram_self_refresh != 0)
    {
        dram_suspend_flag = 1;
        dram_power_save_process(0);
    }
	//mctl_self_refresh_entry();
    
    /* process standby */
    standby();

    if (dram_self_refresh != 0)
    {
        /* enable watch-dog to preserve dram training failed */
        standby_tmr_enable_watchdog();
        /* restore dram */
        //dram_power_up_process();
        //mctl_self_refresh_exit();
        init_DRAM(&pm_info.dram_para);
        
        /* disable watch-dog    */
        standby_tmr_disable_watchdog();
    }
    dram_suspend_flag = 0;

    /* check system wakeup event */
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_EXTNMI)? 0:SUSPEND_WAKEUP_SRC_EXINT;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_USB0)? 0:SUSPEND_WAKEUP_SRC_USB;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_LRADC)? 0:SUSPEND_WAKEUP_SRC_KEY;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_IR0)? 0:SUSPEND_WAKEUP_SRC_IR;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_ALARM)? 0:SUSPEND_WAKEUP_SRC_ALARM;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_TIMER0)? 0:SUSPEND_WAKEUP_SRC_TIMEOFF;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_USB0)? 0:SUSPEND_WAKEUP_SRC_USB;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_USB1)? 0:SUSPEND_WAKEUP_SRC_USB;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_USB2)? 0:SUSPEND_WAKEUP_SRC_USB;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_USB3)? 0:SUSPEND_WAKEUP_SRC_USB;
    pm_info.standby_para.event |= mem_query_int(INT_SOURCE_USB4)? 0:SUSPEND_WAKEUP_SRC_USB;
    printk("%s:%d, wakeup src:%x!\n", __FILE__, __LINE__, pm_info.standby_para.event);

    /* restore stack pointer register, switch stack back to dram */
    restore_sp(sp_backup);

    /* exit standby module */
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_USB){
        standby_usb_exit();
    }
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_IR){
        standby_ir_exit();
    }
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_ALARM){
        //standby_alarm_exit();
    }
    if(pm_info.standby_para.event_enable & SUSPEND_WAKEUP_SRC_KEY){
        standby_key_exit();
    }
    standby_power_exit(pm_info.standby_para.event_enable);
    standby_tmr_exit();
    mem_int_exit();
    standby_clk_apbexit();
    standby_clk_exit();

    /* restore dram traning area */
    standby_memcpy((char *)DRAM_BASE_ADDR, (char *)dram_traning_area_back, DRAM_TRANING_SIZE);

    /* report which wake source wakeup system */
    arg->standby_para.event = pm_info.standby_para.event;

    printk("normal standby end!\n");
    return 0;
}


/*
*********************************************************************************************************
*                                     SYSTEM PWM ENTER STANDBY MODE
*
* Description: enter standby mode.
*
* Arguments  : none
*
* Returns    : none;
*********************************************************************************************************
*/
static void standby(void)
{
    struct pll_factor_t local_pll;
    int pll_mask = 0xffffffff;
    /* gating off dram clock */
    if (dram_self_refresh != 0)
    {
        standby_clk_dramgating(0);
    }
    if (dram_self_refresh == 0)
    {
        pll_mask &= ~(1<<5);     /* keep pll5(dram pll) on*/
    }
    if (cpu_clk_src_pll != 0)
    {
        pll_mask &= ~(1<<1);      /* keep pll1 on*/
    }

	/* backup cpu freq */
	standby_clk_get_pll_factor(&orig_pll);

	/*lower freq from 1008M to 384M*/
	local_pll.FactorN = 16;
	local_pll.FactorK = 0;
	local_pll.FactorM = 0;
	local_pll.FactorP = 0;
	standby_clk_set_pll_factor(&local_pll);
	change_runtime_env(1);
	delay_ms(10);
	
   if (cpu_clk_src_pll == 0)
    {
        /* switch cpu clock to HOSC, and disable pll */
        standby_clk_core2hosc();
        change_runtime_env(1);
        delay_us(1);
    }
    else
    {
        /*lower freq from 384M to 36M*/
        local_pll.FactorN = 12;
        local_pll.FactorK = 0;
        local_pll.FactorM = 0;
        local_pll.FactorP = 3;
        standby_clk_set_pll_factor(&local_pll);
        change_runtime_env(1);
        delay_ms(10);
    }
    /* set clock division cpu:axi:ahb:apb = 2:2:2:1 */
    standby_clk_ahb_2pll();
    standby_clk_getdiv(&clk_div);
    tmp_clk_div.axi_div = 0;
    tmp_clk_div.ahb_div = 0;
    tmp_clk_div.apb_div = 0;
    standby_clk_setdiv(&tmp_clk_div);
 
	//delay_ms(1);
    standby_clk_plldisable(pll_mask);
    

    if (pm_info.standby_para.axp_enable)
    {
        /* backup voltages */
        dcdc2 = standby_get_voltage(POWER_VOL_DCDC2);
		
		#if defined (CONFIG_AW_AXP20)
        dcdc3 = standby_get_voltage(POWER_VOL_DCDC3);
		#endif
		
		#if defined (CONFIG_AW_AXP15)
		dcdc4 = standby_get_voltage(POWER_VOL_DCDC4);
        #endif
		
        /* adjust voltage */
       if (dram_self_refresh != 0)
        {
        	#if defined (CONFIG_AW_AXP20)
        	standby_set_voltage(POWER_VOL_DCDC3, STANDBY_DCDC3_VOL);
			#endif
		
			#if defined (CONFIG_AW_AXP15)
			standby_set_voltage(POWER_VOL_DCDC4, STANDBY_DCDC4_VOL);
        	#endif
        }
        if (cpu_clk_src_pll == 0)
        {
            standby_set_voltage(POWER_VOL_DCDC2, STANDBY_DCDC2_VOL);
        } else {
            standby_set_voltage(POWER_VOL_DCDC2, DVFS_DCDC2_VOL);
        }
		
		#if defined (CONFIG_AW_AXP20)
        printk("adjust dcdc2:%d, dcdc3:%d!\n", standby_get_voltage(POWER_VOL_DCDC2), standby_get_voltage(POWER_VOL_DCDC3));
		#endif
		
		#if defined (CONFIG_AW_AXP15)
		printk("adjust dcdc2:%d, dcdc4:%d!\n", standby_get_voltage(POWER_VOL_DCDC2), standby_get_voltage(POWER_VOL_DCDC4));
        #endif
        
    }


	
    if (cpu_clk_src_pll == 0)
    {
        /* switch cpu to 32k */
        standby_clk_apb2losc();
    	change_runtime_env(1);
        standby_clk_core2losc();
    }
    
    if (pll_mask == 0xffffffff)
    {
        // disable HOSC, and disable LDO
        standby_clk_hoscdisable();
        standby_clk_ldodisable();
    }
    /* cpu enter sleep, wait wakeup by interrupt */
    asm("WFI");
    if (pll_mask == 0xffffffff)
    {
        /* enable LDO, enable HOSC */
        standby_clk_ldoenable();
        /* delay 1ms for power be stable */
        //3ms
        standby_delay_cycle(1);
        standby_clk_hoscenable();
        //3ms
        standby_delay_cycle(1);
    }
    if (cpu_clk_src_pll == 0)
    {
        /* switch clock to hosc */
        standby_clk_core2hosc();
        standby_clk_apb2hosc();
    }

    if (pm_info.standby_para.axp_enable)
    {
        /* restore voltage for exit standby */
        standby_set_voltage(POWER_VOL_DCDC2, dcdc2);
		
        #if defined (CONFIG_AW_AXP15)
		standby_set_voltage(POWER_VOL_DCDC4, dcdc4);
		#endif
		
		#if defined (CONFIG_AW_AXP20)
        standby_set_voltage(POWER_VOL_DCDC3, dcdc3);
        #endif
    }

    /* enable pll */
    standby_clk_pllenable();
	change_runtime_env(1);
	delay_ms(10);
    
    /* restore clock division */
    standby_clk_setdiv(&clk_div);

    standby_clk_ahb_restore();
    
    if (cpu_clk_src_pll == 0)
    {
        /* switch cpu clock to core pll */
        standby_clk_core2pll();
        change_runtime_env(1);
        delay_ms(10);
    }
    else
    {
        /*lower freq from 36M to 384M*/
        local_pll.FactorN = 16;
        local_pll.FactorK = 0;
        local_pll.FactorM = 0;
        local_pll.FactorP = 0;
        standby_clk_set_pll_factor(&local_pll);
        change_runtime_env(1);
        delay_ms(10);
    }

	/*restore freq from 384 to 1008M*/
	standby_clk_set_pll_factor(&orig_pll);
	change_runtime_env(1);
	delay_ms(5);
	
    /* gating on dram clock */
    if (dram_self_refresh != 0)
    {
        standby_clk_dramgating(1);
    }

    return;
}

