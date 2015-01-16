/*
 * rtl8723au usb wifi power management API
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>
#include "wifi_pm.h"

extern int sw_usb_disable_hcd(__u32 usbc_no);
extern int sw_usb_enable_hcd(__u32 usbc_no);

#define rtl8723au_msg(...)    do {printk("[rtl8723au]: "__VA_ARGS__);} while(0)

static char * axp_name = NULL;
static bool axp_power_on = false;
static int rtl8723au_wifi_power = 0;
static int rtl8723au_bt_power = 0;
static int usbc_id = 1;
static struct mutex power_lock;

// power control by axp
static int rtl8723au_module_power(int onoff)
{
	struct regulator* wifi_ldo = NULL;
	static int first = 1;
	int ret = 0;

	rtl8723au_msg("rtl8723au module power set by axp.\n");
	wifi_ldo = regulator_get(NULL, axp_name);
	if (IS_ERR(wifi_ldo)) {
		rtl8723au_msg("get power regulator failed.\n");
		return -ret;
	}

	if (first) {
		rtl8723au_msg("first time\n");
		ret = regulator_force_disable(wifi_ldo);
		if (ret < 0) {
			rtl8723au_msg("regulator_force_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
		first = 0;
	}

	if (onoff) {
		if (axp_power_on == false) {
			ret = regulator_set_voltage(wifi_ldo, 3300000, 3300000);
			if (ret < 0) {
				rtl8723au_msg("regulator_set_voltage fail, return %d.\n", ret);
				regulator_put(wifi_ldo);
				return ret;
			}

			ret = regulator_enable(wifi_ldo);
			if (ret < 0) {
				rtl8723au_msg("regulator_enable fail, return %d.\n", ret);
				regulator_put(wifi_ldo);
				return ret;
			}
			axp_power_on = true;
		}
	} else {
		if (axp_power_on == true) {
			ret = regulator_disable(wifi_ldo);
			if (ret < 0) {
				rtl8723au_msg("regulator_disable fail, return %d.\n", ret);
				regulator_put(wifi_ldo);
				return ret;
			}
			axp_power_on = false;
		}
	}
	regulator_put(wifi_ldo);
	return ret;
}

static int rtl8723au_gpio_ctrl(char* name, int level)
{
	static int usbc_enable = 0;
	mutex_lock(&power_lock);
	if (strcmp(name, "rtl8723au_wl") == 0) {
		if ((level && !rtl8723au_bt_power)	|| (!level && !rtl8723au_bt_power)) {
			rtl8723au_msg("rtl8723au is powered %s by wifi\n", level ? "up" : "down");
			goto power_change;
		} else {
			if (level) {
				rtl8723au_msg("rtl8723au is already on by bt\n");
			} else {
				rtl8723au_msg("rtl8723au should stay on because of bt\n");
			}
			goto state_change;
		}
	}
	
	if (strcmp(name, "rtl8723au_bt") == 0) {
		if ((level && !rtl8723au_wifi_power && !rtl8723au_bt_power)	|| (!level && !rtl8723au_wifi_power)) {
			rtl8723au_msg("rtl8723au is powered %s by bt\n", level ? "up" : "down");
			goto power_change;
		} else {
			if (level) {
				rtl8723au_msg("rtl8723au is already on by wifi\n");
			} else {
				rtl8723au_msg("rtl8723au should stay on because of wifi\n");
			}
			goto state_change;
		}
	}
	mutex_unlock(&power_lock);
	return 0;

power_change:	
	if (level) {
		rtl8723au_module_power(level);
		if (!usbc_enable) {
			msleep(10);
			sw_usb_enable_hcd(usbc_id);
			usbc_enable = 1;
		}
	} else {
		//if (usbc_enable && !rtl8723au_bt_power) {
		if (usbc_enable) {
			sw_usb_disable_hcd(usbc_id);
			msleep(1);
			usbc_enable = 0;
		}
		rtl8723au_module_power(level);
	}
	
state_change:
	if (strcmp(name, "rtl8723au_wl")==0)
		rtl8723au_wifi_power = level;
	if (strcmp(name, "rtl8723au_bt")==0)
		rtl8723au_bt_power = level;
	rtl8723au_msg("rtl8723au power state change: wifi %d, bt %d !!\n", rtl8723au_wifi_power, rtl8723au_bt_power);
	mutex_unlock(&power_lock);
	return 0;
}

void rtl8723au_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
        	rtl8723au_gpio_ctrl("rtl8723au_wl", 1);
        } else {
        	rtl8723au_gpio_ctrl("rtl8723au_wl", 0);
        }
    } else {
        if (rtl8723au_wifi_power)
            *updown = 1;
        else
            *updown = 0;
		rtl8723au_msg("usb wifi power state: %s\n", rtl8723au_wifi_power ? "on" : "off");
    }
    return;
}

static void rtl8723au_standby(int instadby)
{
	if (instadby) {
		//rtl8723au_module_power(0);
	} else {
		//rtl8723au_module_power(1);
	}
}

void rtl8723au_gpio_init(void)
{
	script_item_u val;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	rtl8723au_msg("exec rtl8723au_wifi_gpio_init\n");

	type = script_get_item(wifi_para, "wifi_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		rtl8723au_msg("failed to fetch wifi_power\n");
		return ;
	}
	axp_name = val.str;

	type = script_get_item(wifi_para, "wifi_usbc_id", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		rtl8723au_msg("failed to fetch wifi_usbc_id\n");
		return ;
	}
	usbc_id = val.val;
	
	ops->standby   = rtl8723au_standby;
	ops->gpio_ctrl = rtl8723au_gpio_ctrl;
	ops->power = rtl8723au_power;
	rtl8723au_wifi_power = 0;
	rtl8723au_bt_power = 0;
	
	mutex_init(&power_lock);
	// make sure wifi power down when system start up
	rtl8723au_module_power(0);
}
