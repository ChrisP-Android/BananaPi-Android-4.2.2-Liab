/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include <linux/err.h>
#include <linux/module.h>  
#include <linux/clk.h>
#include <mach/irqs.h>
#include <mach/clock.h>
#include <mach/sys_config.h>
#include <mach/includes.h>
#include <mach/powernow.h>

#define CONFIG_GPU_DVFS
//#define CONFIG_DYNAMIC_GPU_FREQ

static int mali_clk_div    = 1;
static int mali_clk_flag   = 0;
struct clk *h_ahb_mali  = NULL;
struct clk *h_mali_clk  = NULL;
struct clk *h_gpu_pll   = NULL;

#ifdef CONFIG_GPU_DVFS

struct mali_dvfstab
{
    unsigned int vol_max;
    unsigned int freq_max;
    unsigned int freq_min;
    unsigned int mbus_freq;
};
struct mali_dvfstab mali_dvfs_table[] = {
    //extremity
    {1.3*1000*1000, 336*1000*1000, 336*1000*1000, 400*1000*1000},
    //perf
    {1.2*1000*1000, 336*1000*1000, 192*1000*1000, 300*1000*1000},
    //normal
    {1.2*1000*1000, 240*1000*1000, 192*1000*1000, 300*1000*1000},
    {0,0,0,0},
    {0,0,0,0},
};

unsigned int cur_mode   = SW_POWERNOW_PERFORMANCE;
unsigned int user_event = 0;
struct regulator *mali_regulator = NULL;
struct clk *h_mbus_clk           = NULL;
static struct workqueue_struct *mali_dvfs_wq    = 0;
static unsigned int mali_dvfs_utilization       = 255;
static void mali_dvfs_work_handler(struct work_struct *w);
static DECLARE_WORK(mali_dvfs_work, mali_dvfs_work_handler);
//make compiler happy
static mali_bool init_mali_dvfs(void);
static void deinit_mali_dvfs(void);
static int mali_freq_init(void);
static void mali_freq_exit(void);

#endif

module_param(mali_clk_div, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(mali_clk_div, "Clock divisor for mali");

static void mali_platform_device_release(struct device *device);
void mali_gpu_utilization_handler(u32 utilization);

typedef enum mali_power_mode_tag
{
	MALI_POWER_MODE_ON,
	MALI_POWER_MODE_LIGHT_SLEEP,
	MALI_POWER_MODE_DEEP_SLEEP,
} mali_power_mode;

static struct resource mali_gpu_resources[]=
{
    /*
    //MALI_GPU_RESOURCES_MALI400_MP2_PMU(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq, pp1_irq, pp1_mmu_irq) \
    */
    MALI_GPU_RESOURCES_MALI400_MP2_PMU(SW_PA_MALI_IO_BASE, AW_IRQ_GPU_GP, AW_IRQ_GPU_GPMMU, \
                                        AW_IRQ_GPU_PP0, AW_IRQ_GPU_PPMMU0, AW_IRQ_GPU_PP1, AW_IRQ_GPU_PPMMU1)
};

static struct platform_device mali_gpu_device =
{
    .name = MALI_GPU_NAME_UTGARD,
    .id = 0,
    .dev.release = mali_platform_device_release,
};

static struct mali_gpu_device_data mali_gpu_data = 
{
    .dedicated_mem_start = SW_GPU_MEM_BASE,
    .dedicated_mem_size = SW_GPU_MEM_SIZE,
    .shared_mem_size = 512*1024*1024,
    .fb_start = SW_FB_MEM_BASE,
    .fb_size = SW_FB_MEM_SIZE,
    .utilization_interval = 2000,
    .utilization_handler = mali_gpu_utilization_handler,
};

static void mali_platform_device_release(struct device *device)
{
    MALI_DEBUG_PRINT(2,("mali_platform_device_release() called\n"));
}

_mali_osk_errcode_t mali_platform_init(void)
{
	unsigned long rate;
    script_item_u   mali_use, clk_drv;
    
    h_ahb_mali = h_mali_clk = h_gpu_pll = 0;
	//get mali ahb clock
    h_ahb_mali = clk_get(NULL, CLK_AHB_MALI); 
	if(!h_ahb_mali || IS_ERR(h_ahb_mali)){
		MALI_PRINT(("try to get ahb mali clock failed!\n"));
        return _MALI_OSK_ERR_FAULT;
	} else
		pr_info("%s(%d): get %s handle success!\n", __func__, __LINE__, CLK_AHB_MALI);

	//get mali clk
	h_mali_clk = clk_get(NULL, CLK_MOD_MALI);
	if(!h_mali_clk || IS_ERR(h_mali_clk)){
		MALI_PRINT(("try to get mali clock failed!\n"));
        return _MALI_OSK_ERR_FAULT;
	} else
		pr_info("%s(%d): get %s handle success!\n", __func__, __LINE__, CLK_MOD_MALI);

	h_gpu_pll = clk_get(NULL, CLK_SYS_PLL8);
	if(!h_gpu_pll || IS_ERR(h_gpu_pll)){
		MALI_PRINT(("try to get ve pll clock failed!\n"));
        return _MALI_OSK_ERR_FAULT;
	} else
		pr_info("%s(%d): get %s handle success!\n", __func__, __LINE__, CLK_SYS_PLL4);

	//set mali parent clock
	if(clk_set_parent(h_mali_clk, h_gpu_pll)){
		MALI_PRINT(("try to set mali clock source failed!\n"));
        return _MALI_OSK_ERR_FAULT;
	} else
		pr_info("%s(%d): set mali clock source success!\n", __func__, __LINE__);
	
	//set mali clock
	rate = clk_get_rate(h_gpu_pll);
	pr_info("%s(%d): get ve pll rate %d!\n", __func__, __LINE__, (int)rate);

	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_used", &mali_use)) {
		pr_info("%s(%d): get mali_para->mali_used success! mali_use %d\n", __func__, __LINE__, mali_use.val);
		if(mali_use.val == 1) {
			if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_clkdiv", &clk_drv)) {
				pr_info("%s(%d): get mali_para->mali_clkdiv success! clk_drv %d\n", __func__,
					__LINE__, clk_drv.val);
				if(clk_drv.val > 0)
					mali_clk_div = clk_drv.val;
			} else
				pr_info("%s(%d): get mali_para->mali_clkdiv failed!\n", __func__, __LINE__);
		}
	} else
		pr_info("%s(%d): get mali_para->mali_used failed!\n", __func__, __LINE__);

	pr_info("%s(%d): mali_clk_div %d\n", __func__, __LINE__, mali_clk_div);
	rate /= mali_clk_div;
	if(clk_set_rate(h_mali_clk, rate)){
		MALI_PRINT(("try to set mali clock failed!\n"));
        return _MALI_OSK_ERR_FAULT;
	} else
		pr_info("%s(%d): set mali clock rate success!\n", __func__, __LINE__);
	
	if(clk_reset(h_mali_clk, AW_CCU_CLK_NRESET)){
		MALI_PRINT(("try to reset release failed!\n"));
        return _MALI_OSK_ERR_FAULT;
	} else
		pr_info("%s(%d): reset release success!\n", __func__, __LINE__);
		
    if(mali_clk_flag == 0)//jshwang add 2012-8-23 16:05:50
	{
		//printk(KERN_WARNING "enable mali clock\n");
		MALI_PRINT(("enable mali clock\n"));
		mali_clk_flag = 1;
        if(clk_enable(h_ahb_mali)){
		     MALI_PRINT(("try to enable mali ahb failed!\n"));
        }
        if(clk_enable(h_mali_clk)){
            MALI_PRINT(("try to enable mali clock failed!\n"));
        }
	}
    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
    /*close mali axi/apb clock*/
    if(mali_clk_flag == 1){
        //MALI_PRINT(("disable mali clock\n"));
        mali_clk_flag = 0;
        clk_disable(h_mali_clk);
        clk_disable(h_ahb_mali);
    }
    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
    if(power_mode == MALI_POWER_MODE_ON){
        if(mali_clk_flag == 0){
            mali_clk_flag = 1;
            if(clk_enable(h_ahb_mali)){
                MALI_PRINT(("try to enable mali ahb failed!\n"));
                return _MALI_OSK_ERR_FAULT;
            }
            if(clk_enable(h_mali_clk)){
                MALI_PRINT(("try to enable mali clock failed!\n"));
                return _MALI_OSK_ERR_FAULT;
            }
        }
    } else if(power_mode == MALI_POWER_MODE_LIGHT_SLEEP){
        //close mali axi/apb clock/
        if(mali_clk_flag == 1){
            //MALI_PRINT(("disable mali clock\n"));
            mali_clk_flag = 0;
            clk_disable(h_mali_clk);
            clk_disable(h_ahb_mali);
        }
    } else if(power_mode == MALI_POWER_MODE_DEEP_SLEEP){
    	//close mali axi/apb clock
        if(mali_clk_flag == 1){
            //MALI_PRINT(("disable mali clock\n"));
            mali_clk_flag = 0;
            clk_disable(h_mali_clk);
            clk_disable(h_ahb_mali);
        }
    }
    MALI_SUCCESS;
}

int sun7i_mali_platform_device_register(void)
{
    int err;

    MALI_DEBUG_PRINT(2,("sun7i_mali_platform_device_register() called\n"));

    err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources, sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
    if (0 == err){
        err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
        if(0 == err){
            err = platform_device_register(&mali_gpu_device);
            if (0 == err){
                if(_MALI_OSK_ERR_OK != mali_platform_init())return _MALI_OSK_ERR_FAULT;
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif
                MALI_PRINT(("sun7i_mali_platform_device_register() sucess!!\n"));
#ifdef CONFIG_GPU_DVFS
                mali_freq_init();
#endif
                return 0;
            }
        }

        MALI_DEBUG_PRINT(2,("sun7i_mali_platform_device_register() add data failed!\n"));

        platform_device_unregister(&mali_gpu_device);
    }
    return err;
}

void mali_platform_device_unregister(void)
{
    MALI_DEBUG_PRINT(2, ("mali_platform_device_unregister() called!\n"));
    
    mali_platform_deinit();
#ifdef CONFIG_GPU_DVFS
    mali_freq_exit();
#endif
    platform_device_unregister(&mali_gpu_device);
}

#ifndef CONFIG_GPU_DVFS
void mali_gpu_utilization_handler(u32 utilization)
{
}
#endif
void set_mali_parent_power_domain(void* dev)
{
}

#ifdef CONFIG_GPU_DVFS

//modify  them , you must do know what you do
#define MALI_CLK_MAX 336*1000*1000
#define MALI_CLK_MIN 192*1000*1000
#define MBUS_VOL_MAX 13*1000*100 //1.3V
static int mali_dvfs_change_status(struct regulator *mreg, unsigned int vol,
                                    struct clk *mali_clk, unsigned int mali_freq, 
                                    struct clk *mbus_clk, unsigned int mbus_freq)
{
    unsigned int mvol   = 0;
    unsigned int mfreq  = 0;
    unsigned int mbfreq  = 0;

    //do not use too much "if" to make the fucking code
    mreg?(mvol = regulator_get_voltage(mreg)):0;
    mali_clk?(mfreq = clk_get_rate(mali_clk)):0;
    mbus_clk?(mbfreq = clk_get_rate(mbus_clk)):0;
    
    //vol up
    if (mreg && (mvol < vol)){
        regulator_set_voltage(mreg, vol, vol);
        mbfreq==mbus_freq?0:(mbus_clk?clk_set_rate(mbus_clk, 200*1000*1000):0);
        mbfreq==mbus_freq?0:(mbus_clk?clk_set_rate(mbus_clk, mbus_freq):0);
        mfreq==mali_freq?0:(mali_clk?clk_set_rate(mali_clk, mali_freq):0);

    }else if(mreg && vol && (mvol > vol)){
        //vol down
        mfreq==mali_freq?0:(mali_clk?clk_set_rate(mali_clk, mali_freq):0);
        mbfreq==mbus_freq?0:(mbus_clk?clk_set_rate(mbus_clk, 200*1000*1000):0);
        mbfreq==mbus_freq?0:(mbus_clk?clk_set_rate(mbus_clk, mbus_freq):0);
        mvol > MBUS_VOL_MAX?0:(regulator_set_voltage(mreg, vol, vol));
    }else{ 
        mfreq==mali_freq?0:(mali_clk?clk_set_rate(mali_clk, mali_freq):0);
        mbfreq==mbus_freq?0:(mbus_clk?clk_set_rate(mbus_clk, 200*1000*1000):0);
        mbfreq==mbus_freq?0:(mbus_clk?clk_set_rate(mbus_clk, mbus_freq):0);
    }
    //TODO, maybe need to delay for gpu stable here

    return 0;
}
static void mali_dvfs_work_handler(struct work_struct *w)
{
    unsigned mode = cur_mode;//defence complictly
    //check the mode valid
    if (mode > 4){
        mode = 1;
        cur_mode = 1;
    }
    //extremmity mode, if user event comes, we do nothing
    if (mode == SW_POWERNOW_EXTREMITY){
        mali_dvfs_change_status(mali_regulator, 
                                mali_dvfs_table[SW_POWERNOW_EXTREMITY].vol_max,
                                h_gpu_pll,
                                mali_dvfs_table[SW_POWERNOW_EXTREMITY].freq_max,
                                h_mbus_clk,
                                mali_dvfs_table[SW_POWERNOW_EXTREMITY].mbus_freq);
        user_event =0;
        MALI_DEBUG_PRINT(2,("mali_dvfs_work_handler set freq success, cur mode:%d, cur_freq:%d,cur_mbus:%d, cur_vol:%d,gpu usage:%d\n", 
                mode, (int)clk_get_rate(h_mali_clk), (int)clk_get_rate(h_mbus_clk), regulator_get_voltage(mali_regulator), mali_dvfs_utilization));
        return;
    }

    //usb mode set to current mode's freq_clk, if user event comes, we do nothing
    if (mode == SW_POWERNOW_USB){
        mali_dvfs_change_status(mali_regulator,
                                mali_dvfs_table[mode].vol_max,
                                h_gpu_pll,
                                MALI_CLK_MAX,//if we are now in normal mode and set freq to MALI_CLK_MAX,fucking that!
                                h_mbus_clk,
                                mali_dvfs_table[mode].mbus_freq);
        user_event = 0;
        MALI_DEBUG_PRINT(2,("mali_dvfs_work_handler set freq success, cur mode:%d, cur_freq:%d,cur_mbus:%d, cur_vol:%d,gpu usage:%d\n", 
                mode, (int)clk_get_rate(h_mali_clk), (int)clk_get_rate(h_mbus_clk), regulator_get_voltage(mali_regulator), mali_dvfs_utilization));
        return;
    }

    //now here ,we neither usb mode nor extremity mode,if user_event be set, change to current mode's freq_max
    if (user_event){
        mali_dvfs_change_status(mali_regulator,
                                mali_dvfs_table[mode].vol_max,
                                h_gpu_pll,
                                mali_dvfs_table[mode].freq_max, 
                                NULL,
                                0);
        user_event = 0;
        
        MALI_DEBUG_PRINT(2,("mali_dvfs_work_handler set freq success, cur mode:%d, cur_freq:%d,cur_mbus:%d, cur_vol:%d,gpu usage:%d\n", 
                mode, (int)clk_get_rate(h_mali_clk), (int)clk_get_rate(h_mbus_clk), regulator_get_voltage(mali_regulator), mali_dvfs_utilization));
        return;
    }
    //esle we are normal or performance mode, we set vol and freq base on mali utilization
#ifdef CONFIG_DYNAMIC_GPU_FREQ
    {
        unsigned int mali_freq = 0;
        if (mali_dvfs_utilization > 127){
            mali_freq = mali_dvfs_table[mode].freq_max;
        }else if (mode == SW_POWERNOW_PERFORMANCE && mali_dvfs_utilization < 60 && mali_dvfs_utilization > 20){
            mali_freq = 240*1000*1000;
        }else if (mali_dvfs_utilization < 20){
            mali_freq = mali_dvfs_table[mode].freq_min;
        }else{
            //we do nothing when in performance mode and mali_dvfs_utilization is between 127 and 60
            //we do nothinf when in normal mode and mali_dvfs_utilization is between 127 and 20
            return;
        }
        //defence to cash the gpu, we must have a max and min clk volues 
        if (mali_freq < MALI_CLK_MIN){
            printk("freq cant not be smaller than 192mHz!, mali_freq:%d\n", mali_freq);
            mali_freq = MALI_CLK_MIN;
        }else if(mali_freq > MALI_CLK_MAX){
            printk("freq can not be bigger than 336MHz, mali_freq:%d\n", mali_freq);
            mali_freq = MALI_CLK_MAX;
        }
            
        mali_dvfs_change_status(mali_regulator,
                                mali_dvfs_table[mode].vol_max,
                                h_gpu_pll,
                                mali_freq,
                                h_mbus_clk,
                                mali_dvfs_table[mode].mbus_freq);

        MALI_DEBUG_PRINT(2,("mali_dvfs_work_handler set freq success, cur mode:%d, cur_freq:%d,cur_mbus:%d, cur_vol:%d,gpu usage:%d\n", 
                mode, (int)clk_get_rate(h_mali_clk), (int)clk_get_rate(h_mbus_clk), regulator_get_voltage(mali_regulator), mali_dvfs_utilization));
    }
#else
    {
        
        mali_dvfs_change_status(mali_regulator,
                                mali_dvfs_table[mode].vol_max,
                                h_gpu_pll,
                                mali_dvfs_table[mode].freq_max,
                                h_mbus_clk,
                                mali_dvfs_table[mode].mbus_freq);

        MALI_DEBUG_PRINT(2,("mali_dvfs_work_handler set freq success, cur mode:%d, cur_freq:%d,cur_mbus:%d, cur_vol:%d,gpu usage:%d\n", 
                mode, (int)clk_get_rate(h_mali_clk), (int)clk_get_rate(h_mbus_clk), regulator_get_voltage(mali_regulator), mali_dvfs_utilization));
    }
#endif
    return;
}

static mali_bool init_mali_dvfs(void)
{
    if (!mali_dvfs_wq)
	{
		mali_dvfs_wq = create_singlethread_workqueue("mali_dvfs");
	}
	return MALI_TRUE;
}

static void deinit_mali_dvfs(void)
{
	if (mali_dvfs_wq)
	{
		destroy_workqueue(mali_dvfs_wq);
		mali_dvfs_wq = NULL;
	}
}

void mali_gpu_utilization_handler(unsigned int utilization)
{
#ifdef CONFIG_DYNAMIC_GPU_FREQ
    mali_dvfs_utilization = utilization;
	queue_work(mali_dvfs_wq, &mali_dvfs_work);
#endif
	/*add error handle here*/
}

static void mali_mode_set(unsigned long mode)
{
    cur_mode = mode;
    if (mali_regulator && h_mali_clk && h_mbus_clk){
        queue_work(mali_dvfs_wq, &mali_dvfs_work);
    }
    //do not queue the work, just change the mode
    //freq and vol will change on the work handle next schedule
}

static void mali_userevent_mode_set(void)
{
    if (mali_regulator && h_mali_clk && h_mbus_clk){
        user_event = 1;
        queue_work(mali_dvfs_wq, &mali_dvfs_work);
    }
}

static int mali_powernow_mod_change(unsigned long code, void *cmd)
{

    switch (code) {
        case SW_POWERNOW_EXTREMITY:
            MALI_DEBUG_PRINT(2,(">>> %s\n\n\n\n\n\n\n\n", (char *)cmd));
            mali_mode_set(code);
            break;

        case SW_POWERNOW_PERFORMANCE:
        case SW_POWERNOW_NORMAL:
            MALI_DEBUG_PRINT(2,(">>> %s\n\n\n\n\n\n\n\n\n", (char *)cmd));
            mali_mode_set(code);
            break;

        case SW_POWERNOW_USEREVENT:
            MALI_DEBUG_PRINT(2,(">>> %s\n\n\n\n\n\n\n\n\n", (char *)cmd));
            mali_userevent_mode_set();
            break;
        case SW_POWERNOW_USB:
            MALI_DEBUG_PRINT(2,(">>> %s\n\n\n\n\n\n", (char *)cmd));
            mali_mode_set(code);
            break;
        default:
            MALI_DEBUG_PRINT(2, ("powernow no such mode:%d, plz check!\n",code));
            break;
    }
    return 0;
}

static int powernow_notifier_call(struct notifier_block *this, unsigned long code, void *cmd)
{
    if (cur_mode == code){
        return 0;
    }
    MALI_DEBUG_PRINT(2, ("mali mode change\n\n\n\n\n\n\n"));
    return mali_powernow_mod_change(code, cmd);
}

static struct notifier_block powernow_notifier = {
	.notifier_call = powernow_notifier_call,
};

static int mali_freq_init(void)
{
    script_item_u   mali_use, mali_max_freq, mali_min_freq, mali_vol;

    mali_regulator = regulator_get(NULL, "axp20_ddr");
	if (IS_ERR(mali_regulator)) {
	    printk("get mali regulator failed\n");
        mali_regulator = NULL;
	    return -1;
	}
    h_mbus_clk = clk_get(NULL, CLK_MOD_MBUS);
    if (!h_mbus_clk || IS_ERR(h_mbus_clk)){
        printk("get mbus clk failed!\n\n\n\n\n\n\n\n\n\n");
        h_mbus_clk = NULL;
        regulator_put(mali_regulator);
        return -1;
    }
    
    mali_dvfs_table[SW_POWERNOW_USB].vol_max = regulator_get_voltage(mali_regulator);
    mali_dvfs_table[SW_POWERNOW_USB].mbus_freq = clk_get_rate(h_mbus_clk);
    mali_dvfs_table[SW_POWERNOW_USB].freq_max = MALI_CLK_MAX;
    mali_dvfs_table[SW_POWERNOW_USB].freq_min = MALI_CLK_MAX;
    MALI_DEBUG_PRINT(2, ("vol:%d, mbus:%d\n", mali_dvfs_table[SW_POWERNOW_USB].vol_max,
                        mali_dvfs_table[SW_POWERNOW_USB].mbus_freq));

	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_used", &mali_use)) {
		pr_info("%s(%d): get mali_para->mali_used success! mali_use %d\n", __func__, __LINE__, mali_use.val);
		if(mali_use.val == 1) {
			if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_normal_freq", &mali_max_freq)) {
                if (SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_min_freq", &mali_min_freq)){
                    if (mali_max_freq.val > 0 && mali_min_freq.val > 0){
                        mali_dvfs_table[SW_POWERNOW_NORMAL].freq_max = mali_max_freq.val*1000*1000;
                        mali_dvfs_table[SW_POWERNOW_NORMAL].freq_min = mali_min_freq.val*1000*1000;
                        MALI_DEBUG_PRINT(2, ("mali_max_freq:%dMhz, mali_min_freq:%dMhz\n",
                                            mali_max_freq.val, mali_min_freq.val));
                    }        
                }
            }
		}
	} else
		pr_info("%s(%d): get mali_para->mali_used failed!\n", __func__, __LINE__);
   if (SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("target","dcdc3_vol", &mali_vol)){
        if (mali_vol.val > 0){
            mali_dvfs_table[SW_POWERNOW_PERFORMANCE].vol_max = mali_vol.val*1000;
            mali_dvfs_table[SW_POWERNOW_NORMAL].vol_max = mali_vol.val*1000;
        }
   }
    init_mali_dvfs();
    register_sw_powernow_notifier(&powernow_notifier);

	return 0;
}

static void mali_freq_exit(void)
{
    deinit_mali_dvfs();
    if (mali_regulator) {
        regulator_put(mali_regulator);
    }

}

#endif
