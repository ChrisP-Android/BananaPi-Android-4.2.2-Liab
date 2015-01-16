/*
 * bcm40181 sdio wifi power management API
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>

#include "wifi_pm.h"

#define bcm40181_msg(...)    do {printk("[bcm40181]: "__VA_ARGS__);} while(0)

static int bcm40181_powerup = 0;
static int bcm40181_vdd_en = 0;
static int bcm40181_vcc_en = 0;
static int bcm40181_shdn   = 0;

static int bcm40181_gpio_ctrl(char* name, int level)
{
	int i = 0, ret = 0, gpio = 0;
	unsigned long flags = 0;
	char* gpio_name[3] = {"bcm40181_vdd_en", "bcm40181_vcc_en", "bcm40181_shdn"};

	for (i=0; i<3; i++) {
		if (strcmp(name, gpio_name[i])==0) {
			    switch (i)
			    {
			        case 0: /* bcm40181_vdd_en */
						gpio = bcm40181_vdd_en;
			            break;
			        case 1: /* bcm40181_vcc_en */
						gpio = bcm40181_vcc_en;
			            break;
					case 2: /* bcm40181_shdn */
						gpio = bcm40181_shdn;
						break;
					default:
            			bcm40181_msg("no matched gpio!\n");
			    }
			break;
		}
	}

	if (i==3) {
		bcm40181_msg("No gpio %s for bcm40181-wifi module\n", name);
		return -1;
	}
	
	if (1==level)
		flags = GPIOF_OUT_INIT_HIGH;
	else
		flags = GPIOF_OUT_INIT_LOW;

	ret = gpio_request_one(gpio, flags, NULL);
	if (ret) {
		bcm40181_msg("failed to set gpio %d to %d !\n", gpio, level);
		return -1;
	} else {
		gpio_free(gpio);
		bcm40181_msg("succeed to set gpio %d to %d !\n", gpio, level);
	}

    if (strcmp(name, "bcm40181_vdd_en") == 0) {
        bcm40181_powerup = level;
    }
    
	return 0;
}

void bcm40181_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
			bcm40181_gpio_ctrl("bcm40181_vcc_en", 1);
			udelay(50);
			bcm40181_gpio_ctrl("bcm40181_vdd_en", 1);
			udelay(500);
			bcm40181_gpio_ctrl("bcm40181_shdn", 1);
        } else {
			bcm40181_gpio_ctrl("bcm40181_shdn", 0);
			bcm40181_gpio_ctrl("bcm40181_vdd_en", 0);
			bcm40181_gpio_ctrl("bcm40181_vcc_en", 0);
        }
        bcm40181_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
    } else {
        if (bcm40181_powerup)
            *updown = 1;
        else
            *updown = 0;
		bcm40181_msg("sdio wifi power state: %s\n", bcm40181_powerup ? "on" : "off");
    }
    return;	
}

void bcm40181_gpio_init(void)
{
	script_item_u val ;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	bcm40181_msg("exec bcm40181_wifi_gpio_init\n");

	type = script_get_item(wifi_para, "bcm40181_vdd_en", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		bcm40181_msg("get bcm40181 bcm40181_vdd_en gpio failed\n");
	else
		bcm40181_vdd_en = val.gpio.gpio;

	type = script_get_item(wifi_para, "bcm40181_vcc_en", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		bcm40181_msg("get bcm40181 bcm40181_vcc_en gpio failed\n");
	else
		bcm40181_vcc_en = val.gpio.gpio;

	type = script_get_item(wifi_para, "bcm40181_shdn", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		bcm40181_msg("get bcm40181 bcm40181_shdn gpio failed\n");
	else
		bcm40181_shdn = val.gpio.gpio;

	bcm40181_powerup = 0;
	ops->gpio_ctrl	= bcm40181_gpio_ctrl;
	ops->power = bcm40181_power;
}
