/*
 ***************************************************************************************************
 *                                               LINUX-KERNEL
 *                                   ReuuiMlla Linux Platform Develop Kits
 *                                              Kernel Module
 *
 *                                   (c) Copyright 2006-2011, pannan China
 *                                            All Rights Reserved
 *
 * File    : cpu_freq_dvfs.c
 * By      : pannan
 * Version : v1.0
 * Date    : 2011-09-01
 * Descript: cpufreq driver on Reuuimlla chips;
 * Update  : date                auther      ver     notes
 ***************************************************************************************************
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm/io.h>
#include "cpu-freq.h"
#include "pmu_dvfs.h"


static struct sun5i_cpu_freq_t  cpu_cur;    /* current cpu frequency configuration */
static unsigned int last_target = ~0;       /* backup last target frequency        */

static struct clk *clk_pll; /* pll clock handler */
static struct clk *clk_cpu; /* cpu clock handler */
static struct clk *clk_axi; /* axi clock handler */
static struct clk *clk_ahb; /* ahb clock handler */
static struct clk *clk_apb; /* apb clock handler */

/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_verify
 *
 *Description: check if the cpu frequency policy is valid;
 *
 *Arguments  : policy    cpu frequency policy;
 *
 *Return     : result, return if verify ok, else return -EINVAL;
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static int sun5i_cpufreq_verify(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return -EINVAL;

	return 0;
}


/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_show
 *
 *Description: show cpu frequency information;
 *
 *Arguments  : pfx   name;
 *
 *
 *Return     :
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static void sun5i_cpufreq_show(const char *pfx, struct sun5i_cpu_freq_t *cfg)
{
	CPUFREQ_DBG("%s: pll=%u, cpudiv=%u, axidiv=%u, ahbdiv=%u, apb=%u\n",
        pfx, cfg->pll, cfg->div.cpu_div, cfg->div.axi_div, cfg->div.ahb_div, cfg->div.apb_div);
}


/*
 ***************************************************************************************************
 *                           __set_cpufreq_target
 *
 *Description: set target frequency, the frequency limitation of axi is 450Mhz, the frequency
 *             limitation of ahb is 250Mhz, and the limitation of apb is 150Mhz. for usb connecting,
 *             the frequency of ahb must not lower than 60Mhz.
 *
 *Arguments  : old   cpu/axi/ahb/apb frequency old configuration.
 *             new   cpu/axi/ahb/apb frequency new configuration.
 *
 *Return     : result, 0 - set frequency and voltage successed
 *                    -1 - set voltage failed
 *Notes      :
 *
 ***************************************************************************************************
*/
static int __set_cpufreq_target(struct sun5i_cpu_freq_t *old, struct sun5i_cpu_freq_t *new)
{
    struct sun5i_cpu_freq_t old_freq, new_freq;

    if(!old || !new) {
        return -EINVAL;
    }

    old_freq = *old;
    new_freq = *new;

    //printk("cpu: %dMhz->%dMhz\n", old_freq.pll/1000000, new_freq.pll/1000000);

    /* try to adjust pll frequency */
    clk_set_rate(clk_pll,new_freq.pll);
    /* try to adjust cpu frequency */
    clk_set_rate(clk_cpu, new_freq.pll/new_freq.div.cpu_div);

    /* start dvfs, wait for finish */
    pmu_dvfs_start_to_finish();

    //printk("pending: 0x%x\n", readl(SW_VA_PMU_IO_BASE+0x44));

    /* dvfs voltage change failed */
    if(pmu_dvfs_volt_chg_err()){
        DVFS_ERR("dvfs voltage change failed!\n");
        /* clear pending */
        pmu_dvfs_clear_pending();
        /* start dvfs again, wait for finish */
        pmu_dvfs_start_to_finish();
        printk("pending(volt err): 0x%x\n", readl(SW_VA_PMU_IO_BASE+0x44));
        /* dvfs voltage change failed again */
        if(pmu_dvfs_volt_chg_err()){
            DVFS_ERR("dvfs voltage change failed second!\n");
            /* clear pending */
            pmu_dvfs_clear_pending();
            return -1;
        }
    }

    /* clear pending */
    pmu_dvfs_clear_pending();

    CPUFREQ_DBG("%s: finished\n", __func__);

    return 0;
}


/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_settarget
 *
 *Description: adjust cpu frequency;
 *
 *Arguments  : policy    cpu frequency policy, to mark if need notify
 *             cpu_freq  new cpu frequency configuration
 *
 *Return     : return 0 if set successed, otherwise, return -EINVAL
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static int sun5i_cpufreq_settarget(struct cpufreq_policy *policy, struct sun5i_cpu_freq_t *cpu_freq)
{
    struct cpufreq_freqs    freqs;
    struct sun5i_cpu_freq_t cpu_new;

    /* show current cpu frequency configuration, just for debug */
	// sun5i_cpufreq_show("cur", &cpu_cur);

	if(!cpu_freq){
		return -EINVAL;
	}

    /* get new cpu frequency configuration */
	cpu_new = *cpu_freq;
	// sun5i_cpufreq_show("new", &cpu_new);

    /* notify that cpu clock will be adjust if needed */
	if (policy) {
       freqs.cpu = 0;
	    freqs.old = cpu_cur.pll / 1000;
	    freqs.new = cpu_new.pll / 1000;
       cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	}

    if(__set_cpufreq_target(&cpu_cur, &cpu_new)) {
        /* try to set voltage failed when voltage build up*/
        if(cpu_new.pll > cpu_cur.pll) {
            /* notify everyone that clock transition finish */
            if (policy) {
    	    	freqs.cpu = 0;
                freqs.old = freqs.new;
                freqs.new = cpu_cur.pll / 1000;
    	        cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	        }
            return -EINVAL;
        }
    }

	/* update our current settings */
	cpu_cur = cpu_new;

	/* notify everyone we've done this */
	if (policy) {
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	}

	CPUFREQ_DBG("%s: finished\n", __func__);

	return 0;
}


/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_target
 *
 *Description: adjust the frequency that cpu is currently running;
 *
 *Arguments  : policy    cpu frequency policy;
 *             freq      target frequency to be set, based on khz;
 *             relation  method for selecting the target requency;
 *
 *Return     : result, return 0 if set target frequency successed,
 *                     else, return -EINVAL;
 *
 *Notes      : this function is called by the cpufreq core;
 *
 ***************************************************************************************************
*/
static int sun5i_cpufreq_target(struct cpufreq_policy *policy, __u32 freq, __u32 relation)
{
    int                     ret;
    unsigned int            index;
    struct sun5i_cpu_freq_t freq_cfg;

    DVFS_DBG("target_freq: %d\n", freq);
    DVFS_DBG("last_target: %d\n", last_target);

    /* avoid repeated calls which cause a needless amout of duplicated
	 * logging output (and CPU time as the calculation process is
	 * done) */
	if (freq == last_target) {
        DVFS_DBG("target_freq == last_target\n");
		return 0;
	}

	/* try to look for a valid frequency value from cpu frequency table */
    if (cpufreq_frequency_table_target(policy, sun5i_freq_tbl, freq, relation, &index)) {
        CPUFREQ_ERR("%s: try to look for a valid frequency for %u failed!\n", __func__, freq);
		return -EINVAL;
	}

    DVFS_DBG("table_freq: %d\n", sun5i_freq_tbl[index].frequency);

    /* frequency is same as the value last set, need not adjust */
    if (sun5i_freq_tbl[index].frequency == last_target) {
        DVFS_DBG("table_freq == last_target\n");
		return 0;
	}
    freq = sun5i_freq_tbl[index].frequency;

    /* update the target frequency */
    freq_cfg.pll = sun5i_freq_tbl[index].frequency * 1000;
    freq_cfg.div = *(struct sun5i_clk_div_t *)&sun5i_freq_tbl[index].index;
    CPUFREQ_DBG("%s: target frequency find is %u, entry %u\n", __func__, freq_cfg.pll, index);

	/* try to set target frequency */
	ret = sun5i_cpufreq_settarget(policy, &freq_cfg);
    if(!ret) {
        last_target = freq;
    }

    CPUFREQ_DBG("%s: finished!\n", __func__);

    return ret;
}


/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_get
 *
 *Description: get the frequency that cpu currently is running;
 *
 *Arguments  : cpu   cpu number;
 *
 *Return     : cpu frequency, based on khz;
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static unsigned int sun5i_cpufreq_get(unsigned int cpu)
{
	return clk_get_rate(clk_cpu) / 1000;
}


/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_init
 *
 *Description: cpu frequency initialise a policy;
 *
 *Arguments  : policy    cpu frequency policy;
 *
 *Return     : result, return 0 if init ok, else, return -EINVAL;
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static int sun5i_cpufreq_init(struct cpufreq_policy *policy)
{
	CPUFREQ_DBG(KERN_INFO "%s: initialising policy %p\n", __func__, policy);

	if (policy->cpu != 0)
		return -EINVAL;

	policy->cur = sun5i_cpufreq_get(0);
	policy->min = policy->cpuinfo.min_freq = SUN5I_CPUFREQ_MIN / 1000;
	policy->max = policy->cpuinfo.max_freq = SUN5I_CPUFREQ_MAX / 1000;
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	/* feed the latency information from the cpu driver */
	policy->cpuinfo.transition_latency = SUN5I_FREQTRANS_LATENCY;

	return 0;
}


/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_getcur
 *
 *Description: get current cpu frequency configuration;
 *
 *Arguments  : cfg   cpu frequency cofniguration;
 *
 *Return     : result;
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static int sun5i_cpufreq_getcur(struct sun5i_cpu_freq_t *cfg)
{
    unsigned int    freq, freq0;

    if(!cfg) {
        return -EINVAL;
    }

	cfg->pll = clk_get_rate(clk_pll);
    freq = clk_get_rate(clk_cpu);
    cfg->div.cpu_div = cfg->pll / freq;
    freq0 = clk_get_rate(clk_axi);
    cfg->div.axi_div = freq / freq0;
    freq = clk_get_rate(clk_ahb);
    cfg->div.ahb_div = freq0 / freq;
    freq0 = clk_get_rate(clk_apb);
    cfg->div.apb_div = freq /freq0;

	return 0;
}


#ifdef CONFIG_PM

/* variable for backup cpu frequency configuration */
static struct sun5i_cpu_freq_t suspend_freq;

/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_suspend
 *
 *Description: back up cpu frequency configuration for suspend;
 *
 *Arguments  : policy    cpu frequency policy;
 *
 *Return     : return 0,
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static int sun5i_cpufreq_suspend(struct cpufreq_policy *policy)
{
    struct sun5i_cpu_freq_t suspend;

    CPUFREQ_DBG("%s, set cpu frequency to 60Mhz to prepare enter standby\n", __func__);
    sun5i_cpufreq_getcur(&suspend_freq);

    /* set cpu frequency to 60M hz for standby */
    suspend.pll = 60000000;
    suspend.div.cpu_div = 1;
    suspend.div.axi_div = 1;
    suspend.div.ahb_div = 1;
    suspend.div.apb_div = 2;

    /* save cpu frequency configuration */
    __set_cpufreq_target(&suspend_freq, &suspend);

    /* dvfs suspend */
    pmu_dvfs_suspend();

    CPUFREQ_DBG("%s: standby done\n", __func__);

    return 0;
}


/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_resume
 *
 *Description: cpu frequency configuration resume;
 *
 *Arguments  : policy    cpu frequency policy;
 *
 *Return     : result;
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static int sun5i_cpufreq_resume(struct cpufreq_policy *policy)
{
    struct sun5i_cpu_freq_t suspend;

    /* dvfs resume */
    pmu_dvfs_resume();

    /* invalidate last_target setting */
	last_target = ~0;

	CPUFREQ_DBG("%s: resuming with policy %p\n", __func__, policy);
    sun5i_cpufreq_getcur(&suspend);

    /* restore cpu frequency configuration */
    __set_cpufreq_target(&suspend, &suspend_freq);

	CPUFREQ_DBG("%s: resuming done\n", __func__);

	return 0;
}


#else /* #ifdef CONFIG_PM */

#define sun5i_cpufreq_suspend   NULL
#define sun5i_cpufreq_resume    NULL

#endif /* #ifdef CONFIG_PM */


static struct cpufreq_driver sun5i_cpufreq_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= sun5i_cpufreq_verify,
	.target		= sun5i_cpufreq_target,
	.get		= sun5i_cpufreq_get,
	.init		= sun5i_cpufreq_init,
	.suspend	= sun5i_cpufreq_suspend,
	.resume		= sun5i_cpufreq_resume,
	.name	  	= "sun5i-cpufreq",
};


/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_initclks
 *
 *Description: init cpu frequency clock resource;
 *
 *Arguments  : none
 *
 *Return     : result;
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static __init int sun5i_cpufreq_initclks(void)
{
    clk_pll = clk_get(NULL, "core_pll");
    clk_cpu = clk_get(NULL, "cpu");
    clk_axi = clk_get(NULL, "axi");
    clk_ahb = clk_get(NULL, "ahb");
    clk_apb = clk_get(NULL, "apb");

	if (IS_ERR(clk_pll) || IS_ERR(clk_cpu) || IS_ERR(clk_axi) ||
	    IS_ERR(clk_ahb) || IS_ERR(clk_apb)) {
		CPUFREQ_INF(KERN_ERR "%s: could not get clock(s)\n", __func__);
		return -ENOENT;
	}

	CPUFREQ_INF("%s: clocks pll=%lu,cpu=%lu,axi=%lu,ahp=%lu,apb=%lu\n", __func__,
	       clk_get_rate(clk_pll), clk_get_rate(clk_cpu), clk_get_rate(clk_axi),
	       clk_get_rate(clk_ahb), clk_get_rate(clk_apb));   

	return 0;
}


/*
 ***************************************************************************************************
 *                           sun5i_cpufreq_initcall
 *
 *Description: cpu frequency driver initcall
 *
 *Arguments  : none
 *
 *Return     : result
 *
 *Notes      :
 *
 ***************************************************************************************************
*/
static int __init sun5i_cpufreq_initcall(void)
{
	int ret = 0;

    /* initialise some clock resource */
    ret = sun5i_cpufreq_initclks();
    if(ret) {
        return ret;
    }

    /* initialise dvfs hardware configuration */
    ret = pmu_dvfs_hw_init();
    if(ret) {
        return ret;
    }

    /* initialise current frequency configuration */
	sun5i_cpufreq_getcur(&cpu_cur);
	sun5i_cpufreq_show("cur", &cpu_cur);

    /* register cpu frequency driver */
    ret = cpufreq_register_driver(&sun5i_cpufreq_driver);
    /* register cpu frequency table to cpufreq core */
    cpufreq_frequency_table_get_attr(sun5i_freq_tbl, 0);
    /* update policy for boot cpu */
    cpufreq_update_policy(0);

	return ret;
}


late_initcall(sun5i_cpufreq_initcall);
