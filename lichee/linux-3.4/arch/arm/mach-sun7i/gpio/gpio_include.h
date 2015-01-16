/*
 * arch/arm/mach-sun7i/gpio/gpio_includes.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 * James Deng <csjamesdeng@reuuimllatech.com>
 *
 * sun7i gpio header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __GPIO_INCLUDES_H
#define __GPIO_INCLUDES_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/pm.h>
#include <asm-generic/gpio.h>
#include <linux/mfd/axp-mfd.h>

#include <mach/memory.h>
#include <mach/platform.h>
#include <mach/gpio.h>
#include <mach/clock.h>

#include "gpio_common.h"
#include "gpio_eint.h"
#include "gpio_init.h"
#include "gpio_multi_func.h"
#include "gpio_base.h"

#endif  /* __GPIO_INCLUDES_H */

