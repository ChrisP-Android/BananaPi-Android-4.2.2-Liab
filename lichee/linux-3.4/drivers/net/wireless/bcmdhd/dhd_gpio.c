/*
* Customer code to add GPIO control during WLAN start/stop
* Copyright (C) 1999-2011, Broadcom Corporation
* 
*         Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2 (the "GPL"),
* available at http://www.broadcom.com/licenses/GPLv2.php, with the
* following added to such license:
* 
*      As a special exception, the copyright holders of this software give you
* permission to link this software with independent modules, and to copy and
* distribute the resulting executable under terms of your choice, provided that
* you also meet, for each linked independent module, the terms and conditions of
* the license of that module.  An independent module is a module which is not
* derived from this software.  The special exception does not apply to any
* modifications of the software.
* 
*      Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*
* $Id: dhd_custom_gpio.c,v 1.2.42.1 2010-10-19 00:41:09 Exp $
*/

#include <osl.h>

#include <mach/gpio.h>
#include <mach/sys_config.h>

#ifdef CUSTOMER_HW

extern int __gpio_to_irq(unsigned gpio);
extern int gpio_direction_input(unsigned gpio);
extern int gpio_request(unsigned gpio, const char *label);
extern void gpio_free(unsigned gpio);

#ifdef CUSTOMER_OOB
int bcm_wlan_get_oob_irq(void)
{
	int host_oob_irq = 0;
	int ret = 0;
	int wl_host_wake = 0;
	script_item_u val ;
	script_item_value_type_e type;
	
	printk("bcm_wlan_get_oob_irq enter.\n");
	
	type = script_get_item("wifi_para", "ap6xxx_wl_host_wake", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		printk("get bcmdhd ap6xxx_wl_host_wake gpio failed\n");
	else
		wl_host_wake = val.gpio.gpio;
		
	host_oob_irq = __gpio_to_irq(wl_host_wake);
	if (host_oob_irq == -ENXIO) {
		printk("bcm_wlan_get_oob_irq fail to get WL_HOST_WAKE irq.\n");
		return 0;
	}
	printk("bcm_wlan_get_oob_irq GPIO(WL_HOST_WAKE) = %d\n", host_oob_irq);

	ret = gpio_request(wl_host_wake, NULL);
	if (ret != 0) {
		printk("bcm_wlan_get_oob_irq gpio request fail.\n");
		return 0;
	}
	
	ret = gpio_direction_input(wl_host_wake);
	if (ret != 0) {
		printk("bcm_wlan_get_oob_irq gpio_direction_input failed.\n");
	}

	gpio_free(wl_host_wake);

	return host_oob_irq;
}
#endif


#endif /* CUSTOMER_HW */
