/*
 * arch/arm/mach-sun7i/gpio/gpio_multi_func.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 * James Deng <csjamesdeng@reuuimllatech.com>
 *
 * sun7i gpio driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "gpio_include.h"

#ifdef CONFIG_AW_AXP20
extern int axp_gpio_set_io(int gpio, int io_state);
extern int axp_gpio_get_io(int gpio, int *io_state);
extern int axp_gpio_set_value(int gpio, int value);
extern int axp_gpio_get_value(int gpio, int *value);
#endif

/**
 * gpio_set_cfg - set multi sel for the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 * @val:    multi sel to set
 *
 * Returns 0.
 */
u32 gpio_set_cfg(struct aw_gpio_chip *pchip, u32 offset, u32 val)
{
    u32 ureg_off = 0, ubits_off = 0;

#ifdef DBG_GPIO
    if (val & ~((1 << PIO_BITS_WIDTH_CFG) - 1)) {
        PIO_INF("%s: maybe error, val %d\n", __func__, val);
    }
#endif /* DBG_GPIO */

    ureg_off = ((offset << 2) >> 5) << 2;   /* ureg_off = ((offset * 4) / 32) * 4 */
    ubits_off = (offset << 2) % 32;     /* ubits_off = (offset * 4) % 32 */

    PIO_DBG("%s: write cfg reg 0x%08x, bits_off %d, width %d, cfg_val %d\n", __func__,
            ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG, val);

    PIO_WRITE_BITS(ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG, val);
    return 0;
}

/**
 * gpio_get_cfg - get multi sel for the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 *
 * Returns the multi sel value for the gpio
 */
u32 gpio_get_cfg(struct aw_gpio_chip *pchip, u32 offset)
{
    u32 ureg_off = 0, ubits_off = 0;

    ureg_off = ((offset << 2) >> 5) << 2;   /* ureg_off = ((offset * 4) / 32) * 4 */
    ubits_off = (offset << 2) % 32;     /* ubits_off = (offset * 4) % 32 */

    PIO_DBG("%s: read cfg reg 0x%08x, bits_off %d, width %d, ret %d\n", __func__,
            ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG,
            PIO_READ_BITS(ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG));

    return PIO_READ_BITS(ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG);
}

/**
 * gpio_set_pull - set pull state for the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 * @val:    pull value to set
 *
 * Returns 0.
 */
u32 gpio_set_pull(struct aw_gpio_chip *pchip, u32 offset, u32 val)
{
    u32 ureg_off = 0, ubits_off = 0;

#ifdef DBG_GPIO
    if (val & ~((1 << PIO_BITS_WIDTH_PULL) - 1)) {
        PIO_INF("%s: maybe error, val %d\n", __func__, val);
    }
#endif /* DBG_GPIO */

    ureg_off = PIO_OFF_REG_PULL + (((offset << 1) >> 5) << 2);  /* ureg_off = ((offset * 2) / 32) * 4 */
    ubits_off = (offset << 1) % 32;                 /* ubits_off = (offset * 2) % 32 */

    PIO_DBG("%s: write pull reg 0x%08x, bits_off %d, width %d, pul_val %d\n", __func__,
            ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL, val);

    PIO_WRITE_BITS(ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL, val);
    return 0;
}

/**
 * gpio_get_pull - get pull state for the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 *
 * Returns the pull value for the gpio
 */
u32 gpio_get_pull(struct aw_gpio_chip *pchip, u32 offset)
{
    u32 ureg_off = 0, ubits_off = 0;

    ureg_off = PIO_OFF_REG_PULL + (((offset << 1) >> 5) << 2);  /* ureg_off = ((offset * 2) / 32) * 4 */
    ubits_off = (offset << 1) % 32;                 /* ubits_off = (offset * 2) % 32 */

    PIO_DBG("%s: read pul reg 0x%08x, bits_off %d, width %d, ret %d\n", __func__,
            ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL,
            PIO_READ_BITS(ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL));

    return PIO_READ_BITS(ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL);
}

/**
 * gpio_set_drvlevel - set driver level for the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 * @val:    driver level value to set
 *
 * Returns 0.
 */
u32 gpio_set_drvlevel(struct aw_gpio_chip *pchip, u32 offset, u32 val)
{
    u32 ureg_off = 0, ubits_off = 0;

#ifdef DBG_GPIO
    if (val & ~((1 << PIO_BITS_WIDTH_DRVLVL) - 1)) {
        PIO_INF("%s: maybe error, val %d\n", __func__, val);
    }
#endif /* DBG_GPIO */

    ureg_off = PIO_OFF_REG_DRVLVL + (((offset << 1) >> 5) << 2);    /* ureg_off = ((offset * 2) / 32) * 4 */
    ubits_off = (offset << 1) % 32;                 /* ubits_off = (offset * 2) % 32 */

    PIO_DBG("%s: write drvlevel reg 0x%08x, bits_off %d, width %d, drvlvl_val %d\n", __func__,
            ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL, val);

    PIO_WRITE_BITS(ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL, val);
    return 0;
}

/**
 * gpio_get_drvlevel - get driver level for the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 *
 * Returns the driver level value for the gpio
 */
u32 gpio_get_drvlevel(struct aw_gpio_chip *pchip, u32 offset)
{
    u32 ureg_off = 0, ubits_off = 0;

    ureg_off = PIO_OFF_REG_DRVLVL + (((offset << 1) >> 5) << 2);    /* ureg_off = ((offset * 2) / 32) * 4 */
    ubits_off = (offset << 1) % 32;                 /* ubits_off = (offset * 2) % 32 */

    PIO_DBG("%s: read drvlevel reg 0x%08x, bits_off %d, width %d, ret %d\n", __func__,
            ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL,
            PIO_READ_BITS(ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL));

    return PIO_READ_BITS(ureg_off + (u32)pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL);
}

#ifdef DBG_GPIO
/**
 * is_gpio_requested - check if gpio has been requested by gpio_request
 * @gpio:   the global gpio index
 *
 * Returns true if already requested, false otherwise.
 */
bool is_gpio_requested(u32 gpio)
{
    struct gpio_chip *pchip = NULL;

    pchip = to_gpiochip(gpio);
    if (!pchip || !gpiochip_is_requested(pchip, gpio - pchip->base))
        return false;
    else
        return true;
}
#endif /* DBG_GPIO */

/**
 * sw_gpio_setcfg - set multi sel for the gpio
 * @gpio:   the global gpio index
 * @val:    multi sel to set
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_setcfg(u32 gpio, u32 val)
{
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg, "cfg", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg->set_cfg, "set_cfg", (u32)-1);

    offset = gpio - pchip->chip.base;
    PIO_CHIP_LOCK(&pchip->lock, flags);
    WARN_ON (0 != pchip->cfg->set_cfg(pchip, offset, val));
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return 0;
}
EXPORT_SYMBOL(sw_gpio_setcfg);

/**
 * sw_gpio_getcfg - get multi sel for the gpio
 * @gpio:   the global gpio index
 *
 * Returns the multi sel value for the gpio if success, GPIO_CFG_INVALID otherwise.
 */
u32 sw_gpio_getcfg(u32 gpio)
{
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", GPIO_CFG_INVALID);
    PIO_POINTER_CHECK_NULL(pchip->cfg, "cfg", GPIO_CFG_INVALID);
    PIO_POINTER_CHECK_NULL(pchip->cfg->get_cfg, "get_cfg", GPIO_CFG_INVALID);

    offset = gpio - pchip->chip.base;
    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg->get_cfg(pchip, offset);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_getcfg);

/**
 * sw_gpio_setpull - set pull state for the gpio
 * @gpio:   the global gpio index
 * @val:    pull value to set
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_setpull(u32 gpio, u32 val)
{
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    if (GPIO_PULL_DEFAULT == val) {
        PIO_DBG("%s: set gpio %d pull state to default omitting\n", __func__, gpio);
        return 0;
    }

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg, "cfg", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg->set_pull, "set_pull", (u32)-1);

    offset = gpio - pchip->chip.base;
    PIO_CHIP_LOCK(&pchip->lock, flags);
    WARN_ON(0 != pchip->cfg->set_pull(pchip, offset, val));
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return 0;
}
EXPORT_SYMBOL(sw_gpio_setpull);

/**
 * sw_gpio_getpull - get pull state for the gpio
 * @gpio:   the global gpio index
 *
 * Returns the pull value for the gpio if success, GPIO_PULL_INVALID otherwise.
 */
u32 sw_gpio_getpull(u32 gpio)
{
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", GPIO_PULL_INVALID);
    PIO_POINTER_CHECK_NULL(pchip->cfg, "cfg", GPIO_PULL_INVALID);
    PIO_POINTER_CHECK_NULL(pchip->cfg->get_pull, "get_pull", GPIO_PULL_INVALID);

    offset = gpio - pchip->chip.base;
    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg->get_pull(pchip, offset);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_getpull);

/**
 * sw_gpio_setdrvlevel - set driver level for the gpio
 * @gpio:   the global gpio index
 * @val:    driver level to set
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_setdrvlevel(u32 gpio, u32 val)
{
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    if (GPIO_DRVLVL_DEFAULT == val) {
        PIO_DBG("%s: set gpio %d driver level to default omitting\n", __func__, gpio);
        return 0;
    }

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg, "cfg", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg->set_drvlevel, "set_drvlevel", (u32)-1);

    offset = gpio - pchip->chip.base;
    PIO_CHIP_LOCK(&pchip->lock, flags);
    WARN_ON(0 != pchip->cfg->set_drvlevel(pchip, offset, val));
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return 0;
}
EXPORT_SYMBOL(sw_gpio_setdrvlevel);

/**
 * sw_gpio_getdrvlevel - get driver level for the gpio
 * @gpio:   the global gpio index
 *
 * Returns the driver level for the gpio if success, GPIO_DRVLVL_INVALID otherwise.
 */
u32 sw_gpio_getdrvlevel(u32 gpio)
{
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", GPIO_DRVLVL_INVALID);
    PIO_POINTER_CHECK_NULL(pchip->cfg, "cfg", GPIO_DRVLVL_INVALID);
    PIO_POINTER_CHECK_NULL(pchip->cfg->get_drvlevel, "get_drvlevel", GPIO_DRVLVL_INVALID);

    offset = gpio - pchip->chip.base;
    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg->get_drvlevel(pchip, offset);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_getdrvlevel);

/**
 * sw_gpio_setall_range - config a group of pin, include multi sel, pull, driverlevel
 * @pcfg:   the config value group
 * @cfg_num:    gpio number to config, also pcfg's member number
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_setall_range(struct gpio_config *pcfg, u32 cfg_num)
{
    u32 i = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    PIO_POINTER_CHECK_NULL(pcfg, "pcfg", (u32)-1);
    if (0 == cfg_num) {
        PIO_ERR("%s: cfg_num is zero, not allowed\n", __func__);
        return (u32)-1;
    }

    for (i = 0; i < cfg_num; i++, pcfg++) {
#ifdef CONFIG_AW_AXP20
        if (pcfg->gpio >= AXP_NR_BASE && pcfg->gpio < AXP_NR_BASE + AXP_NR) {
            if (0 == pcfg->mul_sel) {
                WARN(0 != gpio_direction_input(pcfg->gpio), "%s: set axp pin(%d) input failed\n",
                        __func__, pcfg->gpio);
            } else if (1 == pcfg->mul_sel) {
                WARN(0 != gpio_direction_output(pcfg->gpio, (pcfg->data ? 1 : 0)),
                        "%s: set axp pin(%d) output(%d) failed\n", __func__, pcfg->gpio, pcfg->data);
            } else {
                PIO_ERR("%s: apx pin(%d) but not input/output\n", __func__, pcfg->gpio);
            }

            continue;
        }
#endif

        pchip = gpio_to_aw_gpiochip(pcfg->gpio);
        if (!pchip || !pchip->cfg || !pchip->cfg->set_cfg ||
            !pchip->cfg->set_pull || !pchip->cfg->set_drvlevel) {
            PIO_ERR("%s: invalid handle NULL occur\n", __func__);
            continue;
        }

        offset = pcfg->gpio - pchip->chip.base;
        PIO_CHIP_LOCK(&pchip->lock, flags);
        WARN_ON(0 != pchip->cfg->set_cfg(pchip, offset, pcfg->mul_sel));
        if (pcfg->pull != GPIO_PULL_DEFAULT) {
            WARN_ON(0 != pchip->cfg->set_pull(pchip, offset, pcfg->pull));
        }
        if (pcfg->drv_level != GPIO_DRVLVL_DEFAULT) {
            WARN_ON(0 != pchip->cfg->set_drvlevel(pchip, offset, pcfg->drv_level));
        }
        PIO_CHIP_UNLOCK(&pchip->lock, flags);

        if (GPIO_CFG_OUTPUT == pcfg->mul_sel &&
            pcfg->data != GPIO_DATA_DEFAULT) {
            __gpio_set_value(pcfg->gpio, (pcfg->data ? 1 : 0));
        }
    }

    return 0;
}
EXPORT_SYMBOL(sw_gpio_setall_range);

/**
 * sw_gpio_getall_range - get the config state for a group of pin,
 *          include multi sel, pull, driverlevel
 * @pcfg:   store the config information for pins
 * @cfg_num:    number of the pins
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_getall_range(struct gpio_config *pcfg, u32 cfg_num)
{
    u32 i = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    PIO_POINTER_CHECK_NULL(pcfg, "pcfg", (u32)-1);
    if (0 == cfg_num) {
        PIO_ERR("%s: cfg_num is zero, not allowed\n", __func__);
        return (u32)-1;
    }

    for (i = 0; i < cfg_num; i++, pcfg++) {
#ifdef CONFIG_AW_AXP20
        if (pcfg->gpio >= AXP_NR_BASE && pcfg->gpio < AXP_NR_BASE + AXP_NR) {
            int io_status = -1;

            PIO_INF("%s: can only get axp pin(%d) data value\n",
                    __func__, pcfg->gpio);

            /* get mul sel: input/output */
            offset = pcfg->gpio - AXP_NR_BASE;
            if (0 == axp_gpio_get_io(offset, &io_status)) {
                pcfg->mul_sel = io_status;
            } else {
                PIO_ERR("%s: get axp pin(%d) io status failed\n", 
                        __func__, pcfg->gpio);
            }

            /* get data */
            pcfg->data = __gpio_get_value(pcfg->gpio);

            continue;
        }
#endif

        pchip = gpio_to_aw_gpiochip(pcfg->gpio);
        if (!pchip || !pchip->cfg || !pchip->cfg->get_cfg ||
            !pchip->cfg->get_pull || !pchip->cfg->get_drvlevel) {
            PIO_ERR("%s: invalid handle NULL occur\n", __func__);
            continue;
        }

        offset = pcfg->gpio - pchip->chip.base;
        PIO_CHIP_LOCK(&pchip->lock, flags);
        pcfg->mul_sel = pchip->cfg->get_cfg(pchip, offset);
        pcfg->pull = pchip->cfg->get_pull(pchip, offset);
        pcfg->drv_level = pchip->cfg->get_drvlevel(pchip, offset);
        PIO_CHIP_UNLOCK(&pchip->lock, flags);

        if (GPIO_CFG_OUTPUT == pcfg->mul_sel ||
            GPIO_CFG_INPUT == pcfg->mul_sel) {
            pcfg->data = __gpio_get_value(pcfg->gpio);
        }
    }

    return 0;
}
EXPORT_SYMBOL(sw_gpio_getall_range);

/**
 * sw_gpio_dump_config - dump config info for a group of pins
 * @pcfg:   store the config information for pins
 * @cfg_num:    number of the pins
 */
void sw_gpio_dump_config(struct gpio_config *pcfg, u32 cfg_num)
{
    u32 i = 0;

    printk("++++++++++%s++++++++++\n", __func__);
    printk("  port    mul_sel    pull    drvlevl\n");
    for (i = 0; i < cfg_num; i++, pcfg++) {
        printk("  %4d    %7d    %4d    %7d\n", pcfg->gpio, pcfg->mul_sel, pcfg->pull, pcfg->drv_level);
    }
    printk("----------%s----------\n", __func__);
}
EXPORT_SYMBOL(sw_gpio_dump_config);

/**
 * sw_gpio_suspend - save somethig for gpio before enter sleep
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_suspend(void)
{
    u32 uret = 0;

    printk("%s: TODO\n", __func__);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_suspend);

/**
 * sw_gpio_suspend - restore somethig for gpio after wake up
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_resume(void)
{
    u32 uret = 0;

    printk("%s: TODO\n", __func__);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_resume);


