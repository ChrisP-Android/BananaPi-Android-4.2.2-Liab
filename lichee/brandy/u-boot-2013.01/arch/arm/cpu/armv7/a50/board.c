/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * Some init for sunxi platform.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/timer.h>
#include <asm/arch/gpio.h>
#include <asm/arch/key.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/boot_type.h>
#include <asm/arch/sys_partition.h>
/* The sunxi internal brom will try to loader external bootloader
 * from mmc0, nannd flash, mmc2.
 * We check where we boot from by checking the config
 * of the gpio pin.
 */
DECLARE_GLOBAL_DATA_PTR;

u32 get_base(void)
{

	u32 val;

	__asm__ __volatile__("mov %0, pc \n":"=r"(val)::"memory");
	val &= 0xF0000000;
	val >>= 28;
	return val;
}

/* do some early init */
void s_init(void)
{
	watchdog_disable();
}

void reset_cpu(ulong addr)
{
	watchdog_enable();
loop_to_die:
	goto loop_to_die;
}

void v7_outer_cache_enable(void)
{
	return ;
}

void v7_outer_cache_inval_all(void)
{
	return ;
}

void v7_outer_cache_flush_range(u32 start, u32 stop)
{
	return ;
}

void enable_caches(void)
{
    icache_enable();
    dcache_enable();
}

void disable_caches(void)
{
    icache_disable();
	dcache_disable();
}

int display_inner(void)
{
	printf("version: %s\n", uboot_spare_head.boot_head.version);

	return 0;
}

int script_init(void)
{
	return 0;
}

int power_source_init(void)
{
	return 0;
}

int check_update_key(void)
{
	return 0;
}



