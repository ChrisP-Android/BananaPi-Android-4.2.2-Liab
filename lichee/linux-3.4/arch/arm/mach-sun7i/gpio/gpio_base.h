/*
 * arch/arm/mach-sun7i/gpio/gpio_base.h
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

#ifndef __GPIO_BASE_H
#define __GPIO_BASE_H

/**
 * gpiochip_match - match function to find gpio_chip
 * @chip:   gpio_chip struct
 * @data:   data for match
 *
 * Returns 1 if match, 0 if not match
 */
static inline int gpiochip_match(struct gpio_chip *chip, const void *data)
{
    u32     num = 0;

    if (!chip || !data) {
        return 0;
    }

    num = *(u32 *)data;
    if (num >= chip->base && num < chip->base + chip->ngpio) {
        return 1;
    }

    return 0;
}

/**
 * to_gpiochip - get gpio_chip for the gpio index
 * @gpio:   the global gpio index
 *
 * Returns gpio_chip pointer if success, NULL otherwise.
 */
static inline struct gpio_chip *to_gpiochip(u32 gpio)
{
    u32     num = gpio;
    struct gpio_chip *pchip = NULL;

    pchip = gpiochip_find((void *)&num, gpiochip_match);
    if (!pchip) {
        return NULL;
    }

    return pchip;
}

/**
 * to_aw_gpiochip - get aw_gpio_chip from gpio_chip poniter
 * @gpc:    the gpio_chip poniter
 *
 * Returns aw_gpio_chip pointer for the gpio_chip
 */
static inline struct aw_gpio_chip *to_aw_gpiochip(struct gpio_chip *gpc)
{
    return container_of(gpc, struct aw_gpio_chip, chip);
}

/**
 * gpio_to_aw_gpiochip - get aw_gpio_chip from gpio index
 * @gpio:   the global gpio index
 *
 * Returns aw_gpio_chip pointer for the gpio if success, NULL ttherwise.
 */
static inline struct aw_gpio_chip *gpio_to_aw_gpiochip(u32 gpio)
{
    struct gpio_chip *pchip = NULL;

    if (GPIO_INDEX_INVALID == gpio) {
        return NULL;
    }

    /* axp pin can only use linux stardard api */
#ifdef CONFIG_AW_AXP20
    if (gpio >= AXP_NR_BASE && gpio < AXP_NR_BASE + AXP_NR) {
        return NULL;
    }
#endif

    pchip = to_gpiochip(gpio);
    if (!pchip) {
        return NULL;
    }

    return to_aw_gpiochip(pchip);
}

int __pio_to_irq(struct gpio_chip *chip, unsigned offset);
int aw_gpiochip_add(struct gpio_chip *chip);

#endif /* __GPIO_BASE_H */

