/*
 * arch/arm/mach-sun7i/gpio/gpio_eint.c
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

/**
 * is_gpio_canbe_eint - check if gpio canbe configured as eint
 * @gpio:   the global gpio index
 *
 * return true if the gpio canbe configured as eint, false otherwise.
 */
bool is_gpio_canbe_eint(u32 gpio)
{
    int i = 0;
    u32 gpio_eint_group[][2] = {
        {GPIOH(0),  GPIOH(21)},
        {GPIOI(10), GPIOI(19)}
    };

    for (i = 0; i < ARRAY_SIZE(gpio_eint_group); i++) {
        if (gpio >= gpio_eint_group[i][0] &&
            gpio <= gpio_eint_group[i][1]) {
            return true;
        }
    }

    return false;
}

/**
 * __is_gpio_i - check if gpio is in GPIO I.
 * @gpio:   the global gpio index
 *
 * return true if the gpio is in GPIO I, false otherwise.
 */
u32 inline __is_gpio_i(u32 gpio)
{
    return (gpio >= GPIOI(0) && gpio <= GPIOI(21));
}

/**
 * gpio_eint_set_trig - set trig type of the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 * @trig_val:   the trig type to set
 *
 * Returns 0.
 */
u32 gpio_eint_set_trig(struct aw_gpio_chip *pchip, u32 offset, enum gpio_eint_trigtype trig_val)
{
    u32 reg_off, bits_off;

    reg_off = ((offset << 2) >> 5) << 2; /* (offset * 4) / 32 * 4 */
    bits_off = (offset << 2) & ((1 << 5) - 1); /* (offset * 4) % 32 */

#ifdef DBG_GPIO
    WARN_ON(trig_val >= TRIG_INALID);
    PIO_DBG("%s: chip 0x%08x, offset %d, write reg 0x%08x, bits off %d, val %d\n", __func__,
            (u32)pchip, offset, (u32)pchip->vbase_eint + reg_off, bits_off, (u32)trig_val);
#endif /* DBG_GPIO */

    PIO_WRITE_BITS((u32)pchip->vbase_eint + reg_off, bits_off, 4, (u32)trig_val);
    return 0;
}

/**
 * gpio_eint_get_trig - get trig type of the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 * @pval:   the trig type got
 *
 * Returns 0.
 */
u32 gpio_eint_get_trig(struct aw_gpio_chip *pchip, u32 offset, enum gpio_eint_trigtype *pval)
{
    u32 reg_off, bits_off;

    reg_off = ((offset << 2) >> 5) << 2; /* (offset * 4) / 32 * 4 */
    bits_off = (offset << 2) & ((1 << 5) - 1); /* (offset * 4) % 32 */

    *pval = (enum gpio_eint_trigtype)PIO_READ_BITS((u32)pchip->vbase_eint + reg_off, bits_off, 4);
#ifdef DBG_GPIO
    PIO_DBG("%s: chip 0x%08x, offset %d, read reg 0x%08x - 0x%08x, bits off %d, ret val %d\n", __func__,
            (u32)pchip, offset, (u32)pchip->vbase_eint + reg_off,
            PIO_READ_REG((u32)pchip->vbase_eint + reg_off), bits_off, (u32)*pval);
    WARN_ON(*pval >= TRIG_INALID);
#endif /* DBG_GPIO */

    return 0;
}

/**
 * gpio_eint_set_enable - enable/disable the gpio eint
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 * @enable: 1 - enable the eint, 0 - disable the eint.
 *
 * Returns 0.
 */
u32 gpio_eint_set_enable(struct aw_gpio_chip *pchip, u32 offset, u32 enable)
{
#ifdef DBG_GPIO
    PIO_DBG("%s: chip 0x%08x, offset %d, enable %d, write reg 0x%08x\n", __func__,
            (u32)pchip, offset, enable, (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL);
#endif /* DBG_GPIO */

    if (0 != enable)
        PIO_SET_BIT((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL, offset);
    else
        PIO_CLR_BIT((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL, offset);
    return 0;
}

/**
 * gpio_eint_get_enable - get the gpio eint's enable/disable satus
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 * @penable:    status got, 1 - the eint is enabled, 0 - disabled
 *
 * Returns 0.
 */
u32 gpio_eint_get_enable(struct aw_gpio_chip *pchip, u32 offset, u32 *penable)
{
    WARN_ON(NULL == penable);
    *penable = PIO_READ_BITS((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL, offset, 1);
#ifdef DBG_GPIO
    PIO_DBG("%s: chip 0x%08x, offset %d, read reg 0x%08x - 0x%08x, penable 0x%08x, *penable %d\n", __func__,
            (u32)pchip, offset, (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL,
            PIO_READ_REG((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL), (u32)penable, *penable);
#endif /* DBG_GPIO */

    return 0;
}

/**
 * gpio_eint_get_irqpd_sta - get the irqpend status of the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 *
 * Returns the irqpend status of the gpio. 1 - irq pend, 0 - no irq pend.
 */
u32 gpio_eint_get_irqpd_sta(struct aw_gpio_chip *pchip, u32 offset)
{
    u32 uret = 0;

    uret = PIO_READ_BITS((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_STATUS, offset, 1);
#ifdef DBG_GPIO
    PIO_DBG("%s: chip 0x%08x, offset %d, read reg 0x%08x - 0x%08x, ret %d\n", __func__,
            (u32)pchip, offset, (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_STATUS,
            PIO_READ_REG((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_STATUS), uret);
#endif /* DBG_GPIO */

    return uret;
}

/**
 * gpio_eint_clr_irqpd_sta - clr the irqpend status of the gpio
 * @chip:   aw_gpio_chip struct for the gpio
 * @offset: offset from gpio_chip->base
 *
 * Returns 0.
 */
u32 gpio_eint_clr_irqpd_sta(struct aw_gpio_chip *pchip, u32 offset)
{
    if (1 == PIO_READ_BITS((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_STATUS, offset, 1)) {
        PIO_WRITE_REG((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_STATUS, 1 << offset);
    }

    return 0;
}

/**
 * gpio_eint_set_debounce - set the debounce of the gpio group
 * @chip:   aw_gpio_chip struct for the gpio
 * @val:    debounce to set.
 *
 * for eint group, not for single port
 *
 * Returns 0.
 */
u32 gpio_eint_set_debounce(struct aw_gpio_chip *pchip, struct gpio_eint_debounce val)
{
    u32 utemp = 0;

    utemp = (val.clk_sel & 1) | ((val.clk_pre_scl & 0b111) << 4);

#ifdef DBG_GPIO
    PIO_DBG("%s: clk_sel %d, clk_pre_scl %d, write 0x%08x to reg 0x%08x\n", __func__, val.clk_sel,
            val.clk_pre_scl, utemp, (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_DEBOUNCE);
#endif /* DBG_GPIO */

    PIO_WRITE_REG((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_DEBOUNCE, utemp);
    return 0;
}

/**
 * gpio_eint_get_debounce - get the debounce of the gpio group
 * @chip:   aw_gpio_chip struct for the gpio
 * @val:    debounce got.
 *
 * for eint group, not for single port
 *
 * Returns 0.
 */
u32 gpio_eint_get_debounce(struct aw_gpio_chip *pchip, struct gpio_eint_debounce *pval)
{
    u32     utemp = 0;

    WARN_ON(NULL == pval);
    utemp = PIO_READ_REG((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_DEBOUNCE);
    pval->clk_sel = utemp & 1;
    pval->clk_pre_scl = (utemp >> 4) & 0b111;
#ifdef DBG_GPIO
    PIO_DBG("%s: read from reg 0x%08x - 0x%08x, clk_sel %d, clk_pre_scl %d\n", __func__,
            (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_DEBOUNCE,
            utemp, pval->clk_sel, pval->clk_pre_scl);
#endif /* DBG_GPIO */

    return 0;
}

/**
 * sw_gpio_eint_set_trigtype - set trig type of the gpio
 * @gpio:   the global gpio index
 * @trig_type:  the trig type to set
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_eint_set_trigtype(u32 gpio, enum gpio_eint_trigtype trig_type)
{
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    PIO_DBG("%s: gpio %d, trig_type %d\n", __func__, gpio, (u32)trig_type);

    if (false == is_gpio_canbe_eint(gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return (u32)-1;
    }
    if (trig_type >= TRIG_INALID) {
        PIO_ERR("%s: invalid trigger type\n", __func__);
        return (u32)-1;
    }

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint, "cfg_eint", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint->eint_set_trig, "eint_set_trig", (u32)-1);

    if (__is_gpio_i(gpio))
        offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
    else
        offset = gpio - pchip->chip.base;

    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg_eint->eint_set_trig(pchip, offset, trig_type);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_set_trigtype);

/**
 * sw_gpio_eint_get_trigtype - get trig type of the gpio
 * @gpio:   the global gpio index
 * @pval:   the trig type got
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_eint_get_trigtype(u32 gpio, enum gpio_eint_trigtype *pval)
{
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    if (false == is_gpio_canbe_eint(gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return (u32)-1;
    }
    PIO_POINTER_CHECK_NULL(pval, "pval", (u32)-1);

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint, "cfg_eint", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint->eint_get_trig, "eint_get_trig", (u32)-1);

    if (__is_gpio_i(gpio))
        offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
    else
        offset = gpio - pchip->chip.base;

    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg_eint->eint_get_trig(pchip, offset, pval);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    PIO_DBG("%s: gpio %d, trig_type ret %d\n", __func__, gpio, (u32)*pval);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_get_trigtype);

/**
 * sw_gpio_eint_set_enable - enable/disable the gpio eint
 * @gpio:   the global gpio index
 * @enable: 1 - enable the eint, 0 - disable the eint.
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_eint_set_enable(u32 gpio, u32 enable)
{
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    PIO_DBG("%s: gpio %d, enable %d\n", __func__, gpio, enable);

    if (false == is_gpio_canbe_eint(gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return (u32)-1;
    }

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint, "cfg_eint", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint->eint_set_enable, "eint_set_enable", (u32)-1);

    if (__is_gpio_i(gpio))
        offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
    else
        offset = gpio - pchip->chip.base;

    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg_eint->eint_set_enable(pchip, offset, enable);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_set_enable);

/**
 * sw_gpio_eint_get_enable - get the gpio eint's enable/disable satus
 * @gpio:   the global gpio index
 * @penable:    status got, 1 - the eint is enabled, 0 - disabled
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_eint_get_enable(u32 gpio, u32 *penable)
{
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    if (false == is_gpio_canbe_eint(gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return (u32)-1;
    }
    PIO_POINTER_CHECK_NULL(penable, "penable", (u32)-1);

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint, "cfg_eint", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint->eint_get_enable, "eint_get_enable", (u32)-1);

    if (__is_gpio_i(gpio))
        offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
    else
        offset = gpio - pchip->chip.base;

    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg_eint->eint_get_enable(pchip, offset, penable);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    PIO_DBG("%s: gpio %d, penable ret %d\n", __func__, gpio, *penable);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_get_enable);

/**
 * sw_gpio_eint_get_irqpd_sta - get the irqpend status of the gpio
 * @gpio:   the global gpio index
 *
 * Returns the irqpend status of the gpio. 1 - irq pend, 0 - no irq pend.
 */
u32 sw_gpio_eint_get_irqpd_sta(u32 gpio)
{
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    if (false == is_gpio_canbe_eint(gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return 0;
    }

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", 0);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint, "cfg_eint", 0);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint->eint_get_irqpd_sta, "eint_get_irqpd_sta", 0);

    if (__is_gpio_i(gpio))
        offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
    else
        offset = gpio - pchip->chip.base;

    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg_eint->eint_get_irqpd_sta(pchip, offset);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    PIO_DBG("%s: gpio %d, ret %d\n", __func__, gpio, uret);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_get_irqpd_sta);

/**
 * sw_gpio_eint_clr_irqpd_sta - clr the irqpend status of the gpio
 * @gpio:   the global gpio index
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_eint_clr_irqpd_sta(u32 gpio)
{
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    if (false == is_gpio_canbe_eint(gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return (u32)-1;
    }

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint, "cfg_eint", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint->eint_clr_irqpd_sta, "eint_clr_irqpd_sta", (u32)-1);

    if (__is_gpio_i(gpio))
        offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
    else
        offset = gpio - pchip->chip.base;

    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg_eint->eint_clr_irqpd_sta(pchip, offset);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_clr_irqpd_sta);

/**
 * sw_gpio_eint_get_debounce - get the debounce of the gpio group
 * @gpio:   the global gpio index
 * @pdbc:   debounce got.
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_eint_get_debounce(u32 gpio, struct gpio_eint_debounce *pdbc)
{
    u32 uret = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    if (false == is_gpio_canbe_eint(gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return (u32)-1;
    }
    PIO_POINTER_CHECK_NULL(pdbc, "pdbc", (u32)-1);

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint, "cfg_eint", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint->eint_get_debounce, "eint_get_debounce", (u32)-1);

    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg_eint->eint_get_debounce(pchip, pdbc);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_get_debounce);

/**
 * sw_gpio_eint_set_debounce - set the debounce of the gpio group
 * @gpio:   the global gpio index
 * @dbc:    debounce to set.
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_eint_set_debounce(u32 gpio, struct gpio_eint_debounce dbc)
{
    u32 uret = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    if (false == is_gpio_canbe_eint(gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return (u32)-1;
    }

    pchip = gpio_to_aw_gpiochip(gpio);
    PIO_POINTER_CHECK_NULL(pchip, "pchip", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint, "cfg_eint", (u32)-1);
    PIO_POINTER_CHECK_NULL(pchip->cfg_eint->eint_set_debounce, "eint_set_debounce", (u32)-1);

    PIO_CHIP_LOCK(&pchip->lock, flags);
    uret = pchip->cfg_eint->eint_set_debounce(pchip, dbc);
    PIO_CHIP_UNLOCK(&pchip->lock, flags);

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_set_debounce);

/**
 * sw_gpio_eint_setall_range - config a range of gpio, config mul sel to eint,
 *  set driver level and pull, set the trig mode, and enable eint.
 * @pcfg:   config info to set.
 * @cfg_num:    member cnt of pcfg
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_eint_setall_range(struct gpio_config_eint_all *pcfg, u32 cfg_num)
{
    u32 i = 0;
    u32 uret = 0;
    u32 offset = 0;
    u32 mulsel_eint = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    PIO_POINTER_CHECK_NULL(pcfg, "pcfg", (u32)-1);
    if (0 == cfg_num) {
        PIO_ERR("%s: cfg_num is zero, not allowed\n", __func__);
        return (u32)-1;
    }

    for (i = 0; i < cfg_num; i++, pcfg++) {
        /*  request gpio */
        if (false == is_gpio_canbe_eint(pcfg->gpio)) {
            PIO_ERR("%s: gpio %d can not be eint\n", __func__, pcfg->gpio);
            continue;
        }
        if (pcfg->trig_type >= TRIG_INALID) {
            PIO_ERR("%s: invalid trigger type\n", __func__);
            continue;
        }

        /*  get aw_gpiochip */
        pchip = gpio_to_aw_gpiochip(pcfg->gpio);
        if (!pchip || !pchip->cfg->set_cfg || !pchip->cfg->set_pull ||
            !pchip->cfg->set_drvlevel || !pchip->cfg_eint->eint_set_trig ||
            !pchip->cfg_eint->eint_set_enable || !pchip->cfg_eint->eint_clr_irqpd_sta) {
            PIO_ERR("%s: invalid handle NULL occur\n", __func__);
            continue;
        }

        offset = pcfg->gpio - pchip->chip.base;
        PIO_CHIP_LOCK(&pchip->lock, flags);

        /* set mul sel to eint, and set pull and drvlvl */
        mulsel_eint = GPIO_CFG_EINT;
        WARN_ON(0 != pchip->cfg->set_cfg(pchip, offset, mulsel_eint));
        if (pcfg->pull != GPIO_PULL_DEFAULT) {
            WARN_ON(0 != pchip->cfg->set_pull(pchip, offset, pcfg->pull));
        }
        if (pcfg->drvlvl != GPIO_DRVLVL_DEFAULT) {
            WARN_ON(0 != pchip->cfg->set_drvlevel(pchip, offset, pcfg->drvlvl));
        }

        /* PI10 offset is 22 */
        if (__is_gpio_i(pcfg->gpio)) {
            offset = offset - PI_EINT_OFFSET + PI_EINT_START_INDEX;
        }

        /* set trig type */
        pchip->cfg_eint->eint_set_trig(pchip, offset, pcfg->trig_type);

        /* enable/disable eint */
        pchip->cfg_eint->eint_set_enable(pchip, offset, pcfg->enabled);

        /* clr the irqpd status */
        if (0 != pcfg->irq_pd)
            pchip->cfg_eint->eint_clr_irqpd_sta(pchip, offset);

        PIO_CHIP_UNLOCK(&pchip->lock, flags);
    }

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_setall_range);

/**
 * sw_gpio_eint_getall_range - get a range of gpio's eint info
 * @pcfg:   config info got.
 * @cfg_num:    member cnt of pcfg
 *
 * Returns 0 if sucess, (u32)-1 if failed.
 */
u32 sw_gpio_eint_getall_range(struct gpio_config_eint_all *pcfg, u32 cfg_num)
{
    u32 i = 0;
    u32 uret = 0;
    u32 offset = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    PIO_POINTER_CHECK_NULL(pcfg, "pcfg", (u32)-1);
    if (0 == cfg_num) {
        PIO_ERR("%s: cfg_num is zero, not allowed\n", __func__);
        return (u32)-1;
    }

    for (i = 0; i < cfg_num; i++, pcfg++) {
        if (false == is_gpio_canbe_eint(pcfg->gpio)) {
            PIO_ERR("%s: gpio %d can not be eint\n", __func__, pcfg->gpio);
            continue;
        }

        pchip = gpio_to_aw_gpiochip(pcfg->gpio);
        if (!pchip || !pchip->cfg->get_cfg || !pchip->cfg->get_pull ||
            !pchip->cfg->get_drvlevel || !pchip->cfg_eint->eint_get_trig ||
            !pchip->cfg_eint->eint_get_enable || !pchip->cfg_eint->eint_get_irqpd_sta) {
            PIO_ERR("%s: invalid handle NULL occur\n", __func__);
            continue;
        }

        offset = pcfg->gpio - pchip->chip.base;
        PIO_CHIP_LOCK(&pchip->lock, flags);

        /* verify mul sel to eint, and get pull and drvlvl */
        WARN_ON(GPIO_CFG_EINT != pchip->cfg->get_cfg(pchip, offset));
        pcfg->pull = pchip->cfg->get_pull(pchip, offset);
        pcfg->drvlvl = pchip->cfg->get_drvlevel(pchip, offset);

        /* redirect offset for eint op, for the case that r_pl_5 is PL_EINT0 */
        if (__is_gpio_i(pcfg->gpio)) {
            offset = offset - PI_EINT_OFFSET + PI_EINT_START_INDEX;
        }

        /* get trig type */
        pchip->cfg_eint->eint_get_trig(pchip, offset, &pcfg->trig_type);
        WARN_ON(pcfg->trig_type >= TRIG_INALID);

        /* get enable/disable status */
        pchip->cfg_eint->eint_get_enable(pchip, offset, &pcfg->enabled);

        /* get the irqpd status */
        pcfg->irq_pd = pchip->cfg_eint->eint_get_irqpd_sta(pchip, offset);

        PIO_CHIP_UNLOCK(&pchip->lock, flags);
    }

    return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_getall_range);

/**
 * sw_gpio_eint_dumpall_range - dump a range of gpio's eint config info.
 * @pcfg:   config info to dump.
 * @cfg_num:    member cnt of pcfg
 *
 */
void sw_gpio_eint_dumpall_range(struct gpio_config_eint_all *pcfg, u32 cfg_num)
{
    u32 i = 0;
    unsigned long flags = 0;
    struct aw_gpio_chip *pchip = NULL;

    if (!pcfg) {
        PIO_ERR("%s: pcfg is NULL\n", __func__);
        return;
    }

    printk("++++++++++%s++++++++++\n", __func__);
    printk("  gpio    pull    drvlevl   enabled  irq_pd   trig_type\n");
    for (i = 0; i < cfg_num; i++, pcfg++) {
        pchip = gpio_to_aw_gpiochip(pcfg->gpio);
        if (!pchip) {
            printk("%s: gpio_to_aw_gpiochip(%d) return NULL\n",
                    __func__, pcfg->gpio);
            continue;
        }

        PIO_CHIP_LOCK(&pchip->lock, flags);
        printk("  %4d    %4d    %7d   %7d  %6d   %9d\n", pcfg->gpio, pcfg->pull, pcfg->drvlvl,
                pcfg->enabled, pcfg->irq_pd, pcfg->trig_type);
        PIO_CHIP_UNLOCK(&pchip->lock, flags);
    }
    printk("----------%s----------\n", __func__);
}
EXPORT_SYMBOL(sw_gpio_eint_dumpall_range);

irqreturn_t gpio_irq_hdl(int irq, void *dev)
{
    struct gpio_irq_handle *pdev_id = (struct gpio_irq_handle *)dev;

    if (false == is_gpio_canbe_eint(pdev_id->gpio) || !pdev_id) {
        return IRQ_NONE;
    }

    if (0 == sw_gpio_eint_get_irqpd_sta(pdev_id->gpio)) {
        return IRQ_NONE;
    }

    sw_gpio_eint_clr_irqpd_sta(pdev_id->gpio);
    if (pdev_id->handler) {
        if (pdev_id->handler(pdev_id->parg)) {
            PIO_ERR("%s: handler failed\n", __func__);
        }
    }

    return IRQ_HANDLED;
}

/**
 * sw_gpio_irq_request - request gpio irq.
 * @gpio:   the global gpio index
 * @trig_type:  trig type of gpio eint
 * @handle: irq callback
 * @para:   para of the handle function.
 *
 * Returns the handle if sucess, 0 if failed.
 */
u32 sw_gpio_irq_request(u32 gpio, enum gpio_eint_trigtype trig_type,
                        peint_handle handle, void *para)
{
    int irq_no = 0;
    int irq_ret = 0;
    struct gpio_config_eint_all cfg = {0};
    struct gpio_irq_handle *pdev_id = NULL;

    PIO_DBG("%s: gpio %d, trig %d, handle 0x%08x, para 0x%08x\n", __func__,
            gpio, trig_type, (u32)handle, (u32)para);

    WARN(NULL == handle, "%s: NULL handle\n", __func__);
    if (false == is_gpio_canbe_eint(gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return 0;
    }

    /* config to eint, enable the eint, and set pull, drivel level, trig type */
    cfg.gpio        = gpio;
    cfg.pull        = GPIO_PULL_DEFAULT;
    cfg.drvlvl      = GPIO_DRVLVL_DEFAULT;
    cfg.enabled     = 0;
    cfg.trig_type   = trig_type;
    if (sw_gpio_eint_setall_range(&cfg, 1)) {
        PIO_ERR("%s: gpio %d eint set all range failed\n", __func__, gpio);
        return 0;
    }

    /* request irq */
    pdev_id = (struct gpio_irq_handle *)kmalloc(sizeof(struct gpio_irq_handle), GFP_KERNEL);
    if (NULL == pdev_id) {
        PIO_ERR("%s: alloc memory for gpio irq handle failed\n", __func__);
        return 0;
    }
    pdev_id->gpio    = gpio;
    pdev_id->handler = handle;
    pdev_id->parg    = para;

    irq_no = __gpio_to_irq(gpio);
    PIO_DBG("%s: __gpio_to_irq return %d\n", __func__, irq_no);

    irq_ret = request_irq(irq_no, gpio_irq_hdl, IRQF_DISABLED | IRQF_SHARED, "gpio_irq", (void *)pdev_id);
    if (irq_ret) {
        PIO_ERR("%s: request_irq failed\n", __func__);
        kfree(pdev_id);
        return 0;
    }

    if (sw_gpio_eint_set_enable(gpio, 1)) {
        PIO_ERR("%s: sw_gpio_eint_set_enable failed\n", __func__);
        free_irq(irq_no, (void *)pdev_id);
        kfree(pdev_id);
        return 0;
    }

    return (u32)pdev_id;
}
EXPORT_SYMBOL(sw_gpio_irq_request);

/**
 * sw_gpio_irq_free - free gpio irq.
 * @handle: handle return by sw_gpio_irq_request
 *
 * Returns 0 if sucess, the err line number if failed.
 */
void sw_gpio_irq_free(u32 handle)
{
    u32 gpio = 0;
    int irq_no = 0;
    struct gpio_irq_handle *pdev_id = (struct gpio_irq_handle *)handle;

    PIO_DBG("%s: handle 0x%08x\n", __func__, (u32)handle);

    if (!pdev_id) {
        PIO_ERR("%s: null handle, invalid\n", __func__);
        return;
    }

    if (false == is_gpio_canbe_eint(pdev_id->gpio)) {
        PIO_ERR("%s: gpio %d can not be eint\n", __func__, gpio);
        return;
    }

    /* clear gpio reg */
    gpio = pdev_id->gpio;
    sw_gpio_eint_set_enable(gpio, 0);
    sw_gpio_eint_clr_irqpd_sta(gpio);

    /* free irq */
    irq_no = __gpio_to_irq(gpio);
    PIO_DBG("%s: __gpio_to_irq(%d) ret %d\n", __func__, gpio, irq_no);
    free_irq(irq_no, (void *)pdev_id);
    kfree((void *)pdev_id);
}
EXPORT_SYMBOL(sw_gpio_irq_free);

