/*
 * drivers/tty/serial/sw_uart.c
 * (C) Copyright 2007-2011
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * Aaron.Maoye <leafy.myeh@reuuimllatech.com>
 *
 * description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>

#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>

#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <mach/clock.h>

#include "sw_uart.h"

#define SW_UART_NR	8

#define CONFIG_SW_UART_FORCE_LCR
//#define CONFIG_SW_UART_DEBUG
//#define CONFIG_SW_UART_DUMP_DATA
/*
 * ********************* Note **********************
 * CONFIG_SW_UART_DUMP_DATA may cause some problems
 * with some commands of 'dmesg', 'logcat', and
 * 'cat /proc/kmsg' in the console. This problem may
 * cause kernel to dead. These commands will work fine
 * in the adb shell. So you must be very clear with
 * this problem if you want define this macro to debug.
 */
/* debug control */
#define DEBUG_CONDITION (sw_uport->id == 3)
#define SERIAL_MSG(...) printk("[uart]: "__VA_ARGS__)

#ifdef CONFIG_SW_UART_DEBUG
#define SERIAL_DBG(...) do { \
				if (DEBUG_CONDITION) \
					printk(KERN_DEBUG "[uart]: "__VA_ARGS__); \
			} while (0)
static void dumpreg(struct sw_uart_port* sw_uport)
{
	u32 i;
	char* base = sw_uport->port.membase;

	printk(KERN_DEBUG "uart %d registers:", sw_uport->id);
	for (i=0; i<0x20; i+=4) {
		if (!(i&0xf))
			printk(KERN_DEBUG "\n0x%p : ", base + i);
		if (i==0 || i==8)
			printk(KERN_DEBUG "  %8s ", " ");
		else
			printk(KERN_DEBUG "0x%08x ", readl(base + i));
	}
	for (i=0x70; i<0xa8; i+=4) {
		if (!(i&0xf))
			printk(KERN_DEBUG "\n0x%p : ", base + i);
		if (i==0 || i==8)
			printk(KERN_DEBUG "  %8s ", " ");
		else
			printk(KERN_DEBUG "0x%08x ", readl(base + i));
	}
	printk(KERN_DEBUG "\n");
}
#else
#define SERIAL_DBG(up, ...)
#endif

#ifdef CONFIG_SW_UART_DUMP_DATA
static void sw_uart_dump_data(struct sw_uart_port *sw_uport, char* prompt)
{
	int i, j;
	int head = 0;
	char* buf = sw_uport->dump_buff;
	u32 len = sw_uport->dump_len;
	static char pbuff[128];
	u32 idx = 0;

	BUG_ON(sw_uport->dump_len > MAX_DUMP_SIZE);
	BUG_ON(!sw_uport->dump_buff);
	#define MAX_DUMP_PER_LINE	(16)
	#define MAX_DUMP_PER_LINE_HALF	(MAX_DUMP_PER_LINE >> 1)
	printk(KERN_DEBUG "%s len %d\n", prompt, len);
	for (i = 0; i < len;) {
		if ((i & (MAX_DUMP_PER_LINE-1)) == 0) {
			idx += sprintf(&pbuff[idx], "%04x: ", i);
			head = i;
		}
		idx += sprintf(&pbuff[idx], "%02x ", buf[i]&0xff);
		if ((i & (MAX_DUMP_PER_LINE-1)) == MAX_DUMP_PER_LINE-1
			|| i==len-1) {
			for (j=i-head+1; j<MAX_DUMP_PER_LINE; j++)
				idx += sprintf(&pbuff[idx], "   ");
			idx += sprintf(&pbuff[idx], " |");
			for (j=head; j<=i; j++) {
				if (isascii(buf[j]) && isprint(buf[j]))
					idx += sprintf(&pbuff[idx], "%c", buf[j]);
				else
					idx += sprintf(&pbuff[idx], ".");
			}
			idx += sprintf(&pbuff[idx], "|\n");
			pbuff[idx] = '\0';
			printk(KERN_DEBUG "%s", pbuff);
			idx = 0;
		}
		i++;
	}
	sw_uport->dump_len = 0;
}
#define SERIAL_DUMP(up, ...) do { \
				if (DEBUG_CONDITION) \
					sw_uart_dump_data(up, __VA_ARGS__); \
			} while (0)
#else
#define SERIAL_DUMP(up, ...)	{up->dump_len = 0;}
#endif

#define UART_TO_SPORT(port)	((struct sw_uart_port*)port)

static inline unsigned char serial_in(struct uart_port *port, int offs)
{
	return __raw_readb(port->membase + offs);
}

static inline void serial_out(struct uart_port *port, unsigned char value, int offs)
{
	__raw_writeb(value, port->membase + offs);
}

static inline void sw_uart_reset(struct sw_uart_port *sw_uport)
{
#if CONFIG_ARCH_SUN7I
    clk_disable(sw_uport->pclk);
    udelay(1);
    clk_enable(sw_uport->pclk);
    udelay(1);
#endif
	clk_reset(sw_uport->mclk, AW_CCU_CLK_RESET);
	clk_reset(sw_uport->mclk, AW_CCU_CLK_NRESET);
}

static unsigned int sw_uart_handle_rx(struct sw_uart_port *sw_uport, unsigned int lsr)
{
	struct tty_struct *tty = sw_uport->port.state->port.tty;
	unsigned char ch = 0;
	int max_count = 256;
	char flag;

	do {
		if (likely(lsr & SW_UART_LSR_DR)) {
			ch = serial_in(&sw_uport->port, SW_UART_RBR);
#ifdef CONFIG_SW_UART_DUMP_DATA
			sw_uport->dump_buff[sw_uport->dump_len++] = ch;
#endif
		}

		flag = TTY_NORMAL;
		sw_uport->port.icount.rx++;

		if (unlikely(lsr & SW_UART_LSR_BRK_ERROR_BITS)) {
			/*
			 * For statistics only
			 */
			if (lsr & SW_UART_LSR_BI) {
				lsr &= ~(SW_UART_LSR_FE | SW_UART_LSR_PE);
				sw_uport->port.icount.brk++;
				/*
				 * We do the SysRQ and SAK checking
				 * here because otherwise the break
				 * may get masked by ignore_status_mask
				 * or read_status_mask.
				 */
				if (uart_handle_break(&sw_uport->port))
					goto ignore_char;
			} else if (lsr & SW_UART_LSR_PE)
				sw_uport->port.icount.parity++;
			else if (lsr & SW_UART_LSR_FE)
				sw_uport->port.icount.frame++;
			if (lsr & SW_UART_LSR_OE)
				sw_uport->port.icount.overrun++;

			/*
			 * Mask off conditions which should be ignored.
			 */
			lsr &= sw_uport->port.read_status_mask;
#ifdef CONFIG_SERIAL_SW_CONSOLE
			if (sw_uport->port.line == sw_uport->port.cons->index) {
				/* Recover the break flag from console xmit */
				lsr |= sw_uport->lsr_break_flag;
			}
#endif
			if (lsr & SW_UART_LSR_BI)
				flag = TTY_BREAK;
			else if (lsr & SW_UART_LSR_PE)
				flag = TTY_PARITY;
			else if (lsr & SW_UART_LSR_FE)
				flag = TTY_FRAME;
		}
		if (uart_handle_sysrq_char(&sw_uport->port, ch))
			goto ignore_char;
		uart_insert_char(&sw_uport->port, lsr, SW_UART_LSR_OE, ch, flag);
ignore_char:
		lsr = serial_in(&sw_uport->port, SW_UART_LSR);
	} while ((lsr & (SW_UART_LSR_DR | SW_UART_LSR_BI)) && (max_count-- > 0));

	SERIAL_DUMP(sw_uport, "Rx");
	spin_unlock(&sw_uport->port.lock);
	tty_flip_buffer_push(tty);
	spin_lock(&sw_uport->port.lock);

	return lsr;
}

static void sw_uart_stop_tx(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (sw_uport->ier & SW_UART_IER_THRI) {
		sw_uport->ier &= ~SW_UART_IER_THRI;
		SERIAL_DBG("stop tx, ier %x\n", sw_uport->ier);
		serial_out(port, sw_uport->ier, SW_UART_IER);
	}
}

static void sw_uart_start_tx(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (!(sw_uport->ier & SW_UART_IER_THRI)) {
		sw_uport->ier |= SW_UART_IER_THRI;
		SERIAL_DBG("start tx, ier %x\n", sw_uport->ier);
		serial_out(port, sw_uport->ier, SW_UART_IER);
	}
}

static void sw_uart_handle_tx(struct sw_uart_port *sw_uport)
{
	struct circ_buf *xmit = &sw_uport->port.state->xmit;
	int count;

	if (sw_uport->port.x_char) {
		serial_out(&sw_uport->port, sw_uport->port.x_char, SW_UART_THR);
		sw_uport->port.icount.tx++;
		sw_uport->port.x_char = 0;
#ifdef CONFIG_SW_UART_DUMP_DATA
		sw_uport->dump_buff[sw_uport->dump_len++] = sw_uport->port.x_char;
		SERIAL_DUMP(sw_uport, "Tx");
#endif
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&sw_uport->port)) {
		sw_uart_stop_tx(&sw_uport->port);
		return;
	}
	count = sw_uport->port.fifosize / 2;
	do {
#ifdef CONFIG_SW_UART_DUMP_DATA
		sw_uport->dump_buff[sw_uport->dump_len++] = xmit->buf[xmit->tail];
#endif
		serial_out(&sw_uport->port, xmit->buf[xmit->tail], SW_UART_THR);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		sw_uport->port.icount.tx++;
		if (uart_circ_empty(xmit)) {
			break;
		}
	} while (--count > 0);

	SERIAL_DUMP(sw_uport, "Tx");
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
		spin_unlock(&sw_uport->port.lock);
		uart_write_wakeup(&sw_uport->port);
		spin_lock(&sw_uport->port.lock);
	}
	if (uart_circ_empty(xmit))
		sw_uart_stop_tx(&sw_uport->port);
}

static unsigned int sw_uart_modem_status(struct sw_uart_port *sw_uport)
{
	unsigned int status = serial_in(&sw_uport->port, SW_UART_MSR);

	status |= sw_uport->msr_saved_flags;
	sw_uport->msr_saved_flags = 0;

	if (status & SW_UART_MSR_ANY_DELTA && sw_uport->ier & SW_UART_IER_MSI &&
	    sw_uport->port.state != NULL) {
		if (status & SW_UART_MSR_TERI)
			sw_uport->port.icount.rng++;
		if (status & SW_UART_MSR_DDSR)
			sw_uport->port.icount.dsr++;
		if (status & SW_UART_MSR_DDCD)
			uart_handle_dcd_change(&sw_uport->port, status & SW_UART_MSR_DCD);
		if (!(sw_uport->mcr & SW_UART_MCR_AFE) && status & SW_UART_MSR_DCTS)
			uart_handle_cts_change(&sw_uport->port, status & SW_UART_MSR_CTS);

		wake_up_interruptible(&sw_uport->port.state->port.delta_msr_wait);
	}

	SERIAL_DBG("modem status: %x\n", status);
	return status;
}

#ifdef CONFIG_SW_UART_FORCE_LCR
static int sw_uart_force_lcr(struct sw_uart_port *sw_uport, unsigned msecs)
{
	int retval = 0;
	unsigned long expire = jiffies + msecs_to_jiffies(msecs);
	struct uart_port *port = &sw_uport->port;

	serial_out(port, SW_UART_HALT_FORCECFG, SW_UART_HALT);
	serial_out(port, sw_uport->lcr|SW_UART_LCR_DLAB, SW_UART_LCR);
	serial_out(port, sw_uport->dll, SW_UART_DLL);
	serial_out(port, sw_uport->dlh, SW_UART_DLH);
	serial_out(port, sw_uport->lcr, SW_UART_LCR);
	serial_out(port, SW_UART_HALT_FORCECFG | SW_UART_HALT_LCRUP, SW_UART_HALT);
	while (jiffies < expire && (serial_in(port, SW_UART_HALT) & SW_UART_HALT_LCRUP));
	if (serial_in(port, SW_UART_HALT) & SW_UART_HALT_LCRUP) {
		retval = 1;
//		SERIAL_MSG("uart%d, Force LCR failed\n", sw_uport->id);
	}
	serial_out(port, 0, SW_UART_HALT);
    return retval;
}
#endif

static irqreturn_t sw_uart_irq(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int iir = 0, lsr = 0;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	iir = serial_in(port, SW_UART_IIR) & SW_UART_IIR_IID_MASK;
	lsr = serial_in(port, SW_UART_LSR);
	SERIAL_DBG("irq: iir %x lsr %x\n", iir, lsr);

	if (iir == SW_UART_IIR_IID_BUSBSY) {
		/* handle busy */
//		SERIAL_MSG("uart%d busy...\n", sw_uport->id);
		serial_in(port, SW_UART_USR);

		#ifdef CONFIG_SW_UART_FORCE_LCR
		sw_uart_force_lcr(sw_uport, 10);
		#else
		serial_out(port, sw_uport->lcr, SW_UART_LCR);
		#endif
	} else {
		if (lsr & (SW_UART_LSR_DR | SW_UART_LSR_BI))
			lsr = sw_uart_handle_rx(sw_uport, lsr);
		sw_uart_modem_status(sw_uport);
		#ifdef CONFIG_SW_UART_PTIME_MODE
		if (iir == SW_UART_IIR_IID_THREMP)
		#else
		if (lsr & SW_UART_LSR_THRE)
		#endif
			sw_uart_handle_tx(sw_uport);
	}

	spin_unlock_irqrestore(&port->lock, flags);

	return IRQ_HANDLED;
}

/*
 * uart buadrate and apb2 clock config selection
 * We should select an apb2 clock as low as possible
 * for lower power comsumpition, which can satisfy the
 * different baudrates of different ttyS applications.
 * 
 * the reference table as follows:
 * pll6 600M
 * apb2div      0        20       19       18       17       16       15       14       13       12       11       10       9        8        7        6         5
 * apbclk       24000000 30000000 31578947 33333333 35294117 37500000 40000000 42857142 46153846 50000000 54545454 60000000 66666666 75000000 85714285 100000000 120000000 
 * 115200            *      *         *        *        *        *        *        *        *        *        *        *        *        *       *         *         *
 * 230400                   *         *        *        *        *        *        *        *        *        *        *        *        *       *         *         *
 * 380400            *      *         *                 *        *                 *        *        *        *        *        *        *       *         *         *
 * 460800                   *                                    *                 *        *        *        *        *        *        *       *         *         *
 * 921600                   *                                                      *        *                          *                 *       *         *         *
 * 1000000                            *        *                                            *        *                          *                          *
 * 1500000           *                                                                      *        *                                   *                 *         *
 * 1750000                                                                                                    *                                  *
 * 2000000                            *        *                                                                                *                          *
 * 2500000                                                                *                                                                                          *
 * 3000000                                                                                  *        *                                                     *
 * 3250000                                                                                                    *                                            *         
 * 3500000                                                                                                    *
 * 4000000                                                                                                                      *
 */
struct baudset {
	u32 baud;
	u32 uartclk_min;
	u32 uartclk_max;
};

static inline int sw_uart_check_baudset(struct uart_port *port, unsigned int baud)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	static struct baudset  baud_set[] = {
		{115200, 24000000, 120000000},
		{230400, 30000000, 120000000},
		{380400, 24000000, 120000000},
		{460800, 30000000, 120000000},
		{921600, 30000000, 120000000},
		{1000000, 31000000, 120000000}, //31578947
		{1500000, 24000000, 120000000},
		{1750000, 54000000, 120000000}, //54545454
		{2000000, 31000000, 120000000}, //31578947
		{2500000, 40000000, 120000000}, //40000000
		{3000000, 46000000, 120000000}, //46153846
		{3250000, 54000000, 120000000}, //54545454
		{3500000, 54000000, 120000000}, //54545454
		{4000000, 66000000, 120000000}, //66666666
	};
	struct baudset *setsel;
	int i;

	if (baud < 115200) {
		if (port->uartclk < 24000000) {
			SERIAL_MSG("uart%d, uartclk(%d) too small for baud %d\n",
				sw_uport->id, port->uartclk, baud);
			return -1;
		}
	} else {
		for (i=0;
		     i<sizeof(baud_set)/sizeof(baud_set[0]) && baud != baud_set[i].baud;
		     i++);
		if (i==sizeof(baud_set)/sizeof(baud_set[0])) {
			SERIAL_MSG("uart%d, baud %d beyond rance\n", sw_uport->id, baud);
			return -1;
		}
		setsel = &baud_set[i];
		if (port->uartclk < setsel->uartclk_min
			|| port->uartclk > setsel->uartclk_max) {
			SERIAL_MSG("uart%d, select set %d, baud %d, uartclk %d beyond rance[%d, %d]\n",
				sw_uport->id, i, baud, port->uartclk,
				setsel->uartclk_min, setsel->uartclk_max);
			return -1;
		}
	}
	return 0;
}

#define BOTH_EMPTY    (SW_UART_LSR_TEMT | SW_UART_LSR_THRE)
static inline void wait_for_xmitr(struct sw_uart_port *sw_uport)
{
	unsigned int status, tmout = 10000;
	#ifdef CONFIG_SW_UART_PTIME_MODE
	unsigned int offs = SW_UART_USR;
	unsigned char mask = SW_UART_USR_TFNF;
	#else
	unsigned int offs = SW_UART_LSR;
	unsigned char mask = BOTH_EMPTY;
	#endif

	/* Wait up to 10ms for the character(s) to be sent. */
	do {
		status = serial_in(&sw_uport->port, offs);
		if (serial_in(&sw_uport->port, SW_UART_LSR) & SW_UART_LSR_BI)
			sw_uport->lsr_break_flag = SW_UART_LSR_BI;
		if (--tmout == 0)
			break;
		udelay(1);
	} while ((status & mask) != mask);

	/* Wait up to 500ms for flow control if necessary */
	if (sw_uport->port.flags & UPF_CONS_FLOW) {
		tmout = 500000;
		for (tmout = 1000000; tmout; tmout--) {
			unsigned int msr = serial_in(&sw_uport->port, SW_UART_MSR);

			sw_uport->msr_saved_flags |= msr & MSR_SAVE_FLAGS;
			if (msr & SW_UART_MSR_CTS)
				break;

			udelay(1);
		}
	}
}

static unsigned int sw_uart_tx_empty(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags = 0;
	unsigned int ret = 0;

	spin_lock_irqsave(&sw_uport->port.lock, flags);
	ret = (serial_in(port, SW_UART_USR) & SW_UART_USR_TFE) ? TIOCSER_TEMT : 0;
	spin_unlock_irqrestore(&sw_uport->port.lock, flags);
	return ret;
}

static void sw_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int mcr = 0;

	if (mctrl & TIOCM_RTS)
		mcr |= SW_UART_MCR_RTS;
	if (mctrl & TIOCM_DTR)
		mcr |= SW_UART_MCR_DTR;
	if (mctrl & TIOCM_LOOP)
		mcr |= SW_UART_MCR_LOOP;
	sw_uport->mcr &= ~(SW_UART_MCR_RTS|SW_UART_MCR_DTR|SW_UART_MCR_LOOP);
	sw_uport->mcr |= mcr;
	SERIAL_DBG("set mcr %x\n", mcr);
	serial_out(port, sw_uport->mcr, SW_UART_MCR);
}

static unsigned int sw_uart_get_mctrl(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int msr;
	unsigned int ret = 0;

	msr = sw_uart_modem_status(sw_uport);
	if (msr & SW_UART_MSR_DCD)
		ret |= TIOCM_CAR;
	if (msr & SW_UART_MSR_RI)
		ret |= TIOCM_RNG;
	if (msr & SW_UART_MSR_DSR)
		ret |= TIOCM_DSR;
	if (msr & SW_UART_MSR_CTS)
		ret |= TIOCM_CTS;
	SERIAL_DBG("get msr %x\n", msr);
	return ret;
}

static void sw_uart_stop_rx(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (sw_uport->ier & SW_UART_IER_RLSI) {
		sw_uport->ier &= ~SW_UART_IER_RLSI;
		SERIAL_DBG("stop rx, ier %x\n", sw_uport->ier);
		sw_uport->port.read_status_mask &= ~SW_UART_LSR_DR;
		serial_out(port, sw_uport->ier, SW_UART_IER);
	}
}

static void sw_uart_enable_ms(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (!(sw_uport->ier & SW_UART_IER_MSI)) {
		sw_uport->ier |= SW_UART_IER_MSI;
		SERIAL_DBG("en msi, ier %x\n", sw_uport->ier);
		serial_out(port, sw_uport->ier, SW_UART_IER);
	}
}

static void sw_uart_break_ctl(struct uart_port *port, int break_state)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	if (break_state == -1)
		sw_uport->lcr |= SW_UART_LCR_SBC;
	else
		sw_uport->lcr &= ~SW_UART_LCR_SBC;
	serial_out(port, sw_uport->lcr, SW_UART_LCR);
	spin_unlock_irqrestore(&port->lock, flags);
}

static int sw_uart_startup(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	int ret;

	SERIAL_DBG("start up ...\n");
	snprintf(sw_uport->name, sizeof(sw_uport->name),
		 "sw_serial%d", port->line);
	ret = request_irq(port->irq, sw_uart_irq, 0, sw_uport->name, port);
	if (unlikely(ret)) {
		SERIAL_MSG("uart%d cannot get irq %d\n", sw_uport->id, port->irq);
		return ret;
	}

	sw_uport->msr_saved_flags = 0;
	/*
	 * PTIME mode to select the THRE trigger condition:
	 * if PTIME=1(IER[7]), the THRE interrupt will be generated when the
	 * the water level of the TX FIFO is lower than the threshold of the
	 * TX FIFO. and if PTIME=0, the THRE interrupt will be generated when
	 * the TX FIFO is empty.
	 * In addition, when PTIME=1, the THRE bit of the LSR register will not
	 * be set when the THRE interrupt is generated. You must check the
	 * interrupt id of the IIR register to decide whether some data need to
	 * send.
	 */
	sw_uport->ier = SW_UART_IER_RLSI | SW_UART_IER_RDI;
	#ifdef CONFIG_SW_UART_PTIME_MODE
	sw_uport->ier |= SW_UART_IER_PTIME;
	#endif

	return 0;
}

static void sw_uart_shutdown(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	SERIAL_DBG("shut down ...\n");
	sw_uport->ier = 0;
	sw_uport->lcr = 0;
	sw_uport->mcr = 0;
	sw_uport->fcr = 0;
	free_irq(port->irq, port);
}

static void sw_uart_set_termios(struct uart_port *port, struct ktermios *termios,
			    struct ktermios *old)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags;
	unsigned int baud, quot, lcr = 0, dll, dlh;
	unsigned int lcr_fail = 0;
    int t1 = 0, t2 = 0;

	SERIAL_DBG("set termios ...\n");
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		lcr |= SW_UART_LCR_WLEN5;
		break;
	case CS6:
		lcr |= SW_UART_LCR_WLEN6;
		break;
	case CS7:
		lcr |= SW_UART_LCR_WLEN7;
		break;
	case CS8:
	default:
		lcr |= SW_UART_LCR_WLEN8;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		lcr |= SW_UART_LCR_STOP;
	if (termios->c_cflag & PARENB)
		lcr |= SW_UART_LCR_PARITY;
	if (!(termios->c_cflag & PARODD))
		lcr |= SW_UART_LCR_EPAR;

	/* set buadrate */
	baud = uart_get_baud_rate(port, termios, old,
				  port->uartclk / 16 / 0xffff,
				  port->uartclk / 16);
	sw_uart_check_baudset(port, baud);
	quot = uart_get_divisor(port, baud);
	dll = quot & 0xff;
	dlh = quot >> 8;
	SERIAL_DBG("set baudrate %d, quot %d\n", baud, quot);

	spin_lock_irqsave(&port->lock, flags);
	uart_update_timeout(port, termios->c_cflag, baud);

	/* Update the per-port timeout. */
	port->read_status_mask = SW_UART_LSR_OE | SW_UART_LSR_THRE | SW_UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= SW_UART_LSR_FE | SW_UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= SW_UART_LSR_BI;

	/* Characteres to ignore */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= SW_UART_LSR_PE | SW_UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= SW_UART_LSR_BI;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= SW_UART_LSR_OE;
	}

	/*
	 * ignore all characters if CREAD is not set
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= SW_UART_LSR_DR;

	/*
	 * if lcr & baud are changed, reset controller to disable transfer
	 */
	if (lcr != sw_uport->lcr || dll != sw_uport->dll || dlh != sw_uport->dlh) {
        t1 = 1;
//		SERIAL_DBG("LCR & BAUD changed, reset controller...\n");
		sw_uart_reset(sw_uport);
	}
	sw_uport->dll = dll;
	sw_uport->dlh = dlh;

	/* flow control */
	sw_uport->mcr &= ~SW_UART_MCR_AFE;
	if (termios->c_cflag & CRTSCTS)
		sw_uport->mcr |= SW_UART_MCR_AFE;
	serial_out(port, sw_uport->mcr, SW_UART_MCR);

	/*
	 * CTS flow control flag and modem status interrupts
	 */
	sw_uport->ier &= ~SW_UART_IER_MSI;
	if (UART_ENABLE_MS(port, termios->c_cflag))
		sw_uport->ier |= SW_UART_IER_MSI;
	serial_out(port, sw_uport->ier, SW_UART_IER);

	sw_uport->fcr = SW_UART_FCR_RXTRG_1_2 | SW_UART_FCR_TXTRG_1_2
			| SW_UART_FCR_FIFO_EN;
	serial_out(port, sw_uport->fcr, SW_UART_FCR);

	sw_uport->lcr = lcr;
	serial_out(port, sw_uport->lcr|SW_UART_LCR_DLAB, SW_UART_LCR);
	if (serial_in(port, SW_UART_LCR) != (sw_uport->lcr|SW_UART_LCR_DLAB)) {
		lcr_fail = 1;
	} else {
		sw_uport->lcr = lcr;
		serial_out(port, sw_uport->dll, SW_UART_DLL);
		serial_out(port, sw_uport->dlh, SW_UART_DLH);
		serial_out(port, sw_uport->lcr, SW_UART_LCR);
		if (serial_in(port, SW_UART_LCR) != sw_uport->lcr) {
			lcr_fail = 2;
		}
	}

	#ifdef CONFIG_SW_UART_FORCE_LCR
	if (lcr_fail) {
		t2 = sw_uart_force_lcr(sw_uport, 50);
		serial_in(port, SW_UART_USR);
	}
	#endif
	port->ops->set_mctrl(port, port->mctrl);

	spin_unlock_irqrestore(&port->lock, flags);
	/* lately output force lcr information */
	if (lcr_fail == 1) {
		SERIAL_MSG("uart%d write LCR(pre-dlab) failed, lcr %x reg %x\n",
			sw_uport->id, sw_uport->lcr|(u32)SW_UART_LCR_DLAB,
			serial_in(port, SW_UART_LCR));
	} else if (lcr_fail == 2) {
		SERIAL_MSG("uart%d write LCR(post-dlab) failed, lcr %x reg %x\n",
			sw_uport->id, sw_uport->lcr, serial_in(port, SW_UART_LCR));
	}
	/* Don't rewrite B0 */
	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);
	SERIAL_DBG("termios lcr 0x%x fcr 0x%x mcr 0x%x dll 0x%x dlh 0x%x\n",
			sw_uport->lcr, sw_uport->fcr, sw_uport->mcr,
			sw_uport->dll, sw_uport->dlh);

    if (t1) {
        SERIAL_MSG("uart%d LCR & BAUD changed, reset controller...\n", sw_uport->id);
    }

    if (t2) {
		SERIAL_MSG("uart%d, Force LCR failed\n", sw_uport->id);
    }
}

static const char *sw_uart_type(struct uart_port *port)
{
	return "SW";
}

#define MAP_SIZE (0x100)
static void sw_uart_release_port(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	struct gpio_config *gpio;
	u32 i;
	int ret;

	SERIAL_DBG("release port(iounmap & release io)\n");
	/* release memory resource */
	release_mem_region(port->mapbase, MAP_SIZE);
	iounmap(port->membase);
	port->membase = NULL;
	/* release io resource */
	for (i=0; i<sw_uport->pdata->io_num; i++) {
		gpio = &sw_uport->pdata->uart_io[i];
		ret = sw_gpio_setcfg(gpio->gpio, 7);
		if (ret) {
			SERIAL_MSG("uart%d set io%d mulsel failed\n", sw_uport->id, i);
			return;
		}
		ret = sw_gpio_setdrvlevel(gpio->gpio, 1);
		if (ret) {
			SERIAL_MSG("uart%d set io%d drvlel failed\n", sw_uport->id, i);
			return;
		}
		ret = sw_gpio_setpull(gpio->gpio, 0);
		if (ret) {
			SERIAL_MSG("uart%d set io%d pull failed\n", sw_uport->id, i);
			return;
		}
	}
}

static int sw_uart_request_port(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	struct gpio_config *gpio;
	int ret;
	u32 i;

	SERIAL_DBG("request port(ioremap & request io)\n");
	/* request memory resource */
	if (!request_mem_region(port->mapbase, MAP_SIZE, "sw_serial")) {
		SERIAL_MSG("uart%d, request mem region failed\n", sw_uport->id);
		return -EBUSY;
	}
	port->membase = ioremap(port->mapbase, MAP_SIZE);
	if (!port->membase) {
		SERIAL_MSG("uart%d, ioremap failed\n", sw_uport->id);
		ret = -EBUSY;
		goto fail_release_port;
	}

	/* request io resource */
	for (i=0; i<sw_uport->pdata->io_num; i++) {
		gpio = &sw_uport->pdata->uart_io[i];
		ret = sw_gpio_setpull(gpio->gpio, gpio->pull);
		if (ret) {
			SERIAL_MSG("uart%d set io%d pull failed\n", sw_uport->id, i);
			ret = -EBUSY;
			goto fail_release_port;
		}
		ret = sw_gpio_setdrvlevel(gpio->gpio, gpio->drv_level);
		if (ret) {
			SERIAL_MSG("uart%d set io%d drvlel failed\n", sw_uport->id, i);
			ret = -EBUSY;
			goto fail_release_port;
		}
		ret = sw_gpio_setcfg(gpio->gpio, gpio->mul_sel);
		if (ret) {
			SERIAL_MSG("uart%d set io%d mulsel failed\n", sw_uport->id, i);
			ret = -EBUSY;
			goto fail_release_port;
		}
	}

	return 0;
fail_release_port:
	release_mem_region(port->mapbase, MAP_SIZE);
	return ret;
}

static void sw_uart_config_port(struct uart_port *port, int flags)
{
	int ret;

	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_SW;
		ret = sw_uart_request_port(port);
		if (ret)
			return;
	}
}

static int sw_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	if (unlikely(ser->type != PORT_UNKNOWN && ser->type != PORT_SW))
		return -EINVAL;
	if (unlikely(port->irq != ser->irq))
		return -EINVAL;
	return 0;
}

static void sw_uart_pm(struct uart_port *port, unsigned int state,
		      unsigned int oldstate)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	int ret;

	SERIAL_DBG("PM state %d -> %d\n", oldstate, state);

	switch (state) {
	case 0:
		ret = clk_enable(sw_uport->pclk);
		if (ret) {
			SERIAL_MSG("uart%d enable pclk failed\n", sw_uport->id);
		}
		ret = clk_reset(sw_uport->mclk, AW_CCU_CLK_NRESET);
		if (ret) {
			SERIAL_MSG("uart%d release reset failed\n", sw_uport->id);
		}
		break;
	case 3:
		ret = clk_reset(sw_uport->mclk, AW_CCU_CLK_RESET);
		if (ret) {
			SERIAL_MSG("uart%d set reset failed\n", sw_uport->id);
		}
		clk_disable(sw_uport->pclk);
		break;
	default:
		SERIAL_MSG("uart%d, Unknown PM state %d\n", sw_uport->id, state);
	}
}

static struct uart_ops sw_uart_ops = {
	.tx_empty = sw_uart_tx_empty,
	.set_mctrl = sw_uart_set_mctrl,
	.get_mctrl = sw_uart_get_mctrl,
	.stop_tx = sw_uart_stop_tx,
	.start_tx = sw_uart_start_tx,
	.stop_rx = sw_uart_stop_rx,
	.enable_ms = sw_uart_enable_ms,
	.break_ctl = sw_uart_break_ctl,
	.startup = sw_uart_startup,
	.shutdown = sw_uart_shutdown,
	.set_termios = sw_uart_set_termios,
	.type = sw_uart_type,
	.release_port = sw_uart_release_port,
	.request_port = sw_uart_request_port,
	.config_port = sw_uart_config_port,
	.verify_port = sw_uart_verify_port,
	.pm = sw_uart_pm,
};

static struct sw_uart_pdata sw_uport_pdata[] = {
	[0] = {.base = SW_PA_UART0_IO_BASE, .irq = AW_IRQ_UART0, .max_ios = 2},
	[1] = {.base = SW_PA_UART1_IO_BASE, .irq = AW_IRQ_UART1, .max_ios = 8},
	[2] = {.base = SW_PA_UART2_IO_BASE, .irq = AW_IRQ_UART2, .max_ios = 4},
	[3] = {.base = SW_PA_UART3_IO_BASE, .irq = AW_IRQ_UART3, .max_ios = 4},
	[4] = {.base = SW_PA_UART4_IO_BASE, .irq = AW_IRQ_UART4, .max_ios = 2},
	[5] = {.base = SW_PA_UART5_IO_BASE, .irq = AW_IRQ_UART5, .max_ios = 2},
	[6] = {.base = SW_PA_UART6_IO_BASE, .irq = AW_IRQ_UART6, .max_ios = 2},
	[7] = {.base = SW_PA_UART7_IO_BASE, .irq = AW_IRQ_UART7, .max_ios = 2},
};

static struct platform_device sw_uport_device[] = {
	[0] = {.name = "sw_serial", .id = 0, .dev.platform_data = &sw_uport_pdata[0]},
	[1] = {.name = "sw_serial", .id = 1, .dev.platform_data = &sw_uport_pdata[1]},
	[2] = {.name = "sw_serial", .id = 2, .dev.platform_data = &sw_uport_pdata[2]},
	[3] = {.name = "sw_serial", .id = 3, .dev.platform_data = &sw_uport_pdata[3]},
	[4] = {.name = "sw_serial", .id = 4, .dev.platform_data = &sw_uport_pdata[4]},
	[5] = {.name = "sw_serial", .id = 5, .dev.platform_data = &sw_uport_pdata[5]},
	[6] = {.name = "sw_serial", .id = 6, .dev.platform_data = &sw_uport_pdata[6]},
	[7] = {.name = "sw_serial", .id = 7, .dev.platform_data = &sw_uport_pdata[7]},
};

static struct sw_uart_port sw_uart_port[] = {
	{ .port = { .iotype = UPIO_MEM, .ops = &sw_uart_ops, .fifosize = 64, .line = 0, },
	  .pdata = &sw_uport_pdata[0], },
	{ .port = { .iotype = UPIO_MEM, .ops = &sw_uart_ops, .fifosize = 64, .line = 1, },
	  .pdata = &sw_uport_pdata[1], },
	{ .port = { .iotype = UPIO_MEM, .ops = &sw_uart_ops, .fifosize = 64, .line = 2, },
	  .pdata = &sw_uport_pdata[2], },
	{ .port = { .iotype = UPIO_MEM, .ops = &sw_uart_ops, .fifosize = 64, .line = 3, },
	  .pdata = &sw_uport_pdata[3], },
	{ .port = { .iotype = UPIO_MEM, .ops = &sw_uart_ops, .fifosize = 64, .line = 4, },
	  .pdata = &sw_uport_pdata[4], },
	{ .port = { .iotype = UPIO_MEM, .ops = &sw_uart_ops, .fifosize = 64, .line = 5, },
	  .pdata = &sw_uport_pdata[5], },
	{ .port = { .iotype = UPIO_MEM, .ops = &sw_uart_ops, .fifosize = 64, .line = 6, },
	  .pdata = &sw_uport_pdata[6], },
	{ .port = { .iotype = UPIO_MEM, .ops = &sw_uart_ops, .fifosize = 64, .line = 7, },
	  .pdata = &sw_uport_pdata[7], },
};

#ifdef CONFIG_SERIAL_SW_CONSOLE
static void sw_console_putchar(struct uart_port *port, int c)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	wait_for_xmitr(sw_uport);
	serial_out(port, c, SW_UART_THR);
}

static void sw_console_write(struct console *co, const char *s,
			      unsigned int count)
{
	struct uart_port *port;
	struct sw_uart_port *sw_uport;
	unsigned long flags;
	unsigned int ier;
	int locked = 1;

	BUG_ON(co->index < 0 || co->index >= SW_UART_NR);

	port = &sw_uart_port[co->index].port;
	sw_uport = UART_TO_SPORT(port);

	local_irq_save(flags);
	if (port->sysrq)
		locked = 0;
	else if (oops_in_progress)
		locked = spin_trylock(&port->lock);
	else
		spin_lock(&port->lock);
	ier = serial_in(port, SW_UART_IER);
	serial_out(port, 0, SW_UART_IER);

	uart_console_write(port, s, count, sw_console_putchar);
	wait_for_xmitr(sw_uport);
	serial_out(port, ier, SW_UART_IER);
	if (locked)
		spin_unlock(&port->lock);
	local_irq_restore(flags);
}

static int __init sw_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	struct sw_uart_port *sw_uport;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (unlikely(co->index >= SW_UART_NR || co->index < 0))
		return -ENXIO;

	port = &sw_uart_port[co->index].port;
	sw_uport = UART_TO_SPORT(port);
	if (!port->iobase && !port->membase)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	SERIAL_MSG("console setup baud %d parity %c bits %d, flow %c\n",
			baud, parity, bits, flow);
	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver sw_uart_driver;
static struct console sw_console = {
	.name = "ttyS",
	.write = sw_console_write,
	.device = uart_console_device,
	.setup = sw_console_setup,
	.flags = CON_PRINTBUFFER | CON_ANYTIME,
	.index = -1,
	.data = &sw_uart_driver,
};

#define SW_CONSOLE	(&sw_console)
#else
#define SW_CONSOLE	NULL
#endif

static struct uart_driver sw_uart_driver = {
	.owner = THIS_MODULE,
	.driver_name = "sw_serial",
	.dev_name = "ttyS",
	.nr = SW_UART_NR,
	.cons = SW_CONSOLE,
};

static inline bool sw_is_console_port(struct uart_port *port)
{
	return port->cons && port->cons->index == port->line;
}

static int sw_uart_request_resource(struct sw_uart_port* sw_uport)
{
	struct uart_port *port = &sw_uport->port;
	char clk_name[16] = {0};
	int ret = -1;
	struct gpio_config *gpio;
	char* ioname[8] = {"tx", "rx", "cts", "rts",
		"dtr", "dsr", "dcd", "ring", };
	u32 i;

	SERIAL_DBG("get system resource(clk & IO)\n");
	/* request io but don't configure them */
	for (i=0; i<sw_uport->pdata->io_num; i++) {
		gpio = &sw_uport->pdata->uart_io[i];
		ret = gpio_request(gpio->gpio, ioname[i]);
		if (unlikely(ret)) {
			SERIAL_MSG("uart%d request %s failed\n", sw_uport->id, ioname[i]);
			goto fail;
		}
	}

	/* pclk */
	sprintf(clk_name, "apb_uart%d", sw_uport->id);
	sw_uport->pclk = clk_get(port->dev, clk_name);
	if (IS_ERR(sw_uport->pclk)) {
		ret = PTR_ERR(sw_uport->pclk);
		SERIAL_MSG("uart%d get pclk failed\n", sw_uport->id);
		goto free_gpio;
	}
	clk_disable(sw_uport->pclk);
	sprintf(clk_name, "uart%d", sw_uport->id);
	sw_uport->mclk = clk_get(port->dev, clk_name);
	if (IS_ERR(sw_uport->mclk)) {
		ret = PTR_ERR(sw_uport->mclk);
		SERIAL_MSG("uart%d get mclk failed\n", sw_uport->id);
		goto free_pclk;
	}

	#ifdef CONFIG_SW_UART_DUMP_DATA
	sw_uport->dump_buff = (char*)kmalloc(MAX_DUMP_SIZE, GFP_KERNEL);
	if (!sw_uport->dump_buff) {
		SERIAL_MSG("uart%d fail to alloc dump buffer\n", sw_uport->id);
	}
	#endif

	return 0;
free_gpio:
	for (i=0; i<sw_uport->pdata->io_num; i++) {
		gpio = &sw_uport->pdata->uart_io[i];
		gpio_free(gpio->gpio);
	}
free_pclk:
	clk_put(sw_uport->pclk);
fail:
	return ret;
}

static int sw_uart_release_resource(struct sw_uart_port* sw_uport)
{
	struct gpio_config *gpio;
	u32 i;

	SERIAL_DBG("put system resource(clk & IO)\n");

	#ifdef CONFIG_SW_UART_DUMP_DATA
	kfree(sw_uport->dump_buff);
	sw_uport->dump_buff = NULL;
	sw_uport->dump_len = 0;
	#endif

	/* release pclk */
	clk_disable(sw_uport->pclk);
	clk_put(sw_uport->pclk);

	/* request io but don't configure them */
	for (i=0; i<sw_uport->pdata->io_num; i++) {
		gpio = &sw_uport->pdata->uart_io[i];
		gpio_free(gpio->gpio);
	}

	return 0;
}

#ifdef CONFIG_PROC_FS
static int sw_uart_proc_info(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	char *p = page;
	struct sw_uart_port *sw_uport = (struct sw_uart_port *)data;
	u32 dl = (u32)sw_uport->dlh << 8 | (u32)sw_uport->dll;

	p += sprintf(p, " Uart%d Infomation:\n", sw_uport->id);
	p += sprintf(p, " ier  : 0x%02x\n", sw_uport->ier);
	p += sprintf(p, " lcr  : 0x%02x\n", sw_uport->lcr);
	p += sprintf(p, " mcr  : 0x%02x\n", sw_uport->mcr);
	p += sprintf(p, " fcr  : 0x%02x\n", sw_uport->fcr);
	p += sprintf(p, " dll  : 0x%02x\n", sw_uport->dll);
	p += sprintf(p, " dlh  : 0x%02x\n", sw_uport->dlh);
	p += sprintf(p, " pclk : %d\n", sw_uport->port.uartclk);
	p += sprintf(p, " last baud : %d\n", (sw_uport->port.uartclk>>4)/dl);
	p += sprintf(p, " TxRx Statistics:\n");
	p += sprintf(p, " tx   : %d\n", sw_uport->port.icount.tx);
	p += sprintf(p, " rx   : %d\n", sw_uport->port.icount.rx);
	p += sprintf(p, " pe   : %d\n", sw_uport->port.icount.parity);
	p += sprintf(p, " fe   : %d\n", sw_uport->port.icount.frame);
	p += sprintf(p, " or   : %d\n", sw_uport->port.icount.overrun);

	return p - page;
}

void sw_uart_procfs_attach(struct sw_uart_port *sw_uport)
{
	char proc_root[32] = {0};

	sprintf(proc_root, "driver/uart%d", sw_uport->id);
	sw_uport->proc_root = proc_mkdir(proc_root, NULL);
	if (IS_ERR(sw_uport->proc_root))
		SERIAL_MSG("failed to create \"driver/uart%d\".\n",
				sw_uport->id);

	sw_uport->proc_info = create_proc_read_entry("ctrller_info", 0444,
				sw_uport->proc_root, sw_uart_proc_info, sw_uport);
	if (IS_ERR(sw_uport->proc_info))
		SERIAL_MSG("failed to create \"driver/uart%d/ctrller_info\".\n", sw_uport->id);
}

void sw_uart_procfs_remove(struct sw_uart_port *sw_uport)
{
	char proc_root[32] = {0};

	sprintf(proc_root, "driver/uart%d", sw_uport->id);
	remove_proc_entry("ctrller_info", sw_uport->proc_root);
	remove_proc_entry(proc_root, NULL);
}
#endif

static int __devinit sw_uart_probe(struct platform_device *pdev)
{
	u32 id = pdev->id;
	struct uart_port *port;
	struct sw_uart_port *sw_uport;
	struct clk *apbclk;
	int ret = -1;

	if (unlikely(pdev->id < 0 || pdev->id >= SW_UART_NR))
		return -ENXIO;
	port = &sw_uart_port[id].port;
	port->dev = &pdev->dev;
	sw_uport = UART_TO_SPORT(port);
	sw_uport->id = id;
	sw_uport->ier = 0;
	sw_uport->lcr = 0;
	sw_uport->mcr = 0;
	sw_uport->fcr = 0;
	sw_uport->dll = 0;
	sw_uport->dlh = 0;

	/* request system resource and init them */
	ret = sw_uart_request_resource(sw_uport);
	if (unlikely(ret)) {
		SERIAL_MSG("uart%d error to get resource\n", id);
		return -ENXIO;
	}

	apbclk = clk_get(&pdev->dev, CLK_SYS_APB1);
	if (IS_ERR(apbclk)) {
		SERIAL_MSG("uart%d error to get source clock\n", id);
		return -ENXIO;
	}
	ret = clk_set_parent(sw_uport->mclk, apbclk);
	if (ret) {
		SERIAL_MSG("uart%d set mclk parent error\n", id);
		clk_put(apbclk);
		return -ENXIO;
	}
	port->uartclk = clk_get_rate(apbclk);
	clk_put(apbclk);

	port->type = PORT_SW;
	port->flags = UPF_BOOT_AUTOCONF;
	port->mapbase = sw_uport->pdata->base;
	port->irq = sw_uport->pdata->irq;
	platform_set_drvdata(pdev, port);
#ifdef CONFIG_PROC_FS
	sw_uart_procfs_attach(sw_uport);
#endif
	SERIAL_DBG("add uart%d port, port_type %d, uartclk %d\n",
			id, port->type, port->uartclk);
	return uart_add_one_port(&sw_uart_driver, port);
}

static int __devexit sw_uart_remove(struct platform_device *pdev)
{
	struct sw_uart_port *sw_uport = platform_get_drvdata(pdev);

	SERIAL_DBG("release uart%d port\n", sw_uport->id);
#ifdef CONFIG_PROC_FS
	sw_uart_procfs_remove(sw_uport);
#endif
	sw_uart_release_resource(sw_uport);
	return 0;
}

/* UART power management code */
#ifdef CONFIG_PM_SLEEP
static int sw_uart_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct uart_port *port = &sw_uart_port[pdev->id].port;

	if (port) {
		SERIAL_MSG("uart%d suspend\n", port->line);
		uart_suspend_port(&sw_uart_driver, port);
	}

	return 0;
}

static int sw_uart_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct uart_port *port = &sw_uart_port[pdev->id].port;
	struct sw_uart_port *sw_uport = &sw_uart_port[pdev->id];

	if (port) {
		if (sw_is_console_port(port) && !console_suspend_enabled) {
			sw_uart_reset(sw_uport);
			serial_out(port, sw_uport->fcr, SW_UART_FCR);
			serial_out(port, sw_uport->mcr, SW_UART_MCR);
			serial_out(port, sw_uport->lcr|SW_UART_LCR_DLAB, SW_UART_LCR);
			serial_out(port, sw_uport->dll, SW_UART_DLL);
			serial_out(port, sw_uport->dlh, SW_UART_DLH);
			serial_out(port, sw_uport->lcr, SW_UART_LCR);
			serial_out(port, sw_uport->ier, SW_UART_IER);
		}
		uart_resume_port(&sw_uart_driver, port);
		SERIAL_MSG("uart%d resume\n", port->line);
	}

	return 0;
}

static const struct dev_pm_ops sw_uart_pm_ops = {
	.suspend = sw_uart_suspend,
	.resume = sw_uart_resume,
};
#define SERIAL_SW_PM_OPS	(&sw_uart_pm_ops)

#else /* !CONFIG_PM_SLEEP */

#define SERIAL_SW_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver sw_uport_platform_driver = {
	.probe = sw_uart_probe,
	.remove = __devexit_p(sw_uart_remove),
	.driver.name = "sw_serial",
	.driver.pm = SERIAL_SW_PM_OPS,
	.driver.owner = THIS_MODULE,
};

static int sw_uart_get_devinfo(void)
{
	u32 i, j;
	char uart_para[16] = {0};
	char* ioname[8] = {"uart_tx", "uart_rx", "uart_cts", "uart_rts",
		"uart_dtr", "uart_dsr", "uart_dcd", "uart_ring", };
	struct sw_uart_pdata *pdata;
	script_item_u val;
	script_item_value_type_e type;

	for (i=0; i<SW_UART_NR; i++) {
		pdata = &sw_uport_pdata[i];
		sprintf(uart_para, "uart_para%d", i);
		/* get used information */
		type = script_get_item(uart_para, "uart_used", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			SERIAL_MSG("get uart%d's usedcfg failed\n", i);
			goto fail;
		}
		pdata->used = val.val;
		if (pdata->used) {
			/* get type information */
			type = script_get_item(uart_para, "uart_type", &val);
			if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
				SERIAL_MSG("get uart%d's type failed\n", i);
				goto fail;
			}
			if (val.val > pdata->max_ios) {
				SERIAL_MSG("io type error: (%d > max_io_num %d)\n", val.val, pdata->max_ios);
				goto fail;
			}
			pdata->io_num = val.val;
			/* get io */
			for (j=0; j<pdata->io_num; j++) {
				type = script_get_item(uart_para, ioname[j], &val);
				if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
					SERIAL_MSG("get uart%d's IO(uart_tx) failed\n", i);
					goto fail;
				}
				pdata->uart_io[j] = val.gpio;
			}
		}
	}

	return 0;
fail:
	SERIAL_MSG("get uart configuration failed\n");
	return -1;
}

static int __init sw_uart_init(void)
{
	int ret;
	u32 i;
	struct sw_uart_pdata *pdata;

	SERIAL_MSG("driver initializied\n");
	ret = sw_uart_get_devinfo();
	if (unlikely(ret))
		return ret;

	ret = uart_register_driver(&sw_uart_driver);
	if (unlikely(ret)) {
		SERIAL_MSG("driver initializied\n");
		return ret;
	}

	for (i=0; i<SW_UART_NR; i++) {
		pdata = &sw_uport_pdata[i];
		if (pdata->used)
			platform_device_register(&sw_uport_device[i]);
	}

	return platform_driver_register(&sw_uport_platform_driver);
}

static void __exit sw_uart_exit(void)
{
	SERIAL_MSG("driver exit\n");
#ifdef CONFIG_SERIAL_SW_CONSOLE
	unregister_console(&sw_console);
#endif
	platform_driver_unregister(&sw_uport_platform_driver);
	uart_unregister_driver(&sw_uart_driver);
}

module_init(sw_uart_init);
module_exit(sw_uart_exit);

MODULE_AUTHOR("Aaron<leafy.myeh@allwinnertech.com>");
MODULE_DESCRIPTION("Driver for SW serial device");
MODULE_LICENSE("GPL");
