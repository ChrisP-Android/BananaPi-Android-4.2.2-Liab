/*
 * arch/arm/mach-sun7i/gpio/gpio_multi_func.h
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

#ifndef __GPIO_MULTI_FUNC_H
#define __GPIO_MULTI_FUNC_H

#ifdef DBG_GPIO
bool is_gpio_requested(u32 gpio);
#endif /* DBG_GPIO */

u32 gpio_set_cfg(struct aw_gpio_chip *pchip, u32 offset, u32 val);
u32 gpio_get_cfg(struct aw_gpio_chip *pchip, u32 offset);
u32 gpio_set_pull(struct aw_gpio_chip *pchip, u32 offset, u32 val);
u32 gpio_get_pull(struct aw_gpio_chip *pchip, u32 offset);
u32 gpio_set_drvlevel(struct aw_gpio_chip *pchip, u32 offset, u32 val);
u32 gpio_get_drvlevel(struct aw_gpio_chip *pchip, u32 offset);

#endif /* __GPIO_MULTI_FUNC_H */

