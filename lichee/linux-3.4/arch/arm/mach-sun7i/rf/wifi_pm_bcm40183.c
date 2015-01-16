/*
 * bcm40183 sdio wifi power management API
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/delay.h>

#include "wifi_pm.h"

#define bcm40183_msg(...)    do {printk("[bcm40183]: "__VA_ARGS__);} while(0)
static int bcm40183_wl_on = 0;
static int bcm40183_bt_on = 0;
static int bcm40183_vdd_en = 0;
static int bcm40183_vcc_en = 0;
static int bcm40183_wl_regon = 0;
static int bcm40183_bt_regon = 0;
static int bcm40183_bt_rst = 0;

static int bcm40183_gpio_ctrl(char* name, int level)
{
	int i = 0, ret = 0, gpio = 0;
	unsigned long flags = 0;
	char* gpio_name[3] = {"bcm40183_wl_regon", "bcm40183_bt_regon", "bcm40183_bt_rst"};
	
	for (i=0; i<3; i++) {
		if (strcmp(name, gpio_name[i])==0) {
			    switch (i)
			    {
			        case 0: /* bcm40183_wl_regon */
						gpio = bcm40183_wl_regon;
			            break;
			        case 1: /* bcm40183_bt_regon */
						gpio = bcm40183_bt_regon;
			            break;
					case 2: /* bcm40183_bt_rst */
						gpio = bcm40183_bt_rst;
						break;
					default:
            			bcm40183_msg("no matched gpio!\n");
			    }
			break;
		}
	}

	if (i==3) {
		bcm40183_msg("No gpio %s for BCM40183 module\n", name);
		return -1;
	}

	if (1==level)
		flags = GPIOF_OUT_INIT_HIGH;
	else 
		flags = GPIOF_OUT_INIT_LOW;

	bcm40183_msg("set GPIO %s to %d !\n", name, level);
	if (strcmp(name, "bcm40183_wl_regon") == 0) {
		if (level) {
			if (bcm40183_bt_on) {
				bcm40183_msg("BCM40183 is already powered up by bluetooth\n");
				goto change_state;
			} else {
				bcm40183_msg("BCM40183 is powered up by wifi\n");
				goto power_change;
			}
		} else {
			if (bcm40183_bt_on) {
				bcm40183_msg("BCM40183 should stay on because of bluetooth\n");
				goto change_state;
			} else {
				bcm40183_msg("BCM40183 is powered off by wifi\n");
				goto power_change;
			}
		}
	}

	if (strcmp(name, "bcm40183_bt_regon") == 0) {
		if (level) {
			if (bcm40183_wl_on) {
				bcm40183_msg("BCM40183 is already powered up by wifi\n");
				goto change_state;
			} else {
				bcm40183_msg("BCM40183 is powered up by bt\n");
				goto power_change;
			}
		} else {
			if (bcm40183_wl_on) {
				bcm40183_msg("BCM40183 should stay on because of wifi\n");
				goto change_state;
			} else {
				bcm40183_msg("BCM40183 is powered off by bt\n");
				goto power_change;
			}
		}
	}

gpio_state_change:

	ret = gpio_request_one(gpio, flags, NULL);
	if (ret) {
		bcm40183_msg("failed to set gpio %d to %d !\n", gpio, level);
		return -1;
	} else {
		gpio_free(gpio);
		bcm40183_msg("succeed to set gpio %d to %d !\n", gpio, level);
	}

	return 0;

power_change:

	ret = gpio_request_one(bcm40183_vcc_en, flags, NULL);
	if (ret) {
		bcm40183_msg("failed to set gpio bcm40183_vcc_en to %d !\n", level);
		return -1;
	} else {
		gpio_free(bcm40183_vcc_en);
		bcm40183_msg("succeed to set gpio bcm40183_vcc_en to %d !\n", level);
	}

	ret = gpio_request_one(bcm40183_vdd_en, flags, NULL);
	if (ret) {
		bcm40183_msg("failed to set gpio bcm40183_vdd_en to %d !\n", level);
		return -1;
	} else {
		gpio_free(bcm40183_vdd_en);
		bcm40183_msg("succeed to set gpio bcm40183_vdd_en to %d !\n", level);
	}

	udelay(500);

change_state:
	if (strcmp(name, "bcm40183_wl_regon")==0)
		bcm40183_wl_on = level;
	if (strcmp(name, "bcm40183_bt_regon")==0)
		bcm40183_bt_on = level;
	bcm40183_msg("BCM40183 power state change: wifi %d, bt %d !!\n", bcm40183_wl_on, bcm40183_bt_on);
	
	goto gpio_state_change;
}

static void bcm40183_power(int mode, int *updown)
{
	if (mode) {
		if (*updown) {
            bcm40183_gpio_ctrl("bcm40183_wl_regon", 1);
		} else {
            bcm40183_gpio_ctrl("bcm40183_wl_regon", 0);
		}
		bcm40183_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
	} else {
        if (bcm40183_wl_on)
            *updown = 1;
        else
            *updown = 0;
		bcm40183_msg("sdio wifi power state: %s\n", bcm40183_wl_on ? "on" : "off");
	}	
	return;
}

void bcm40183_gpio_init(void)
{
	script_item_u val ;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;
	
	bcm40183_msg("exec bcm40183_wifi_gpio_init\n");

	type = script_get_item(wifi_para, "bcm40183_vdd_en", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		bcm40183_msg("get bcm40183 bcm40183_vdd_en gpio failed\n");
	else
		bcm40183_vdd_en = val.gpio.gpio;

	type = script_get_item(wifi_para, "bcm40183_vcc_en", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		bcm40183_msg("get bcm40183 bcm40183_vcc_en gpio failed\n");
	else
		bcm40183_vcc_en = val.gpio.gpio;

	type = script_get_item(wifi_para, "bcm40183_wl_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		bcm40183_msg("get bcm40183 bcm40183_wl_regon gpio failed\n");
	else
		bcm40183_wl_regon = val.gpio.gpio;

	type = script_get_item(wifi_para, "bcm40183_bt_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		bcm40183_msg("get bcm40183 bcm40183_bt_regon gpio failed\n");
	else
		bcm40183_bt_regon = val.gpio.gpio;

	type = script_get_item(wifi_para, "bcm40183_bt_rst", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		bcm40183_msg("get bcm40183 bcm40183_bt_rst gpio failed\n");
	else
		bcm40183_bt_rst = val.gpio.gpio;

	bcm40183_wl_on = 0;
	bcm40183_bt_on = 0;
	ops->gpio_ctrl	= bcm40183_gpio_ctrl;
	ops->power = bcm40183_power;
}
