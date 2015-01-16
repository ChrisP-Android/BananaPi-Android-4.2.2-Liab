/*
 * arch/arm/mach-sun7i/gpio/gpio_init.c
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
 * gpio_save - save somethig for the chip before enter sleep
 * @chip:   aw_gpio_chip which will be saved
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_save(struct aw_gpio_chip *pchip)
{
    /* save something before suspend */

    return 0;
}

/**
 * gpio_resume - restore somethig for the chip after wake up
 * @chip:   aw_gpio_chip which will be saved
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_resume(struct aw_gpio_chip *pchip)
{
    /* restore something after wakeup */

    return 0;
}

/*
 * gpio power api struct
 */
struct gpio_pm_t g_pm = {
    gpio_save,
    gpio_resume
};

/*
 * gpio config api struct
 */
struct gpio_cfg_t g_cfg = {
    gpio_set_cfg,
    gpio_get_cfg,
    gpio_set_pull,
    gpio_get_pull,
    gpio_set_drvlevel,
    gpio_get_drvlevel,
};

/*
 * gpio eint config api struct
 */
struct gpio_eint_cfg_t g_eint_cfg = {
    gpio_eint_set_trig,
    gpio_eint_get_trig,
    gpio_eint_set_enable,
    gpio_eint_get_enable,
    gpio_eint_get_irqpd_sta,
    gpio_eint_clr_irqpd_sta,
    gpio_eint_set_debounce,
    gpio_eint_get_debounce,
};

/*
 * gpio chips for the platform
 */
struct aw_gpio_chip gpio_chips[] = {
    {
        .cfg    = &g_cfg,
        .pm     = &g_pm,
        .chip   = {
            .base   = PA_NR_BASE,
            .ngpio  = PA_NR,
            .label  = "GPA",
        },
        .vbase  = (void __iomem *)PIO_VBASE(0),
    }, {
        .cfg    = &g_cfg,
        .pm     = &g_pm,
        .chip   = {
            .base   = PB_NR_BASE,
            .ngpio  = PB_NR,
            .label  = "GPB",
        },
        .vbase  = (void __iomem *)PIO_VBASE(1),
    }, {
        .cfg    = &g_cfg,
        .pm     = &g_pm,
        .chip   = {
            .base   = PC_NR_BASE,
            .ngpio  = PC_NR,
            .label  = "GPC",
        },
        .vbase  = (void __iomem *)PIO_VBASE(2),
    }, {
        .cfg    = &g_cfg,
        .pm     = &g_pm,
        .chip   = {
            .base   = PD_NR_BASE,
            .ngpio  = PD_NR,
            .label  = "GPD",
        },
        .vbase  = (void __iomem *)PIO_VBASE(3),
    }, {
        .cfg    = &g_cfg,
        .pm     = &g_pm,
        .chip   = {
            .base   = PE_NR_BASE,
            .ngpio  = PE_NR,
            .label  = "GPE",
        },
        .vbase  = (void __iomem *)PIO_VBASE(4),
    }, {
        .cfg    = &g_cfg,
        .pm     = &g_pm,
        .chip   = {
            .base   = PF_NR_BASE,
            .ngpio  = PF_NR,
            .label  = "GPF",
        },
        .vbase  = (void __iomem *)PIO_VBASE(5),
    }, {
        .cfg    = &g_cfg,
        .pm     = &g_pm,
        .chip   = {
            .base   = PG_NR_BASE,
            .ngpio  = PG_NR,
            .label  = "GPG",
        },
        .vbase  = (void __iomem *)PIO_VBASE(6),
    }, {
        .cfg    = &g_cfg,
        .pm     = &g_pm,
        .chip   = {
            .base   = PH_NR_BASE,
            .ngpio  = PH_NR,
            .label  = "GPH",
            .to_irq = __pio_to_irq,
        },
        .vbase  = (void __iomem *)PIO_VBASE(7),
        /* cfg for eint */
        .irq_num = AW_IRQ_GPIO,
        .vbase_eint = (void __iomem *)PIO_VBASE_EINT,
        .cfg_eint = &g_eint_cfg,
    }, {
        .cfg    = &g_cfg,
        .pm     = &g_pm,
        .chip   = {
            .base   = PI_NR_BASE,
            .ngpio  = PI_NR,
            .label  = "GPI",
            .to_irq = __pio_to_irq,
        },
        .vbase  = (void __iomem *)PIO_VBASE(8),
        /* cfg for eint */
        .irq_num = AW_IRQ_GPIO,
        .vbase_eint = (void __iomem *)PIO_VBASE_EINT,
        .cfg_eint = &g_eint_cfg,
    }
};

static struct clk *g_apb_pio_clk = NULL;
int gpio_clk_init(void)
{
    if (g_apb_pio_clk) {
        PIO_ERR("%s: apb pio clk handle not NULL\n", __func__);
    }

    g_apb_pio_clk = clk_get(NULL, CLK_APB_PIO);
    PIO_DBG("%s: g_apb_pio_clk: 0x%x\n", __func__, (u32)g_apb_pio_clk);
    if (!g_apb_pio_clk || IS_ERR(g_apb_pio_clk)) {
        PIO_ERR("%s: get apb pio clk failed\n", __func__);
        return -EPERM;
    } else {
        if (clk_enable(g_apb_pio_clk)) {
            PIO_ERR("%s: enable apb pio clk failed\n", __func__);
            return -EPERM;
        }
        PIO_INF("%s: apb pio clk enable success\n", __func__);
    }

    return 0;
}

int gpio_clk_deinit(void)
{
    if (!g_apb_pio_clk || IS_ERR(g_apb_pio_clk)) {
        PIO_ERR("%s: invalid apb pio clk handle\n", __func__);
        return -EPERM;
    }

    clk_disable(g_apb_pio_clk);
    clk_put(g_apb_pio_clk);
    g_apb_pio_clk = NULL;

    PIO_INF("%s: apb pio clk disable success\n", __func__);
    return 0;
}

#ifdef GPIO_SUPPORT_STANDBY
int gpio_drv_suspend(struct device *dev)
{
    if (NORMAL_STANDBY == standby_type) {
        PIO_INF("%s: normal standby, do nothing\n", __func__);
    } else if (SUPER_STANDBY == standby_type) {
        PIO_INF("%s: super standby\n", __func__);
        gpio_clk_deinit();
    }

    return 0;
}

int gpio_drv_resume(struct device *dev)
{
    if (NORMAL_STANDBY == standby_type) {
        PIO_INF("%s: normal standby, do nothing\n", __func__);
    } else if (SUPER_STANDBY == standby_type) {
        PIO_INF("%s: super standby\n", __func__);
        gpio_clk_init();
    }

}

static const struct dev_pm_ops sw_gpio_pm = {
    .suspend = gpio_drv_suspend,
    .resume  = gpio_drv_resume,
};
#endif

static struct platform_device sw_gpio_device = {
    .name = "sw_gpio",
};

static struct platform_driver sw_gpio_driver = {
    .driver = {
        .name  = "sw_gpio",
        .owner = THIS_MODULE,
#ifdef GPIO_SUPPORT_STANDBY
        .pm    = &sw_gpio_pm,
#endif
    }
};

/**
 * aw_gpio_init - gpio driver init function
 *
 * Returns 0 if sucess, -1 if failed.
 */
static __init int aw_gpio_init(void)
{
    u32 i = 0;

    PIO_INF("aw gpio init start\n");

    if (gpio_clk_init()) {
        PIO_ERR("%s: gpio clk init failed\n", __func__);
    }

    for (i = 0; i < ARRAY_SIZE(gpio_chips); i++) {
        /* lock init */
        PIO_CHIP_LOCK_INIT(&gpio_chips[i].lock);

        /* register gpio_chip */
        if (0 != aw_gpiochip_add(&gpio_chips[i].chip)) {
            return -1;
        }
    }

    if (platform_device_register(&sw_gpio_device)) {
        PIO_ERR("%s: register gpio device failed\n", __func__);
    }
    if (platform_driver_register(&sw_gpio_driver)) {
        PIO_ERR("%s: register gpio driver failed\n", __func__);
    }

    PIO_INF("aw gpio init done\n");
    return 0;
}
subsys_initcall(aw_gpio_init);

