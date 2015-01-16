/*
 * (C) Copyright 2002-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef	__ASM_GBL_DATA_H
#define __ASM_GBL_DATA_H
/*
 * The following data structure is placed in some memory which is
 * available very early after boot (like DPRAM on MPC8xx/MPC82xx, or
 * some locked parts of the data cache) to allow for a minimum set of
 * global variables during system initialization (until we have set
 * up the memory controller so that we can use RAM).
 */

typedef	struct	global_data {
	bd_t		*bd;
	unsigned long	flags;
	unsigned int	baudrate;
	unsigned long	have_console;	/* serial_init() was called */
#ifdef CONFIG_PRE_CONSOLE_BUFFER
	unsigned long	precon_buf_idx;	/* Pre-Console buffer index */
#endif
	unsigned long	env_addr;	/* Address  of Environment struct */
	unsigned long	env_valid;	/* Checksum of Environment valid? */
	unsigned long	fb_base;	/* base address of frame buffer */
#ifdef CONFIG_FSL_ESDHC
	unsigned long	sdhc_clk;
#endif
#ifdef CONFIG_AT91FAMILY
	/* "static data" needed by at91's clock.c */
	unsigned long	cpu_clk_rate_hz;
	unsigned long	main_clk_rate_hz;
	unsigned long	mck_rate_hz;
	unsigned long	plla_rate_hz;
	unsigned long	pllb_rate_hz;
	unsigned long	at91_pllb_usb_init;
#endif
#ifdef CONFIG_ARM
	/* "static data" needed by most of timer.c on ARM platforms */
	unsigned long	timer_rate_hz;
	unsigned long	tbl;
	unsigned long	tbu;
	unsigned long long	timer_reset_value;
	unsigned long	lastinc;
#endif
#ifdef CONFIG_IXP425
	unsigned long	timestamp;
#endif
	unsigned long	relocaddr;	/* Start address of U-Boot in RAM */
	phys_size_t	ram_size;	/* RAM size */
	unsigned long	mon_len;	/* monitor len */
	unsigned long	irq_sp;		/* irq stack pointer */
	unsigned long	start_addr_sp;	/* start_addr_stackpointer */
	unsigned long	reloc_off;
#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))
	unsigned long	tlb_addr;
	unsigned long	tlb_size;
#endif
#if defined(CONFIG_ALLWINNER)
	int             bi_card_num;
	int             uart_console;
    int             boot_card_num;
	unsigned int    layer_para;
    unsigned int    layer_hd;
	int             key_pressd_value;
	int             power_step_level;
	int             pmu_suspend_chgcur;
	int             pmu_runtime_chgcur;
	int             limit_vol;
	int             limit_cur;
	int             limit_pcvol;
	int             limit_pccur;

	char            *script_mod_buf;
	int             script_main_key_count;
#endif
	const void	*fdt_blob;	/* Our device tree, NULL if none */
	void		**jt;		/* jump table */
	char		env_buf[32];	/* buffer for getenv() before reloc. */
#if defined(CONFIG_POST) || defined(CONFIG_LOGBUFFER)
	unsigned long	post_log_word; /* Record POST activities */
	unsigned long	post_log_res; /* success of POST test */
	unsigned long	post_init_f_time; /* When post_init_f started */
#endif
} gd_t;

#include <asm-generic/global_data_flags.h>

#define DECLARE_GLOBAL_DATA_PTR     register volatile gd_t *gd asm ("r8")

#endif /* __ASM_GBL_DATA_H */
