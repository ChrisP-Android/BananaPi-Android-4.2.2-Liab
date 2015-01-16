/*
 *  arch/arm/mach-sun7i/cpu-freq/cpu-freq-table.c
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

#include <linux/types.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include "cpu-freq.h"

struct cpufreq_frequency_table sunxi_freq_tbl[] = {

    { .frequency = 720000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 768000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 816000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 864000,  .index = SUNXI_CLK_DIV(1, 3, 2, 2), },
    { .frequency = 912000,  .index = SUNXI_CLK_DIV(1, 3, 2, 2), },

    /* table end */
    { .frequency = CPUFREQ_TABLE_END,  .index = 0,              },
};

