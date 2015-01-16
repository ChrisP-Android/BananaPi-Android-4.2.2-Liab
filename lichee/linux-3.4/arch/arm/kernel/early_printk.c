/*
 *  linux/arch/arm/kernel/early_printk.c
 *
 *  Copyright (C) 2009 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/init.h>
#include <mach/platform.h>
#include <mach/uart.h>
#include <asm/io.h>

#define aw_readb(addr)		(*((volatile unsigned char  *)(addr)))
#define aw_readw(addr)		(*((volatile unsigned short *)(addr)))
#define aw_readl(addr)		(*((volatile unsigned long  *)(addr)))
#define aw_writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define aw_writew(v, addr)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define aw_writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

extern void printch(int);

void aw_putc(char c)
{
	while(!(aw_readb(SW_VA_UART0_IO_BASE + AW_UART_USR) & 2));
		aw_writeb(c, SW_VA_UART0_IO_BASE + AW_UART_THR);
}

static void early_write(const char *s, unsigned n)
{
	while(n-- > 0) {
		if(*s == '\n')
			printch('\r');
		aw_putc(*s);
		s++;
	}
}

static void early_console_write(struct console *con, const char *s, unsigned n)
{
	early_write(s, n);
}

static struct console early_console = {
	.name =		"earlycon",
	.write =	early_console_write,
	.flags =	CON_PRINTBUFFER | CON_BOOT,
	.index =	-1,
};

asmlinkage void early_printk(const char *fmt, ...)
{
	char buf[512];
	int n;
	va_list ap;

	va_start(ap, fmt);
	n = vscnprintf(buf, sizeof(buf), fmt, ap);
	early_write(buf, n);
	va_end(ap);
}

static int __init setup_early_printk(char *buf)
{
	register_console(&early_console);
	return 0;
}

early_param("earlyprintk", setup_early_printk);
