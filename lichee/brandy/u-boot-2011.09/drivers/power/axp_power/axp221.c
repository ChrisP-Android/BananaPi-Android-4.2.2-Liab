/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include "common.h"
#include "axp_power.h"
#include "axp221.h"

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
 int axp221_set_charge_control(int onoff)
{
	u8 reg_value;
	//disable ts adc, enable all other adc
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_ADC_EN, &reg_value))
    {
        return -1;
    }
    reg_value |= 0xfe;
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_ADC_EN, reg_value))
    {
        return -1;
    }


	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_CHARGE1, &reg_value))
    {
        return -1;
    }
    if(onoff)
    {
         reg_value |= 0x80; //enable charge
    }
    else
    {
        reg_value  &=~0x80;//disable
    }
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_CHARGE1, reg_value))
    {
        return -1;
    }

    return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
static int axp221_get_charger_is_charge(void)
{
	u8 reg_value;
	//disable ts adc, enable all other adc
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_MODE_CHGSTATUS, &reg_value))
    {
        return -1;
    }
    return (reg_value >>=6)&0x1;

}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
static int axp221_clear_data_buffer(void)
{
	int i;

	for(i=BOOT_POWER22_DATA_BUFFER0;i<=BOOT_POWER22_DATA_BUFFER11;i++)
	{
		if(pm_bus_write(AXP22_ADDR, i, 0x00))
	    {
	        return -1;
	    }
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
 int axp221_probe_ac_exist(void)
{
	u8 reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_STATUS, &reg_value))
    {
        return -1;
    }

    return (reg_value>>6)&0x1 ;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
static int axp221_probe_vbus_exist(void)
{
	u8 reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_STATUS, &reg_value))
    {
        return -1;
    }

    return (reg_value>>4)&0x1 ;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_battery_ratio(void)
{
	u8 reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_COULOMB_CAL, &reg_value))
    {
        return -1;
    }

	return reg_value & 0x7f;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_battery_exist(void)
{
	u8 reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_MODE_CHGSTATUS, &reg_value))
    {
        return -1;
    }

	if(reg_value & 0x10)
	{
		return (reg_value>>5 & 0x1);
	}
	else
	{
		return -1;
	}
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_power_status(void)
{
	u8 reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_STATUS, &reg_value))
    {
        return -1;
    }
	if(reg_value & 0x40)		//dc in exist
	{
		return 3;
	}
	if(reg_value & 0x10)		//vbus exist
	{
		return 2;
	}
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_battery_vol(void)
{
	u8  reg_value_h, reg_value_l;
	int bat_vol, tmp_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_BAT_AVERVOL_H8, &reg_value_h))
    {
        return -1;
    }
    if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_BAT_AVERVOL_L4, &reg_value_l))
    {
        return -1;
    }
    tmp_value = (reg_value_h << 4) | reg_value_l;
    bat_vol = tmp_value * 11;
    bat_vol /= 10;

	return bat_vol;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_buttery_resistance_record(void)
{
	u8  reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_DATA_BUFFER1, &reg_value))
    {
        return -1;
    }

	return reg_value;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_key(void)
{
	u8  reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_INTSTS3, &reg_value))
    {
        return -1;
    }
    reg_value &= 0x03;
	if(reg_value)
	{
		if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_INTSTS3, reg_value))
	    {
	        return -1;
	    }
	}

	return reg_value;
}

int axp221_clear_key_status(int status_mask)
{
	u8  reg_value;


    status_mask &= 0x03;
	if(status_mask)
	{
		if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_INTSTS3, status_mask))
	    {
	        return -1;
	    }
	}
     else
	{
        return -1;
    }

	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_last_poweron_status(void)
{
	u8  reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_DATA_BUFFER11, &reg_value))
    {
        return -1;
    }
	printf("axp221_probe_last_poweron_status 0x%x\n", reg_value);

	return reg_value;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_next_poweron_status(int data)
{
	printf("axp221_set_next_poweron_status 0x%x\n", data);
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DATA_BUFFER11, (u8)data))
    {
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_write_data_buffer(int buffer, __u8 value)
{
	if(pm_bus_write(AXP22_ADDR, (u8)buffer, value))
    {
        return -1;
    }

	return 0;
}


/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_poweron_cause(void)
{
    __u8   reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_STATUS, &reg_value))
    {
        return -1;
    }

    return reg_value & 0x01;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dc1sw(int on_off)
{
    u8   reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, &reg_value))
    {
		printf("%d\n", __LINE__);
        return -1;
    }
    if(on_off)
    {
		reg_value |= (1 << 7);
	}
	else
	{
		reg_value &= ~(1 << 7);
	}
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, reg_value))
	{
		printf("sunxi pmu error : unable to set dcdc1\n");

		return -1;
	}

	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dcdc1(int set_vol)
{
    u8   reg_value;

	if((set_vol < 1600) || (set_vol > 3400))
	{
		printf("%d\n", __LINE__);
		return -1;
	}
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_DC1OUT_VOL, &reg_value))
    {
		printf("%d\n", __LINE__);
        return -1;
    }
	reg_value = ((set_vol - 1600)/100);
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DC1OUT_VOL, reg_value))
	{
		printf("sunxi pmu error : unable to set dcdc1\n");

		return -1;
	}

	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dcdc2(int set_vol)
{
    uint tmp, i;
    int  vol;
    u8   reg_value;

	if((set_vol < 600) || (set_vol > 1540))
	{
		return -1;
	}
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_DC2OUT_VOL, &reg_value))
    {
        return -1;
    }
    reg_value &= ~0x3f;
    reg_value |= (set_vol - 600)/20;
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DC2OUT_VOL, reg_value))
    {
        return -1;
    }

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dcdc3(int set_vol)
{
    u8   reg_value;
    u8   reg_dump;

	if((set_vol < 600) || (set_vol > 1860))
	{
		printf("%d\n", __LINE__);

		return -1;
	}
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_DC3OUT_VOL, &reg_value))
    {
    	printf("%d\n", __LINE__);

        return -1;
    }
	reg_value = ((set_vol - 600)/20);
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DC3OUT_VOL, reg_value))
	{
		printf("sunxi pmu error : unable to set dcdc3\n");

		return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dcdc4(int set_vol)
{
    u8   reg_value;

	if((set_vol < 600) || (set_vol > 1540))
	{
		printf("%d\n", __LINE__);

		return -1;
	}
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_DC4OUT_VOL, &reg_value))
    {
    	printf("%d\n", __LINE__);

        return -1;
    }
	reg_value = ((set_vol - 600)/20);
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DC4OUT_VOL, reg_value))
	{
		printf("sunxi pmu error : unable to set dcdc1\n");

		return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dcdc5(int set_vol)
{
    u8   reg_value;

	if((set_vol < 1000) || (set_vol > 2550))
	{
		printf("%d\n", __LINE__);

		return -1;
	}
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_DC5OUT_VOL, &reg_value))
    {
    	printf("%d\n", __LINE__);

        return -1;
    }
	reg_value = ((set_vol - 1000)/50);
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DC5OUT_VOL, reg_value))
	{
		printf("sunxi pmu error : unable to set dcdc1\n");

		return -1;
	}

	return 0;
}


/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_aldo1(int set_vol)
{
	u8 gate;
	u8 vol_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL1, &gate))
    {
        return -1;
    }
	if(!set_vol)
	{
	    gate &= ~(1<<6);
	}
	else
	{
		gate |=  (1<<6);
		if((set_vol < 700) || (set_vol > 3300))
		{
			return -1;
		}
		if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_ALDO1OUT_VOL, &vol_value))
	    {
	        return -1;
	    }
	    vol_value &= 0x1f;
		vol_value |= ((set_vol - 700)/100);
	    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_ALDO1OUT_VOL, vol_value))
	    {
	        return -1;
	    }
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL1, gate))
    {
    	printf("sunxi pmu error : unable to set aldo1\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_aldo2(int set_vol)
{
	u8 gate;
	u8 vol_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL1, &gate))
    {
        return -1;
    }
	if(!set_vol)
	{
	    gate &= ~(1<<7);
	}
	else
	{
		gate |=  (1<<7);
		if((set_vol < 700) || (set_vol > 3300))
		{
			return -1;
		}
		if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_ALDO2OUT_VOL, &vol_value))
	    {
	        return -1;
	    }
	    vol_value &= 0x1f;
		vol_value |= ((set_vol - 700)/100);
	    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_ALDO2OUT_VOL, vol_value))
	    {
	        return -1;
	    }
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL1, gate))
    {
    	printf("sunxi pmu error : unable to set aldo2\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_aldo3(int set_vol)
{
	u8 gate;
	u8 vol_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL3, &gate))
    {
        return -1;
    }
	if(!set_vol)
	{
	    gate &= ~(1<<7);
	}
	else
	{
		gate |=  (1<<7);
		if((set_vol < 700) || (set_vol > 3300))
		{
			return -1;
		}
		if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_ALDO3OUT_VOL, &vol_value))
	    {
	        return -1;
	    }
	    vol_value &= 0x1f;
		vol_value |= ((set_vol - 700)/100);
	    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_ALDO3OUT_VOL, vol_value))
	    {
	        return -1;
	    }
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL3, gate))
    {
    	printf("sunxi pmu error : unable to set aldo3\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dldo1(int on_off)
{
	u8 gate;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, &gate))
    {
        return -1;
    }
	if(!on_off)
	{
	    gate &= ~(1<<3);
	}
	else
	{
		gate |=  (1<<3);
		if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DLDO1_VOL, 0x0b))
		{
			printf("sunxi pmu error : unable to set dldo1\n");

			return -1;
		}
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, gate))
    {
    	printf("sunxi pmu error : unable to set dldo1\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dldo2(int on_off)
{
	u8 gate;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, &gate))
    {
        return -1;
    }
	if(!on_off)
	{
	    gate &= ~(1<<4);
	}
	else
	{
		gate |=  (1<<4);
		if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DLDO2_VOL, 0x0b))
		{
			printf("sunxi pmu error : unable to set dldo2\n");

			return -1;
		}
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, gate))
    {
    	printf("sunxi pmu error : unable to set dldo2\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dldo3(int on_off)
{
	u8 gate;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, &gate))
    {
        return -1;
    }
	if(!on_off)
	{
	    gate &= ~(1<<5);
	}
	else
	{
		gate |=  (1<<5);
		if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DLDO3_VOL, 0x0b))
		{
			printf("sunxi pmu error : unable to set dldo3\n");

			return -1;
		}
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, gate))
    {
    	printf("sunxi pmu error : unable to set dldo3\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_dldo4(int on_off)
{
	u8 gate;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, &gate))
    {
        return -1;
    }
	if(!on_off)
	{
	    gate &= ~(1<<6);
	}
	else
	{
		gate |=  (1<<6);
		if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_DLDO4_VOL, 0x0b))
		{
			printf("sunxi pmu error : unable to set dldo4\n");

			return -1;
		}
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, gate))
    {
    	printf("sunxi pmu error : unable to set dldo4\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_eldo1(int on_off)
{
	u8 gate;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, &gate))
    {
        return -1;
    }
	if(!on_off)
	{
	    gate &= ~(1<<0);
	}
	else
	{
		gate |=  (1<<0);
		if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_ELDO1_VOL, 0x0b))
		{
			printf("sunxi pmu error : unable to set eldo1\n");

			return -1;
		}
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, gate))
    {
    	printf("sunxi pmu error : unable to set eldo2\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_eldo2(int on_off)
{
	u8 gate;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, &gate))
    {
        return -1;
    }
	if(!on_off)
	{
	    gate &= ~(1<<1);
	}
	else
	{
		gate |=  (1<<1);
		if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_ELDO1_VOL, 0x0b))
		{
			printf("sunxi pmu error : unable to set eldo1\n");

			return -1;
		}
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, gate))
    {
    	printf("sunxi pmu error : unable to set eldo2\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_eldo3(int on_off)
{
	u8 gate;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, &gate))
    {
        return -1;
    }
	if(!on_off)
	{
	    gate &= ~(1<<2);
	}
	else
	{
		gate |=  (1<<2);
		if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_ELDO3_VOL, 0x0b))
		{
			printf("sunxi pmu error : unable to set eldo3\n");

			return -1;
		}
	}
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OUTPUT_CTL2, gate))
    {
    	printf("sunxi pmu error : unable to set eldo3\n");
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_gpio1ldo(int onoff, int set_vol)
{
	u8 reg_value;

	if(onoff)
	{
		if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_GPIO1_VOL, &reg_value))
	    {
	        return -1;
	    }
	    reg_value &= 0xf0;
	    if(set_vol < 700)
	    {
	    	set_vol = 700;
	    }
	    else if(set_vol > 3300)
	    {
	    	set_vol = 3300;
	    }
	    reg_value |= ((set_vol - 700)/100) & 0x0f;
	    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_GPIO1_VOL, reg_value))
	    {
	        return -1;
	    }

		if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_GPIO1_CTL, &reg_value))
	    {
	        return -1;
	    }
	    reg_value &= ~7;
	    reg_value |=  3;
	    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_GPIO1_CTL, reg_value))
	    {
	        return -1;
	    }
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_power_off(void)
{
    u8 reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_OFF_CTL, &reg_value))
    {
        return -1;
    }
    reg_value |= 1 << 7;
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_OFF_CTL, reg_value))
    {
        return -1;
    }

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_poweroff_vol(int set_vol)
{
	u8 reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_VOFF_SET, &reg_value))
    {
        return -1;
    }
	reg_value &= 0xf8;
	if(set_vol >= 2600 && set_vol <= 3300)
	{
		reg_value |= (set_vol - 2600)/100;
	}
	else if(set_vol <= 2600)
	{
		reg_value |= 0x00;
	}
	else
	{
		reg_value |= 0x07;
	}
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_VOFF_SET, reg_value))
    {
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_charge_current(int current)
{
	u8   reg_value;
	int  step;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_CHARGE1, &reg_value))
    {
        return -1;
    }
	reg_value &= ~0x0f;
	if(current > 2550)
	{
		current = 2550;
	}
	else if(current < 300)
	{
		current = 300;
	}
	step       = (current/150) - 2;
	reg_value |= (step & 0x0f);

	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_CHARGE1, reg_value))
    {
        return -1;
    }

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_charge_current(void)
{
	__u8  reg_value;
	int	  current;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_CHARGE1, &reg_value))
    {
        return -1;
    }
	reg_value &= 0x0f;
	current = (reg_value + 2) * 150;

	return current;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_charge_status(void)
{
	__u8  reg_value;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_INTSTS2, &reg_value))
    {
        return -1;
    }
	return (reg_value >> 2) & 0x01;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_vbus_cur_limit(int current)
{
	__u8 reg_value;

	//set bus current limit off
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_IPS_SET, &reg_value))
    {
        return -1;
    }
    reg_value &= 0xfC;
	if(!current)
	{
	    reg_value |= 0x03;
	}
	else if(current <= 500)		//limit to 500
	{
		reg_value |= 0x01;
	}
	else						//limit to 900
	{
		reg_value |= 0;
	}
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_IPS_SET, reg_value))
    {
        return -1;
    }

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_vbus_vol_limit(int vol)
{
	__u8 reg_value;

	//set bus vol limit off
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_IPS_SET, &reg_value))
    {
        return -1;
    }
    reg_value &= ~(7 << 3);
	if(!vol)
	{
	    reg_value &= ~(1 << 6);
	}
	else
	{
		if(vol < 4000)
		{
			vol = 4000;
		}
		else if(vol > 4700)
		{
			vol = 4700;
		}
		reg_value |= ((vol-4000)/100) << 3;
	}
	if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_IPS_SET, reg_value))
    {
        return -1;
    }

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_chgcur(int cur)
{
	__u8 reg_value;

	if(cur < 300)
	{
		cur = 300;
	}
	else if(cur > 2550)
	{
		cur = 2550;
	}
	//set charge current
	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_CHARGE1, &reg_value))
    {
        return -1;
    }
    reg_value &= 0xf0;
    reg_value |= (((cur - 300)/150) & 0x0f);
    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_CHARGE1, reg_value))
    {
        return -1;
    }

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_rest_battery_capacity(void)
{
	u8  reg_value;
	int bat_rest;

	if(axp221_probe_charge_status() == 1)
	{
		return 100;
	}
    if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_COULOMB_CAL, &reg_value))
    {
        return -1;
    }
	bat_rest = reg_value & 0x7F;
    return bat_rest;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_int_query(__u8 *addr)
{
	int   i;
	__u8  int_reg = BOOT_POWER22_INTSTS1;

	for(i=0;i<5;i++)
	{
	    if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_INTSTS1 + i, addr + i))
	    {
	        return -1;
	    }
	}

	for(i=0;i<5;i++)
	{
	    if(pm_bus_write(AXP22_ADDR, BOOT_POWER22_INTSTS1 + i, 0xff))
	    {
	        return -1;
	    }
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_int_enable_read(__u8 *addr)
{
	int   i;
	__u8  int_reg = BOOT_POWER22_INTEN1;

	for(i=0;i<5;i++)
	{
		if(pm_bus_read(AXP22_ADDR, int_reg, addr + i))
	    {
	        return -1;
	    }
	    int_reg ++;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_int_enable_write(__u8 *addr)
{
	int   i;
	__u8  int_reg = BOOT_POWER22_INTEN1;

	for(i=0;i<5;i++)
	{
		if(pm_bus_write(AXP22_ADDR, int_reg, addr[i]))
	    {
	        return -1;
	    }
	    int_reg ++;
	}

	return 0;
}


DECLARE_GLOBAL_DATA_PTR;
#if 0
extern int axp_probe_power_supply_condition(void);
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp_probe(void)
{
	u8    pmu_type;

	if(pm_bus_read(AXP22_ADDR, BOOT_POWER22_VERSION, &pmu_type))
	{
		printf("axp read error\n");

		return -1;
	}
	pmu_type &= 0x0f;
	if(pmu_type & 0x06)
	{
		/* pmu type AXP221 */
		tick_printf("PMU: AXP221\n");

		return 0;
	}

	tick_printf("PMU: NULL\n");

	return -1;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp_probe_startup_cause(void)
{
	int ret = -1;
	int buffer_value;
	int status;
	int poweron_reason, next_action = 0;

	buffer_value = axp221_probe_last_poweron_status();
	debug("axp buffer %x\n", buffer_value);
	if(buffer_value < 0)
	{
		return -1;
	}
    if(buffer_value == 0x0e)		//表示前一次是在系统状态，下一次应该也进入系统
    {
    	tick_printf("pre sys mode\n");
    	return -1;
    }
    else if(buffer_value == 0x0f)      //表示前一次是在boot standby状态，下一次也应该进入boot standby
	{
		tick_printf("pre boot mode\n");
		status = axp221_probe_power_status();
    	if(status <= 0)
    	{
    		return 0;
    	}
    	else if(status == 2)	//only vbus exist
    	{
    		return 2;
    	}
    	else if(status == 3)	//dc exist(dont care wether vbus exist)
    	{
    		return 3;
    	}
		return 0;
	}
	//获取 开机原因，是按键触发，或者插入电压触发
	poweron_reason = axp221_probe_poweron_cause();
	if(poweron_reason == AXP_POWER_UP_TRIGGER_BY_KEY)
	{
		tick_printf("key trigger\n");
		next_action = 0x0e;
		ret = 0;
	}
	else if(poweron_reason == AXP_POWER_UP_TRIGGER_BY_POWER)
	{
		tick_printf("power trigger\n");
		next_action = 0x0f;
    	ret = 1;
	}
	//把开机原因写入寄存器
	axp221_set_next_poweron_status(next_action);

    return ret;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_power_off(void)
{
	return axp221_set_power_off();
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp_set_next_poweron_status(int value)
{
	return axp221_set_next_poweron_status(value);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_power_get_dcin_battery_exist(int *dcin_exist, int *battery_exist)
{
	*dcin_exist = axp221_probe_dcin_exist();
	*battery_exist = axp221_probe_battery_exist();

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_probe_battery_vol(void)
{
	return axp221_probe_battery_vol();
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_probe_rest_battery_capacity(void)
{
	return axp221_probe_battery_ratio();
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_probe_key(void)
{
	return axp221_probe_key();
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_dc1sw(int on_off)
{
	return axp221_set_dc1sw(on_off);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_dcdc1(int set_vol)
{
	return axp221_set_dcdc1(set_vol);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_dcdc2(int set_vol)
{
	return axp221_set_dcdc2(set_vol);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_dcdc3(int set_vol)
{
	return axp221_set_dcdc3(set_vol);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_dcdc4(int set_vol)
{
	return axp221_set_dcdc4(set_vol);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_dcdc5(int set_vol)
{
	return axp221_set_dcdc5(set_vol);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_eldo3(int on_off)
{
	return axp221_set_eldo3(on_off);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_dldo3(int on_off)
{
	return axp221_set_dldo3(on_off);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_probe_charge_current(void)
{
	return axp221_probe_charge_current();
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_charge_current(int current)
{
	return axp221_set_charge_current(current);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int  axp_set_charge_control(void)
{
	return axp221_set_charge_control();
}
#endif

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_vbus_limit_dc(void)
{
	axp221_set_vbus_vol_limit(gd->limit_vol);
	axp221_set_vbus_cur_limit(gd->limit_cur);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_vbus_limit_pc(void)
{
	axp221_set_vbus_vol_limit(gd->limit_pcvol);
	axp221_set_vbus_cur_limit(gd->limit_pccur);

	return 0;
}




/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_suspend_chgcur(void)
{
	return axp221_set_chgcur(gd->pmu_suspend_chgcur);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_runtime_chgcur(void)
{
	return axp221_set_chgcur(gd->pmu_runtime_chgcur);
}




/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_power_supply_output(void)
{
	int vol_value;
	int cpus_sel;

	//set dcdc1
	if(!script_parser_fetch("power_sply", "dcdc1_vol", &vol_value, 1))
	{
		if(!axp_set_dcdc1(vol_value))
		{
			printf("dcdc1 %d\n", vol_value);
		}
	}
	//set dcdc2
	if(!script_parser_fetch("power_sply", "dcdc2_vol", &vol_value, 1))
	{
		if(!axp_set_dcdc2(vol_value))
		{
			printf("dcdc2 %d\n", vol_value);
		}
	}
#ifdef DEBUG
	else
	{
		printf("boot power:unable to find dcdc2 set\n");
	}
#endif
	//set dcdc4
	if(!script_parser_fetch("power_sply", "dcdc4_vol", &vol_value, 1))
	{
		if(!axp_set_dcdc4(vol_value))
		{
			printf("dcdc4 %d\n", vol_value);
		}
	}
#ifdef DEBUG
	else
	{
		printf("boot power:unable to find dcdc4 set\n");
	}
#endif
    //set dcdc5
	if(!script_parser_fetch("power_sply", "dcdc5_vol", &vol_value, 1))
	{
		if(!axp_set_dcdc5(vol_value))
		{
			printf("dcdc5 %d\n", vol_value);
		}
	}
#ifdef DEBUG
	else
	{
		printf("boot power:unable to find dcdc5 set\n");
	}
#endif
#if 0
	//set ldo2
	if(!script_parser_fetch("power_sply", "ldo2_vol", &vol_value, 1))
	{
		if(!axp_set_ldo2(vol_value))
		{
			printf("boot power:set ldo2 to %d ok\n", vol_value);
		}
	}
#ifdef DEBUG
	else
	{
		printf("boot power:unable to find ldo2 set\n");
	}
#endif
	//set ldo3
	if(!script_parser_fetch("power_sply", "ldo3_vol", &vol_value, 1))
	{
		if(!axp_set_ldo3(vol_value))
		{
			printf("boot power:set ldo3 to %d ok\n", vol_value);
		}
	}
#ifdef DEBUG
	else
	{
		printf("boot power:unable to find ldo3 set\n");
	}
#endif
#endif
	//set ldo4
//	if(!script_parser_fetch("power_sply", "ldo4_vol", &vol_value, 1))
//	{
//		axp_set_ldo4(vol_value);
//	}
//	else
//	{
//		printf("boot power:unable to find ldo4 set\n");
//	}
	if(!script_parser_fetch("power_sply", "cpus_source", &cpus_sel, 1))
	{
		if(cpus_sel)
		{
			if(!script_parser_fetch("power_sply", "gpio1ldo_vol", &vol_value, 1))
			{
				if(!axp221_set_gpio1ldo(cpus_sel, vol_value))
				{
					printf("boot power:set gpio1ldo to %d ok\n", vol_value);
				}
			}
		}
	}

	return 0;
}



/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_set_hardware_poweron_vol(int vol) //设置开机之后，PMU硬件关机电压为2.9V
{
	int vol_value;

    if(vol<=0)
    {

    	if(script_parser_fetch(PMU_SCRIPT_NAME, "pmu_pwron_vol", &vol_value, 1))
    	{
    		printf("boot power:unable to find power off vol set\n");
    		vol_value = 2900;
    	}

    }
    else
    {
        vol_value = vol;
    }
	return axp221_set_poweroff_vol(vol_value);
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/

int axp221_set_hardware_poweroff_vol(int vol) //设置关机之后，PMU硬件下次开机电压为3.3V
{
	int vol_value;

    if (vol <=0)
    {
    	if(script_parser_fetch(PMU_SCRIPT_NAME, "pmu_pwroff_vol", &vol_value, 1))
    	{
    		printf("boot power:unable to find power off vol set\n");
    		vol_value = 3300;
    	}
    }
    else
    {
        vol_value=vol;
    }
	return axp221_set_poweroff_vol(vol_value);
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_probe_power_supply_condition(void)
{
	int   dc_exist,vbus_exist, bat_vol;
	//int   buffer_value;
	//char  bat_value, bat_cou;
	int   ratio;

	//检测电压，决定是否开机
    dc_exist = axp221_probe_ac_exist();
    vbus_exist = axp221_probe_vbus_exist();
#ifdef DEBUG
    printf("dc_exist = %d,vbus_exist=%d\n", dc_exist,vbus_exist);
#endif
    //先判断条件，如果上次关机记录的电量百分比<=5%,同时库仑计值小于5mAh，则关机，否则继续判断
    if(!dc_exist && !vbus_exist)
    {
    	bat_vol = axp221_probe_battery_vol();
		tick_printf("PMU: bat vol = %d\n", bat_vol);
		if(bat_vol < 3400)
		{
			printf("bat vol is lower than 3400 and dcin is not exist\n");
			printf("we have to close it\n");
		    axp221_set_hardware_poweroff_vol(-1);
			axp221_set_power_off();
			for(;;);
		}
    }

	ratio = axp221_probe_battery_ratio();
	tick_printf("PMU: bat ratio = %d\n", ratio);
	if(ratio < 0)
	{
		return -1;
	}
	else if(ratio < 1)
	{
		if(dc_exist || vbus_exist)
		{
			//外部电源存在，电池电量极低
			gd->power_step_level = BATTERY_RATIO_TOO_LOW_WITH_DCIN;
		}
		else
		{
			//外部电源不存在，电池电量极低
			gd->power_step_level = BATTERY_RATIO_TOO_LOW_WITHOUT_DCIN;
		}
	}
	else
	{
		gd->power_step_level = BATTERY_RATIO_ENOUGH;
	}

	return 0;
}




/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
static __u8 power_int_value[8];

int axp221_int_enable(int int_mask)
{
    __u8 int_value[8];
    int i;
    for(i=0;i<sizeof(int_value);i++)
    {
        int_value[i] = 0x00;
    }
    //back up the default int mask.
	axp221_int_enable_read(power_int_value);

    if(int_mask & AXP_INT_MASK_AC_REMOVE)       int_value[0] |= 1<<5;
    if(int_mask & AXP_INT_MASK_AC_INSERT)       int_value[0] |= 1<<6;
    if(int_mask & AXP_INT_MASK_VBUS_REMOVE)     int_value[0] |= 1<<2;
    if(int_mask & AXP_INT_MASK_VBUS_INSERT)     int_value[0] |= 1<<3;
    if(int_mask & AXP_INT_MASK_CHARGE_DONE)     int_value[1] |= 1<<2;
    if(int_mask & AXP_INT_MASK_LONG_KEY_PRESS)  int_value[2] |= 1<<0;
    if(int_mask & AXP_INT_MASK_SHORT_KEY_PRESS) int_value[2] |= 1<<1;
	axp221_int_enable_write(int_value);
	//打开小cpu的中断使能
	*(volatile unsigned int *)(0x01f00c00 + 0x10) |= 1;
	*(volatile unsigned int *)(0x01f00c00 + 0x40) |= 1;

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_int_disable(int int_mask)
{
	*(volatile unsigned int *)(0x01f00c00 + 0x10) |= 1;
	*(volatile unsigned int *)(0x01f00c00 + 0x40) &= ~1;
	return axp221_int_enable_write(power_int_value);
}


/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp221_int_get_status(int *int_mask)
{
	int ret;
    __u8 int_value[8];
	ret = axp221_int_query(int_value);
	*(volatile unsigned int *)(0x01f00c00 + 0x10) |= 1;

    *int_mask=0;

    if(int_value[0] &= 1<<5)    *int_mask |=AXP_INT_MASK_AC_REMOVE;
    if(int_value[0] &= 1<<6)    *int_mask |=AXP_INT_MASK_AC_INSERT;
    if(int_value[0] &= 1<<2)    *int_mask |=AXP_INT_MASK_VBUS_REMOVE;
    if(int_value[0] &= 1<<3)    *int_mask |=AXP_INT_MASK_VBUS_INSERT;
    if(int_value[1] &= 1<<2)    *int_mask |=AXP_INT_MASK_CHARGE_DONE;
    if(int_value[2] &= 1<<0)    *int_mask |=AXP_INT_MASK_LONG_KEY_PRESS;
    if(int_value[2] &= 1<<1)    *int_mask |=AXP_INT_MASK_SHORT_KEY_PRESS;

	return ret;
}

void parse_axp221_config_para(void)
{

    int ret1,ret2;
    int usbvol_limit = 0;
    int usbcur_limit = 0;

    ret1 = script_parser_fetch(PMU_SCRIPT_NAME, "pmu_runtime_chgcur", (int *)&gd->pmu_runtime_chgcur, 1);
    ret2 = script_parser_fetch(PMU_SCRIPT_NAME, "pmu_suspend_chgcur", (int *)&gd->pmu_suspend_chgcur, 1);

	if(ret1)
	{
		gd->pmu_runtime_chgcur = 600;
	}
	if(ret2)
	{
		gd->pmu_suspend_chgcur = 1500;
	}

    #if DEBUG
	printf("pmu_runtime_chgcur=%d\n", gd->pmu_runtime_chgcur);
	printf("pmu_suspend_chgcur=%d\n", gd->pmu_suspend_chgcur);
    #endif

    script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbvol_limit", &usbvol_limit, 1);
    script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbcur_limit", &usbcur_limit, 1);
    script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbvol", (int *)&gd->limit_vol, 1);
    script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbcur", (int *)&gd->limit_cur, 1);
    script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbvol_pc", (int *)&gd->limit_pcvol, 1);
    script_parser_fetch(PMU_SCRIPT_NAME, "pmu_usbcur_pc", (int *)&gd->limit_pccur, 1);

    #ifdef DEBUG
    printf("usbvol_limit = %d, limit_vol = %d\n", usbvol_limit, gd->limit_vol);
    printf("usbcur_limit = %d, limit_cur = %d\n", usbcur_limit, gd->limit_cur);
    #endif

    if(!usbvol_limit)
    {
        gd->limit_vol = 0;

    }
    if(!usbcur_limit)
    {
        gd->limit_cur = 0;
    }

}

int axp221_init(struct axp_power *axp_power_instance)
{

    int ret = -1;

	int dcdc3_vol;

	ret = script_parser_fetch("power_sply", "dcdc3_vol", &dcdc3_vol, 1);
	if(ret)
	{
		dcdc3_vol = 1200;
	}
#ifdef DEBUG
	printf("set dcdc3 to %d\n", dcdc3_vol);
#endif

	if(!axp221_probe_power_supply_condition())
	{
		if(!axp_set_dcdc3(dcdc3_vol))
		{
			tick_printf("PMU: dcdc3 %d\n", dcdc3_vol);

			ret = 0;
		}
		else
		{
			printf("axp_set_dcdc3 fail\n");
		}

	}
	else
	{
		printf("axp_probe_power_supply_condition error\n");
	}

    if(ret<0)
    {
        return ret;
    }

    parse_axp221_config_para();
    axp221_set_runtime_chgcur();
    axp221_set_vbus_limit_pc();
    axp221_set_hardware_poweron_vol(-1);
    axp221_set_power_supply_output();

	return ret;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：axp221_ioctl
*
*    参数列表：
*
*    返回值  ：reuturn >0 if success, -1 if failed.
*
*    说明    :ioctl the axp221.
*
*
*
************************************************************************************************************
*/
int axp221_ioctl(struct axp_power *axp_power_instance,int cmd, void *arg)
{

    int arg0=*((int*)arg);

  switch(cmd){

    case AXP_CMD_SET_CPU_CORE_VOLTAGE:

         return axp221_set_dcdc3(arg0);

    case AXP_CMD_SET_POWER_UP_VOLTAGE:

         return axp221_set_hardware_poweron_vol(arg0);

    case AXP_CMD_SET_POWER_OFF_VOLTAGE:

         return axp221_set_hardware_poweroff_vol(arg0);

    case AXP_CMD_GET_POWER_LEVEL:

         return gd->power_step_level;

    //battery operation
    case AXP_CMD_GET_BATTERY_EXISTENCE:

         return axp221_probe_battery_exist();

    case AXP_CMD_GET_BATTERY_CAPACITY:

         return -1;

    case AXP_CMD_GET_BATTERY_REST_VOLUME:

         return axp221_probe_rest_battery_capacity();

    case AXP_CMD_GET_BATTERY_OCV:

         return axp221_probe_battery_vol();

    case AXP_CMD_GET_BATTERY_RDC:

         return axp221_probe_buttery_resistance_record();

    case AXP_CMD_SET_BATTERY_RDC:

         return -1;
    //power up and down votage set


    //charger info and operation
    case AXP_CMD_SET_CHARGER_ENABLE:

         return axp221_set_charge_control(1);

    case AXP_CMD_SET_CHARGER_DISABLE:

         return axp221_set_charge_control(0);

    case AXP_CMD_GET_CHARGER_IS_CHARGING:

         return axp221_get_charger_is_charge();

    case AXP_CMD_GET_CHARGER_CHARGE_CURRENT:

         return axp221_probe_charge_current();

    case AXP_CMD_SET_CHARGER_CHARGE_CURRENT:

         return axp221_set_charge_current(arg0);

    //usb vbus operation
    case AXP_CMD_GET_VBUS_EXISTENCE:

         return axp221_probe_vbus_exist();

    case AXP_CMD_GET_VBUS_CURRENT:

         return -1;

    case AXP_CMD_GET_VBUS_VOLTAGE:

         return -1;

    case AXP_CMD_SET_VBUS_LIMIT_PC:

         return axp221_set_vbus_limit_pc();

    case AXP_CMD_SET_VBUS_LIMIT_DC:

         return axp221_set_vbus_limit_dc();

    //AC bus info
    case AXP_CMD_GET_AC_EXISTENCE:

         return axp221_probe_ac_exist();

    case AXP_CMD_GET_AC_CURRENT:

         return -1;

    case AXP_CMD_GET_AC_VOLTAGE:

         return -1;

    //AXP buffer operation
    case AXP_CMD_CLEAR_ALL_BUFFER:

         return axp221_clear_data_buffer();

    case AXP_CMD_SET_NEXT_MODE:

         return axp221_set_next_poweron_status(arg0);

    case AXP_CMD_GET_PRE_MODE:

         return axp221_probe_last_poweron_status();

    //AXP power key operation
    case AXP_CMD_GET_POWER_KEY_STATUS:

         return axp221_probe_key();

    case AXP_CMD_CLEAR_POWER_KEY_STATUS:

         return axp221_clear_key_status(arg0);

    case AXP_CMD_GET_POWER_UP_TRIGGER:

         return axp221_probe_poweron_cause();
    //AXP gpio operation
    case AXP_CMD_GPIO_REQUEST:
    case AXP_CMD_GPIO_IO_CTL:
    case AXP_CMD_GPIO_GET_VALUE:
    case AXP_CMD_GPIO_SET_VALUE:
         return -1;

    //interrupt
    case AXP_CMD_INT_ENABLE:

         return axp221_int_enable(arg0);
    case AXP_CMD_INT_DISABLE:

         return axp221_int_disable(arg0);
    case AXP_CMD_INT_GET_STATUS:

         return axp221_int_get_status((int*)arg);

    //misc
    case AXP_CMD_POWER_OFF:

         return axp221_set_power_off();

    case AXP_CMD_GET_BUS_ID:

         *((int*)arg)= axp_power_instance->bus_id;
         return 0;

    case AXP_CMD_GET_BUS_TYPE:

         *((int*)arg)= axp_power_instance->bus_type;
         return 0;
    case AXP_CMD_GET_IRQ_NO:

         *((int*)arg)= axp_power_instance->irq_no;
         return 0;
    case AXP_CMD_GET_NAME:

         return 0;
    default:

         return -1;

  }
    return 0;
}

int axp221_exit(struct axp_power *axp_power_instance)
{
    //exit the axp209 before boot to kernel.
    //return >0 if success , -1 if failed.
    axp221_int_disable(0);
    axp221_set_runtime_chgcur();
    axp221_set_vbus_limit_pc();
    return 0;
}

axp_power_register(axp221,AXP_BUS_TYPE_P2WI,0,0,\
                   axp221_init,axp221_ioctl,axp221_exit,0);

