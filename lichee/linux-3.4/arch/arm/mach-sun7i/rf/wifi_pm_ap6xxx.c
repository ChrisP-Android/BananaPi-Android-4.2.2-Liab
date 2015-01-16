/*
 * ap6xxx sdio wifi power management API
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <mach/clock.h>
#include <linux/regulator/consumer.h>
#include "wifi_pm.h"

#define ap6xxx_msg(...)    do {printk("[ap6xxx]: "__VA_ARGS__);} while(0)

static int ap6xxx_wl_regon = 0;
static int ap6xxx_bt_regon = 0;
static char * axp_name = NULL;

// power control by axp
static int ap6xxx_module_power(int onoff)
{
	struct regulator* wifi_ldo = NULL;
	static int first = 1;
	int ret = 0;

	ap6xxx_msg("ap6xxx module power set by axp.\n");
	wifi_ldo = regulator_get(NULL, axp_name);
	if (IS_ERR(wifi_ldo)) {
		ap6xxx_msg("get power regulator failed.\n");
		return -ret;
	}

	if (first) {
		ap6xxx_msg("first time\n");
		ret = regulator_force_disable(wifi_ldo);
		if (ret < 0) {
			ap6xxx_msg("regulator_force_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
		first = 0; 
	}

	if (onoff) {
		ap6xxx_msg("regulator on.\n");
		ret = regulator_set_voltage(wifi_ldo, 3300000, 3300000);
		if (ret < 0) {
			ap6xxx_msg("regulator_set_voltage fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}

		ret = regulator_enable(wifi_ldo);
		if (ret < 0) {
			ap6xxx_msg("regulator_enable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	} else {
		ap6xxx_msg("regulator off.\n");
		ret = regulator_disable(wifi_ldo);
		if (ret < 0) {
			ap6xxx_msg("regulator_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	}
	regulator_put(wifi_ldo);
	return ret;
}

static int ap6xxx_gpio_ctrl(char* name, int level)
{
	int i = 0;	
	int ret = 0;
	int gpio = 0;
	unsigned long flags = 0;
	char * gpio_name[2] = {"ap6xxx_wl_regon", "ap6xxx_bt_regon"};

	for (i = 0; i < 2; i++) {
		if (strcmp(name, gpio_name[i]) == 0) {
			switch (i)
			{
				case 0: /*ap6xxx_wl_regon*/
					gpio = ap6xxx_wl_regon;
					break;
				case 1: /*ap6xxx_bt_regon*/
					gpio = ap6xxx_bt_regon;
					break;
				default:
					ap6xxx_msg("no matched gpio.\n");
			}
			break;
		}
	}

	if (1==level)
		flags = GPIOF_OUT_INIT_HIGH;
	else
		flags = GPIOF_OUT_INIT_LOW;

	ret = gpio_request_one(gpio, flags, NULL);
	if (ret) {
		ap6xxx_msg("failed to set gpio %s to %d !\n", name, level);
		return -1;
	} else {
		gpio_free(gpio);
		ap6xxx_msg("succeed to set gpio %s to %d !\n", name, level);
	}
    
	return 0;
}

void ap6xxx_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
			ap6xxx_gpio_ctrl("ap6xxx_wl_regon", 1);
			mdelay(100);
        } else {
			ap6xxx_gpio_ctrl("ap6xxx_wl_regon", 0);
			mdelay(100);
        }
        ap6xxx_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
    }
    return;	
}

static void cfg_gpio_32k_clkout(int gpio_index)
{
    int ret;    
    struct clk *clk_32k, *parent;    
    
    if(gpio_index != GPIOI(12) && gpio_index != GPIOI(13)){
        printk("gpio %d can't config as clock output\n", gpio_index);
        return;
    }
    
    if(0 != gpio_request(gpio_index, NULL)) {
    	printk("request gpio failed\n");
    	return;
    }
    
    if(0 != sw_gpio_setcfg(gpio_index, 4))
    	printk("set gpio function failed\n");
    
    if(0 != sw_gpio_setpull(gpio_index, 1))
    	printk("set pull failed\n");
    	
    if(0 != sw_gpio_setdrvlevel(gpio_index, 2))
	    printk("set drv level failed\n");
	
  	
    parent = clk_get(NULL, CLK_SYS_LOSC);	

    if(gpio_index == GPIOI(12))
        clk_32k = clk_get(NULL, CLK_MOD_OUTA);	
    else
        clk_32k = clk_get(NULL, CLK_MOD_OUTB);	
        
    ret = clk_set_parent(clk_32k, parent);
    if(ret){
        printk("clk_set_parent fail\n");
        goto exit;
    }
    ret = clk_set_rate(clk_32k, 32768);
    if(ret){
        printk("clk_set_rate fail\n");	
        goto exit;
    }
    clk_enable(clk_32k);	

exit:    
    gpio_free(gpio_index);    
}
void ap6xxx_gpio_init(void)
{
	script_item_u val;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;
    int ap6xxx_lpo = 0;

	type = script_get_item(wifi_para, "wifi_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		ap6xxx_msg("failed to fetch wifi_power\n");
		return;
	}

	axp_name = val.str;
	ap6xxx_msg("module power name %s\n", axp_name);

    type = script_get_item(wifi_para, "ap6xxx_lpo", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		ap6xxx_msg("get ap6xxx ap6xxx_lpo gpio failed\n");
	else
		ap6xxx_lpo = val.gpio.gpio;
    if(ap6xxx_lpo)
	    cfg_gpio_32k_clkout(ap6xxx_lpo);	
	type = script_get_item(wifi_para, "ap6xxx_wl_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		ap6xxx_msg("get ap6xxx ap6xxx_wl_regon gpio failed\n");
	else
		ap6xxx_wl_regon = val.gpio.gpio;

	type = script_get_item(wifi_para, "ap6xxx_bt_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		ap6xxx_msg("get ap6xxx ap6xxx_bt_regon gpio failed\n");
	else
		ap6xxx_bt_regon = val.gpio.gpio;

	ops->gpio_ctrl	= ap6xxx_gpio_ctrl;
	ops->power = ap6xxx_power;

	ap6xxx_module_power(1);
}
