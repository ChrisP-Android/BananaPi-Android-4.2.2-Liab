/*
 *  arch/arm/mach-sun7i/cpu-freq/cpu-freq.h
 *
 * Copyright (c) 2012 Allwinner.
 * kevin.z.m (kevin@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __CPU_FREQ_H__
#define __CPU_FREQ_H__

#include <linux/types.h>
#include <linux/cpufreq.h>

#undef CPUFREQ_DBG
#undef CPUFREQ_ERR
#undef CPUFREQ_INF
#if (1)
    #define CPUFREQ_DBG(format,args...)   printk("[cpu_freq] DBG:"format,##args)
#else
    #define CPUFREQ_DBG(format,args...)   do{}while(0)
#endif

#define CPUFREQ_INF(format,args...)   printk("[cpu_freq] INF:"format,##args)
#define CPUFREQ_ERR(format,args...)   printk("[cpu_freq] ERR:"format,##args)

#define SUN5I_CPUFREQ_MAX       (1008000000)    /* config the maximum frequency of sun5i core */
#define SUN5I_CPUFREQ_MIN       (60000000)      /* config the minimum frequency of sun5i core */
#define SUN5I_FREQTRANS_LATENCY (2000000)       /* config the transition latency, based on ns */

struct sun5i_clk_div_t {
    __u32   cpu_div:4;      /* division of cpu clock, divide core_pll */
    __u32   axi_div:4;      /* division of axi clock, divide cpu clock*/
    __u32   ahb_div:4;      /* division of ahb clock, divide axi clock*/
    __u32   apb_div:4;      /* division of apb clock, divide ahb clock*/
    __u32   reserved:16;
};


struct sun5i_cpu_freq_t {
    __u32                   pll;    /* core pll frequency value */
    struct sun5i_clk_div_t  div;    /* division configuration   */
};


#define SUN5I_CLK_DIV(cpu_div, axi_div, ahb_div, apb_div)       \
                ((cpu_div<<0)|(axi_div<<4)|(ahb_div<<8)|(apb_div<<12))



extern struct cpufreq_frequency_table sun5i_freq_tbl[];

#endif  /* #ifndef __CPU_FREQ_H__ */


