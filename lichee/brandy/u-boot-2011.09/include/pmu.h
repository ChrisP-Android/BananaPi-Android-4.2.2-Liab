/*
 * (C) Copyright 2012
 *     wangflord@allwinnertech.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */

#ifndef _PMU_H_
#define _PMU_H_

#define PMU_PRE_SYS_MODE           (0x0e)
#define PMU_PRE_BOOT_MODE          (0x0f)

#define PMU_SHORT_KEY_PRESSED      (1<<1)
#define PMU_LONG_KEY_PRESSED       (1<<0)

#define AXP_VBUS_EXIST             (2)
#define AXP_DCIN_EXIST             (3)


#define PMU_SUPPLY_DCDC_TYPE    (0x00010000)
#define PMU_SUPPLY_DCDC1        (0x00010001)
#define PMU_SUPPLY_DCDC2        (0x00010002)
#define PMU_SUPPLY_DCDC3        (0x00010003)
#define PMU_SUPPLY_DCDC4        (0x00010004)
#define PMU_SUPPLY_DCDC5        (0x00010005)

#define PMU_SUPPLY_ADLO_TYPE    (0x00020000)
#define PMU_SUPPLY_ALDO1        (0x00020001)
#define PMU_SUPPLY_ALDO2		(0x00020002)
#define PMU_SUPPLY_ALDO3		(0x00020003)

#define PMU_SUPPLY_DDLO_TYPE    (0x00030000)
#define PMU_SUPPLY_DLDO1		(0x00030001)
#define PMU_SUPPLY_DLDO2		(0x00030002)
#define PMU_SUPPLY_DLDO3		(0x00030003)
#define PMU_SUPPLY_DLDO4		(0x00030004)

#define PMU_SUPPLY_EDLO_TYPE    (0x00040000)
#define PMU_SUPPLY_ELDO1		(0x00040001)
#define PMU_SUPPLY_ELDO2		(0x00040002)
#define PMU_SUPPLY_ELDO3		(0x00040003)

#define PMU_SUPPLY_GPIOLDO_TYPE (0x00050000)
#define PMU_SUPPLY_GPIO0LDO		(0x00050001)
#define PMU_SUPPLY_GPIO1LDO		(0x00050002)

#define PMU_SUPPLY_MISC_TYPE    (0x00060000)
#define PMU_SUPPLY_DC5LDO		(0x00060001)
#define PMU_SUPPLY_DC1SW	    (0x00060002)


extern int axp_probe(void);
extern int axp_reinit(void);

extern int axp_set_supply_status(int pmu_type, int vol_name, int vol_value, int onoff);
extern int axp_set_power_supply_output(void);
extern int axp_set_supply_status_byname(char *vol_type, int vol_value, int onoff);

extern int axp_set_hardware_poweroff_vol(void);
extern int axp_set_hardware_poweron_vol(void);
extern int axp_probe_startup_cause(void);
extern int axp_probe_power_supply_condition(void);
extern int axp_set_power_off(void);
extern int axp_set_next_poweron_status(int data);

extern int axp_probe_key(void);

extern int axp_probe_dcin_exist(void);
extern int axp_probe_battery_exist(void);
extern int axp_probe_battery_vol(void);

extern int axp_probe_charge_current(void);
extern int axp_set_charge_current(int current);
extern int axp_set_charge_control(void);

extern int axp_set_vbus_cur_limit(int current);
extern int axp_set_vbus_vol_limit(int vol);

extern int axp_probe_key(void);

extern int axp_get_power_vol_level(void);
extern int axp_power_get_dcin_battery_exist(int *dcin_exist, int *battery_exist);
extern int  axp_probe_rest_battery_capacity(void);

extern int axp_set_vbus_limit_dc(void);
extern int axp_set_vbus_limit_pc(void);

extern int axp_set_charge_vol_limit(void);
extern int axp_set_all_limit(void);

extern int axp_int_enable(unsigned char *value);
extern int axp_int_disable(void);
extern int axp_int_query(unsigned char *addr);


#endif	/* _PMU_H_ */
