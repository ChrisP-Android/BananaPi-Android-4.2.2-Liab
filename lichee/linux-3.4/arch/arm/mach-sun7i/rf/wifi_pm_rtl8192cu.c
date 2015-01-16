/*
 * rtl8192cu usb wifi power management API
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>
#include "wifi_pm.h"

#define rtl8192cu_msg(...)    do {printk("[rtl8192cu]: "__VA_ARGS__);} while(0)

static int rtl8192cu_powerup = 0;
static int rtk8192cu_suspend = 0;
static char * axp_name = NULL;

// power control by axp
static int rtl8192cu_module_power(int onoff)
{
	struct regulator* wifi_ldo = NULL;
	static int first = 1;
	int ret = 0;

	rtl8192cu_msg("rtl8192cu module power set by axp.\n");
	wifi_ldo = regulator_get(NULL, axp_name);
	if (IS_ERR(wifi_ldo)) {
		rtl8192cu_msg("get power regulator failed.\n");
		return -ret;
	}

	if (first) {
		rtl8192cu_msg("first time\n");
		ret = regulator_force_disable(wifi_ldo);
		if (ret < 0) {
			rtl8192cu_msg("regulator_force_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
		regulator_put(wifi_ldo);
		first = 0;
		return ret;
	}

	if (onoff) {
		rtl8192cu_msg("regulator on.\n");
		ret = regulator_set_voltage(wifi_ldo, 3300000, 3300000);
		if (ret < 0) {
			rtl8192cu_msg("regulator_set_voltage fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}

		ret = regulator_enable(wifi_ldo);
		if (ret < 0) {
			rtl8192cu_msg("regulator_enable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	} else {
		rtl8192cu_msg("regulator off.\n");
		ret = regulator_disable(wifi_ldo);
		if (ret < 0) {
			rtl8192cu_msg("regulator_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	}
	regulator_put(wifi_ldo);
	return ret;
}

void rtl8192cu_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
			rtl8192cu_module_power(1);
			udelay(50);
			rtl8192cu_powerup = 1;
        } else {
			rtl8192cu_module_power(0);
			rtl8192cu_powerup = 0;
        }
        rtl8192cu_msg("usb wifi power state: %s\n", *updown ? "on" : "off");
    } else {
        if (rtl8192cu_powerup)
            *updown = 1;
        else
            *updown = 0;
		rtl8192cu_msg("usb wifi power state: %s\n", rtl8192cu_powerup ? "on" : "off");
    }
    return;	
}

static void rtl8192cu_standby(int instadby)
{
	if (instadby) {
		if (rtl8192cu_powerup) {
			rtl8192cu_module_power(0);
			rtk8192cu_suspend = 1;
		}
	} else {
		if (rtk8192cu_suspend) {
			rtl8192cu_module_power(1);
			rtk8192cu_suspend = 0;
		}
	}
	rtl8192cu_msg("usb wifi : %s\n", instadby ? "suspend" : "resume");
}

void rtl8192cu_gpio_init(void)
{
	script_item_u val;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	rtl8192cu_msg("exec rtl8192cu_wifi_gpio_init\n");

	type = script_get_item(wifi_para, "wifi_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		rtl8192cu_msg("failed to fetch wifi_power\n");
		return ;
	}

	axp_name = val.str;
	rtl8192cu_powerup = 0;
	rtk8192cu_suspend = 0;
	ops->power     = rtl8192cu_power;
	ops->standby   = rtl8192cu_standby;

	// force to disable wifi power in system booting,
	// make sure wifi power is down when system start up
	rtl8192cu_module_power(0);
}
