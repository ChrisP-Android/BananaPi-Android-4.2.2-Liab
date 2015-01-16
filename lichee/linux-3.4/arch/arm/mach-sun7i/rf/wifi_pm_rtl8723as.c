/*
 * rtl8723as sdio wifi power management API
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>

#include "wifi_pm.h"

#define SDIO_MODULE_NAME "RTL8723AS"
#define rtl8723as_msg(...)    do {printk("[RTL8723AS]: "__VA_ARGS__);} while(0)
static int rtl8723as_wl_on = 0;
static int rtl8723as_bt_on = 0;
static int rtk_suspend = 0;
static int rtk_rtl8723as_wl_dis = 0;
static int rtk_rtl8723as_bt_dis = 0;
static char * axp_name = NULL;
static bool axp_power_on = false;
static struct mutex power_lock;

static int rtl8723as_module_power(int onoff)
{
	struct regulator* wifi_ldo = NULL;
	static int first = 1;
	int ret = 0;

	rtl8723as_msg("rtl8723as module power set by axp.\n");
	rtl8723as_msg("axp_name = %s\n", axp_name);
	wifi_ldo = regulator_get(NULL, axp_name);
	if (IS_ERR(wifi_ldo)) {
		rtl8723as_msg("get power regulator failed.\n");
		return -ret;
	}

	if (first) {
		rtl8723as_msg("first time\n");
		ret = regulator_force_disable(wifi_ldo);
		if (ret < 0) {
			rtl8723as_msg("regulator_force_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
		regulator_put(wifi_ldo);
		first = 0;
		return ret;
	}

	if (onoff) {
		if (axp_power_on == false) {
			rtl8723as_msg("regulator on.\n");
			ret = regulator_set_voltage(wifi_ldo, 3300000, 3300000);
			if (ret < 0) {
				rtl8723as_msg("regulator_set_voltage fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
				return ret;
			}

			ret = regulator_enable(wifi_ldo);
			if (ret < 0) {
				rtl8723as_msg("regulator_enable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
				return ret;
			}
			axp_power_on = true;
		}
	} else {
		if (axp_power_on == true) {
			rtl8723as_msg("regulator off.\n");
			ret = regulator_disable(wifi_ldo);
			if (ret < 0) {
				rtl8723as_msg("regulator_disable fail, return %d.\n", ret);
				return ret;
			}
			axp_power_on = false;
		}
	}
	regulator_put(wifi_ldo);
	return ret;
}

static int rtl8723as_gpio_ctrl(char* name, int level)
{
	int i = 0, ret = 0, gpio = 0;
	unsigned long flags = 0;
	char* gpio_name[2] = {"rtk_rtl8723as_wl_dis", "rtk_rtl8723as_bt_dis"};
	mutex_lock(&power_lock);
	for (i=0; i<2; i++) {
		if (strcmp(name, gpio_name[i])==0) {
			    switch (i)
			    {
			        case 0: /* rtk_rtl8723as_wl_dis */
						gpio = rtk_rtl8723as_wl_dis;
			            break;
					case 1: /* rtk_rtl8723as_bt_dis */
						gpio = rtk_rtl8723as_bt_dis;
						break;
					default:
            			rtl8723as_msg("no matched gpio!\n");
			    }
			break;
		}
	}

	if (i==2) {
		rtl8723as_msg("No gpio %s for %s module\n", name, SDIO_MODULE_NAME);
		mutex_unlock(&power_lock);
		return -1;
	}

	if (1==level)
		flags = GPIOF_OUT_INIT_HIGH;
	else
		flags = GPIOF_OUT_INIT_LOW;
	
	rtl8723as_msg("Set GPIO %s to %d !\n", name, level);
	if (strcmp(name, "rtk_rtl8723as_wl_dis") == 0) {
		if ((level && !rtl8723as_bt_on)	|| (!level && !rtl8723as_bt_on)) {
			rtl8723as_msg("%s is powered %s by wifi\n", SDIO_MODULE_NAME, level ? "up" : "down");
			goto power_change;
		} else {
			if (level) {
				rtl8723as_msg("%s is already on by bt\n", SDIO_MODULE_NAME);
			} else {
				rtl8723as_msg("%s should stay on because of bt\n", SDIO_MODULE_NAME);
			}
			goto state_change;
		}
	}
	if (strcmp(name, "rtk_rtl8723as_bt_dis") == 0) {
		if ((level && !rtl8723as_wl_on && !rtl8723as_bt_on)	|| (!level && !rtl8723as_wl_on)) {
			rtl8723as_msg("%s is powered %s by bt\n", SDIO_MODULE_NAME, level ? "up" : "down");
			goto power_change;
		} else {
			if (level) {
				rtl8723as_msg("%s is already on by %s\n", SDIO_MODULE_NAME, rtl8723as_bt_on ? "bt" : "wifi");
			} else {
				rtl8723as_msg("%s should stay on because of wifi\n", SDIO_MODULE_NAME);
			}
			goto state_change;
		}
	}

gpio_state_change:

	ret = gpio_request_one(gpio, flags, NULL);
	if (ret) {
		rtl8723as_msg("failed to set gpio %s to %d !\n", name, level);
		mutex_unlock(&power_lock);
		return -1;
	} else {
		gpio_free(gpio);
		rtl8723as_msg("succeed to set gpio %s to %d !\n", name, level);
	}
	mutex_unlock(&power_lock);
	return 0;
	
power_change:

	rtl8723as_module_power(level);
	udelay(500);
	
state_change:
	if (strcmp(name, "rtk_rtl8723as_wl_dis")==0)
		rtl8723as_wl_on = level;
	if (strcmp(name, "rtk_rtl8723as_bt_dis")==0)
		rtl8723as_bt_on = level;
	rtl8723as_msg("%s power state change: wifi %d, bt %d !!\n", SDIO_MODULE_NAME, rtl8723as_wl_on, rtl8723as_bt_on);
	
	goto gpio_state_change;
}

void rtl8723as_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
        	rtl8723as_gpio_ctrl("rtk_rtl8723as_wl_dis", 1);
        } else {
        	rtl8723as_gpio_ctrl("rtk_rtl8723as_wl_dis", 0);
        }
        rtl8723as_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
    } else {
        if (rtl8723as_wl_on)
            *updown = 1;
        else
            *updown = 0;
		rtl8723as_msg("sdio wifi power state: %s\n", rtl8723as_wl_on ? "on" : "off");
    }
    return;
}

static void rtl8723as_standby(int instadby)
{
	if (instadby) {
		if (rtl8723as_wl_on) {
			rtl8723as_gpio_ctrl("rtk_rtl8723as_wl_dis", 0);
			rtk_suspend = 1;
		}
	} else {
		if (rtk_suspend) {
			rtl8723as_gpio_ctrl("rtk_rtl8723as_wl_dis", 1);
			rtk_suspend = 0;
		}
	}
	rtl8723as_msg("sdio wifi : %s\n", instadby ? "suspend" : "resume");
}

void rtl8723as_gpio_init(void)
{
	script_item_u val ;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;
	
	rtl8723as_msg("exec rt8723as_wifi_gpio_init\n");

	type = script_get_item(wifi_para, "wifi_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		rtl8723as_msg("failed to fetch wifi_power\n");
		return ;
	}
	axp_name = val.str;

	type = script_get_item(wifi_para, "rtk_rtl8723as_wl_dis", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type)
		rtl8723as_msg("get rtl8723as rtk_rtl8723as_wl_dis gpio failed\n");
	else
		rtk_rtl8723as_wl_dis = val.gpio.gpio;

	type = script_get_item(wifi_para, "rtk_rtl8723as_bt_dis", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type)
		rtl8723as_msg("get rtl8723as rtk_rtl8723as_bt_dis gpio failed\n");
	else
		rtk_rtl8723as_bt_dis = val.gpio.gpio;
	
	rtl8723as_wl_on = 0;
	rtl8723as_bt_on = 0;
	rtk_suspend 	= 0;
	ops->gpio_ctrl	= rtl8723as_gpio_ctrl;
	ops->power 		= rtl8723as_power;
	ops->standby	= rtl8723as_standby;
	mutex_init(&power_lock);

	// force to disable wifi power in system booting,
	// make sure wifi power is down when system start up
	rtl8723as_module_power(0);
}

#undef SDIO_MODULE_NAME
