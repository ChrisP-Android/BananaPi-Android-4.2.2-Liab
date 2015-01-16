/*
 * rtl8189es sdio wifi power management API
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>
#include "wifi_pm.h"

#define rtl8189es_msg(...)    do {printk("[rtl8189es]: "__VA_ARGS__);} while(0)

static int rtl8189es_powerup = 0;
static int rtl8189es_suspend = 0;
static int rtl8189es_shdn = 0;
static char * axp_name = NULL;

// power control by axp
static int rtl8189es_module_power(int onoff)
{
	struct regulator* wifi_ldo = NULL;
	static int first = 1;
	int ret = 0;

	rtl8189es_msg("rtl8189es module power set by axp.\n");
	wifi_ldo = regulator_get(NULL, axp_name);
	if (IS_ERR(wifi_ldo)) {
		rtl8189es_msg("get power regulator failed.\n");
		return -ret;
	}

	if (first) {
		rtl8189es_msg("first time\n");
		ret = regulator_force_disable(wifi_ldo);
		if (ret < 0) {
			rtl8189es_msg("regulator_force_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
		regulator_put(wifi_ldo);
		first = 0;
		return ret;
	}

	if (onoff) {
		rtl8189es_msg("regulator on.\n");
		ret = regulator_set_voltage(wifi_ldo, 3300000, 3300000);
		if (ret < 0) {
			rtl8189es_msg("regulator_set_voltage fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}

		ret = regulator_enable(wifi_ldo);
		if (ret < 0) {
			rtl8189es_msg("regulator_enable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	} else {
		rtl8189es_msg("regulator off.\n");
		ret = regulator_disable(wifi_ldo);
		if (ret < 0) {
			rtl8189es_msg("regulator_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	}
	regulator_put(wifi_ldo);
	return ret;
}

static int rtl8189es_gpio_ctrl(char* name, int level)
{
	int ret = 0;
	unsigned long flags = 0;

	if (1==level)
		flags = GPIOF_OUT_INIT_HIGH;
	else
		flags = GPIOF_OUT_INIT_LOW;

	ret = gpio_request_one(rtl8189es_shdn, flags, NULL);
	if (ret) {
		rtl8189es_msg("failed to set gpio %s to %d !\n", name, level);
		return -1;
	} else {
		gpio_free(rtl8189es_shdn);
		rtl8189es_msg("succeed to set gpio %s to %d !\n", name, level);
	}

	rtl8189es_powerup = level;

	return 0;
}

static void rtl8189es_standby(int instadby)
{
	if (instadby) {
		if (rtl8189es_powerup) {
			rtl8189es_gpio_ctrl("rtl8189es_shdn", 0);
			rtl8189es_module_power(0);
			rtl8189es_suspend = 1;
		}
	} else {
		if (rtl8189es_suspend) {
			rtl8189es_module_power(1);
			udelay(500);
			rtl8189es_gpio_ctrl("rtl8189es_shdn", 1);
			rtl8189es_suspend = 0;
		}
	}
	rtl8189es_msg("sdio wifi : %s\n", instadby ? "suspend" : "resume");
}

static void rtl8189es_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
			rtl8189es_module_power(1);
			udelay(500);
			rtl8189es_gpio_ctrl("rtl8189es_shdn", 1);
        } else {
			rtl8189es_gpio_ctrl("rtl8189es_shdn", 0);
			rtl8189es_module_power(0);
        }
        rtl8189es_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
    } else {
        if (rtl8189es_powerup)
            *updown = 1;
        else
            *updown = 0;
		rtl8189es_msg("sdio wifi power state: %s\n", rtl8189es_powerup ? "on" : "off");
    }
    return;
}

void rtl8189es_gpio_init(void)
{
	script_item_u val ;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	rtl8189es_msg("exec rtl8189es_wifi_gpio_init\n");

	type = script_get_item(wifi_para, "wifi_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		rtl8189es_msg("failed to fetch wifi_power\n");
		return ;
	}

	axp_name = val.str;

	type = script_get_item(wifi_para, "rtl8189es_shdn", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		rtl8189es_msg("get rtl8189es rtl8189es_shdn gpio failed\n");
	else
		rtl8189es_shdn = val.gpio.gpio;

	rtl8189es_powerup = 0;
	rtl8189es_suspend = 0;
	ops->standby 	  = rtl8189es_standby;
	ops->power 		  = rtl8189es_power;
	
	// force to disable wifi power in system booting,
	// make sure wifi power is down when system start up
	rtl8189es_module_power(0);
}
