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
#include <common.h>
#include <power/axp20_reg.h>
#include "axp.h"
#include <pmu.h>

extern int axp20_set_supply_status(int vol_name, int vol_value, int onoff);
extern int axp20_set_supply_status_byname(char *vol_name, int vol_value, int onoff);
extern int axp20_probe_supply_status(int vol_name, int vol_value, int onoff);
int axp20_probe_battery_vol(void);
__s32 axp20_set_power_on_vol(void);
__s32 axp20_set_power_off_vol(void);

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
static __s32 axp20_clr_status(void)
{
	__u8  reg_addr1,value1;

	reg_addr1 = BOOT_POWER20_DATA_BUFFER0;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER1;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER2;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER3;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER4;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER5;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER6;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER7;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER8;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER9;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER10;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

	reg_addr1 = BOOT_POWER20_DATA_BUFFER11;
	value1 = 0x00;
	if(axp_i2c_write(AXP20_ADDR, reg_addr1, value1))
	{
		return -1;
	}

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
__s32 axp20_power_enable_coulomb(void)
{
    __u8   reg_addr;
    __u8   tmp_value;

	reg_addr = BOOT_POWER20_COULOMB_CTL;
	tmp_value = 0x80;
	if(axp_i2c_write(AXP20_ADDR, reg_addr, tmp_value))
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
int axp20_probe(void)
{
	u8    pmu_type;
	u8    reg_addr,value;
    __msdelay(100);//确保硬件ADC准备好。
	if(axp_i2c_read(AXP20_ADDR, BOOT_POWER20_VERSION, &pmu_type))
	{
		printf("axp read error\n");

		return -1;
	}
	pmu_type &= 0x0f;
	if(pmu_type & 0x01)
	{
		/* pmu type AXP209 */
		tick_printf("PMU: AXP209\n");
		 //为兼容新旧版本IC，取消PMU硬件检测RDC功能
		reg_addr = BOOT_POWER20_COULOMB_CAL;
    	axp_i2c_read(AXP20_ADDR, reg_addr, &value);
    	value |= 0x80;//pouse ocv.
    	axp_i2c_write(AXP20_ADDR, reg_addr, value);
    	reg_addr = BOOT_POWER20_RDC1;
    	axp_i2c_read(AXP20_ADDR, reg_addr, &value);
    	value &= 0x7F;//disable rdc calculation
    	axp_i2c_write(AXP20_ADDR, reg_addr, value);
		reg_addr = BOOT_POWER20_COULOMB_CAL;
    	axp_i2c_read(AXP20_ADDR, reg_addr, &value);
    	value &= 0x7F;//start ocv
    	axp_i2c_write(AXP20_ADDR, reg_addr, value);
		reg_addr = BOOT_POWER20_COULOMB_CTL;
		if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
		{
	    	return -1;
		}
		if(!(value & 0x80)) // coulomb conuter on off control
		{
			axp20_clr_status(); //clear the data buffer
		}
		axp20_power_enable_coulomb();

		// open bat acin vbus voltage and current adc
		reg_addr = BOOT_POWER20_ADC_EN1;
	    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
	    {
	        return -1;
	    }
	    value |= 0xfe << 0;
	    reg_addr = BOOT_POWER20_ADC_EN1;
	    if(axp_i2c_write(AXP20_ADDR, reg_addr, value))
	    {
	        return -1;
	    }
	    //Enable Charge
	    reg_addr = BOOT_POWER20_CHARGE1;
	    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
	    {
	        return -1;
	    }
	    value |= 0x80 << 0;
	    reg_addr = BOOT_POWER20_CHARGE1;
	    if(axp_i2c_write(AXP20_ADDR, reg_addr, value))
	    {
	        return -1;
	    }
	    //set bus current limit off
	    reg_addr = BOOT_POWER20_IPS_SET;
	    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
	    {
	        return -1;
	    }
	    value &= 0xfC;
	    value |= 0x03;
	    reg_addr = BOOT_POWER20_IPS_SET;
	    if(axp_i2c_write(AXP20_ADDR, reg_addr, value))
	    {
	        return -1;
	    }
	    {
	    	__u8 coulomb_val, tmp_val;
	        //hold
	        reg_addr = BOOT_POWER20_COULOMB_CAL;
	        if(axp_i2c_read(AXP20_ADDR, reg_addr, &coulomb_val))
	        {
	            return -1;
	        }
	        coulomb_val |= 0x80;
	        if(axp_i2c_write(AXP20_ADDR, reg_addr, coulomb_val))
	        {
	            return -1;
	        }
	        //
	        reg_addr = 0xBA;
	        if(axp_i2c_read(AXP20_ADDR, reg_addr, &tmp_val))
	        {
	            return -1;
	        }
	        tmp_val &= ~0x80;
	        if(axp_i2c_write(AXP20_ADDR, reg_addr, tmp_val))
	        {
	            return -1;
	        }
	        reg_addr = BOOT_POWER20_COULOMB_CAL;
	        coulomb_val &= ~0x80;
	        if(axp_i2c_write(AXP20_ADDR, reg_addr, coulomb_val))
	        {
	            return -1;
	        }
	    }
		return 0;
	}

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
int axp20_set_charge_control(void)
{
	u8 reg_value;
	//disable ts adc, enable all other adc
	if(axp_i2c_read(AXP20_ADDR, BOOT_POWER20_ADC_EN1, &reg_value))
    {
        return -1;
    }
    reg_value |= 0xfe;
    if(axp_i2c_write(AXP20_ADDR, BOOT_POWER20_ADC_EN1, reg_value))
    {
        return -1;
    }
    //enable charge
	if(axp_i2c_read(AXP20_ADDR, BOOT_POWER20_CHARGE1, &reg_value))
    {
        return -1;
    }
    reg_value |= 0x80;
    if(axp_i2c_write(AXP20_ADDR, BOOT_POWER20_CHARGE1, reg_value))
    {
        return -1;
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
int axp20_probe_battery_exist(void)
{
	u8 reg_value;

	if(axp_i2c_read(AXP20_ADDR, BOOT_POWER20_MODE_CHGSTATUS, &reg_value))
    {
        return -1;
    }
	printf("battery: reg value=%x \n",reg_value);
	if(reg_value & 0x10)
	{
		return (reg_value & 0x20);
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
int axp20_probe_battery_ratio(void)
{
	u8 reg_value;
	int vol_threshold=0;

	#ifdef CONFIG_ARCH_HOMELET
		return 100;
	#endif
	if(script_parser_fetch("target", "vol_threshold", &vol_threshold, 1))
	{
		vol_threshold = 3600;
	}
	if(vol_threshold >= axp20_probe_battery_vol())
    {
		return -1;
	}
	if(axp_i2c_read(AXP20_ADDR, BOOT_POWER20_COULOMB_CAL, &reg_value))
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
int axp20_probe_power_status(void)
{
	u8 reg_value;
	if(axp_i2c_read(AXP20_ADDR, BOOT_POWER20_STATUS, &reg_value))
    {
        return -1;
    }
	if(reg_value & 0x40)		//dc in exist
	{
		return AXP_DCIN_EXIST;
	}
	if(reg_value & 0x10)		//vbus exist
	{
		return AXP_VBUS_EXIST;
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
int axp20_probe_battery_vol(void)
{
	u8 reg_addr, value;
	int battery_vol;
    //pmu is AXP209
    reg_addr = BOOT_POWER20_BAT_AVERVOL_H8;
    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
    {
        return -1;
    }

	battery_vol = (value << 4);
    reg_addr = BOOT_POWER20_BAT_AVERVOL_L4;
    if(axp_i2c_write(AXP20_ADDR, reg_addr, value))
    {
        return -1;
    }
    battery_vol += value;
    battery_vol *= 1.1;
	return battery_vol;
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
int axp20_probe_key(void)
{
 	u8  reg_addr, value;
    s32 key_exist;
    key_exist = 0;
    reg_addr = BOOT_POWER20_INTSTS3;
    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
    {
        return -1;
    }
    key_exist = (value >> 0) & 0x03;
    if(key_exist)
    {
    	value |= 0x01 << 1;
    	axp_i2c_write(AXP20_ADDR, reg_addr, value);
	}
    return key_exist;
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
int axp20_probe_pre_sys_mode(void)
{
	u8  reg_value;
	if(axp_i2c_read(AXP20_ADDR, BOOT_POWER20_DATA_BUFFER11, &reg_value))
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
int axp20_set_next_sys_mode(int data)
{
	if(axp_i2c_write(AXP20_ADDR, BOOT_POWER20_DATA_BUFFER11, (u8)data))
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
int axp20_probe_this_poweron_cause(void)
{
    uchar   reg_value;
	if(axp_i2c_read(AXP20_ADDR, BOOT_POWER20_STATUS, &reg_value))
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
int axp20_set_power_off(void)
{
    u8 reg_addr;
    u8  reg_value;

	printf("axp20_set_power_off\n");
	reg_addr = BOOT_POWER20_OFF_CTL;
	if(axp_i2c_read(AXP20_ADDR, reg_addr, &reg_value))
	{
	    printf("axp20_set_power_off read error\n");
    	return -1;
	}
	reg_value |= 1 << 7;
	if(axp_i2c_write(AXP20_ADDR, reg_addr, reg_value))
    {
        return -1;
    }
    printf("axp20_set_power_off failed\n");
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
int axp20_set_power_onoff_vol(int set_vol, int stage)
{
	//tage为0，设置关机之后，PMU硬件下次开机电压为3.3V
	//stage非0，设置开机之后，PMU硬件关机电压为2.9V
    //u8  reg_addr;
    //u8  reg_value;
    //s32 vol_value, ret;	

	if(!set_vol)
	{
		if(!stage)//stage为0
		{
			//set_vol = 3300;	
			axp20_set_power_off_vol();
			return 0;
		}
		else
		{
			//set_vol = 2900;
			axp20_set_power_on_vol();
			return 0;
		}
	}
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
int axp20_set_charge_current(int current)
{
    u8  reg_addr, value;
    int  step;
    reg_addr = BOOT_POWER20_CHARGE1;
    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
    {
        return -1;
    }
    value &= ~0x0f;
    if(current > 1800)
    {
        current = 1800;
    }
    else if(current < 300)
    {
        current = 300;
    }
    step   = (current-300)/100 ;//before is (current/100-3)
    value |= (step & 0x0f);
    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
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
int axp20_probe_charge_current(void)
{
    u8  reg_addr, value;
    int	  current;
    reg_addr = BOOT_POWER20_CHARGE1;
    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
    {
        return -1;
    }
    value &= 0x0f;
    current = (value + 3) * 100;
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
int axp20_set_vbus_cur_limit(int current)
{
	u8 reg_addr, value;
	//set bus current limit off
    reg_addr = BOOT_POWER20_IPS_SET;
    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
    {
        return -1;
    }
    value &= 0xfC;
	if(!current)
	{
	    value |= 0x03;
	}
	else if(current < 500)		//limit to 100
	{
		value |= 0x02;
	}
	else if(current < 900)		//limit to 500
	{
		value |= 0x01;
	}
	else						//limit to 900
	{
		;
	}
	if(axp_i2c_write(AXP20_ADDR, reg_addr, value))
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
int axp20_set_vbus_vol_limit(int vol)
{
	u8 reg_addr, value;
	//set bus current limit off
    reg_addr = BOOT_POWER20_IPS_SET;
    if(axp_i2c_read(AXP20_ADDR, reg_addr, &value))
    {
        return -1;
    }
    value &= ~(7 << 3);
	if(!vol)
	{
	    value &= ~(1 << 6);
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
		value |= ((vol-4000)/100) << 3;
	}
	if(axp_i2c_write(AXP20_ADDR, reg_addr, value))
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
int axp20_probe_int_pending(uchar *addr)
{
    int   i;
    for(i=0;i<5;i++)
    {
        if(axp_i2c_read(AXP20_ADDR, BOOT_POWER20_INTSTS1 + i, addr + i))
        {
            return -1;
        }
    }

    for(i=0;i<5;i++)
    {
        if(axp_i2c_write(AXP20_ADDR, BOOT_POWER20_INTSTS1 + i, 0xff))
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
int axp20_probe_int_enable(uchar *addr)
{
   	int   i;
	uchar  int_reg = BOOT_POWER20_INTEN1;
	for(i=0;i<5;i++)
	{
		if(axp_i2c_read(AXP20_ADDR, int_reg, addr + i))
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
int axp20_set_int_enable(uchar *addr)
{
	int   i;
	uchar  int_reg = BOOT_POWER20_INTEN1;
	for(i=0;i<5;i++)
	{
		if(axp_i2c_write(AXP20_ADDR, int_reg, addr[i]))
	    {
	        return -1;
	    }
	    int_reg ++;
	}
	return 0;
}


sunxi_axp_module_init("axp20", SUNXI_AXP_20X);

int axp20_config_charge_current(int start_time)
{
	int ret;
	int   value;
	script_gpio_set_t  gpio;
	//__u32  gpio_hd;

	//1 代表关机状态
	//0 代表开机状态
	if(start_time == 1)
	{
		ret = script_parser_fetch("pmu_para", "pmu_used2", &value, 1);
		if((ret >= 0) && (value == 1))
		{
			ret = script_parser_fetch("pmu_para", "pmu_adpdet", (void *)&gpio, sizeof(script_gpio_set_t)/sizeof(int));
			if(ret >= 0)
			{
				//gpio_hd = GPIO_Request((void *)&gpio, 1);
				//value = GPIO_Read_One_PIN_Value(gpio_hd, 0);
				if(value > 0)
				{
					ret = script_parser_fetch("pmu_para", "pmu_shutdown_chgcur", &value, 1);
				}
				else
				{
					ret = script_parser_fetch("pmu_para", "pmu_shutdown_chgcur2", &value, 1);
				}
				if(ret >= 0)
				{
					axp20_set_charge_current(value);
					return 0;
				}
			}
		}
		else
		{
			ret = script_parser_fetch("pmu_para", "pmu_shutdown_chgcur", &value, 1);
			if(ret >= 0)
			{
				axp20_set_charge_current(value);

				return 0;
			}
		}
	}
	else
	{
		axp20_set_charge_current(300);
	}

	return -1;
}

__s32 axp20_set_power_on_vol(void) //设置开机之后，PMU硬件关机电压为2.9V
{
	__u8  reg_addr;
	__u8  reg_value;
	__s32 vol_value, ret;

	ret = script_parser_fetch("pmu_para", "pmu_pwron_vol", &vol_value, 1);
	if(ret)
	{
		printf("boot power:unable to find power off vol set\n");
		vol_value = 2900;
	}
	reg_addr = BOOT_POWER20_VOFF_SET;
	if(!axp_i2c_read(AXP20_ADDR, reg_addr, &reg_value))
	{
		reg_value &= 0xf8;
		if(vol_value >= 2600 && vol_value <= 3300)
		{
			reg_value |= (vol_value - 2600)/100;
		}
		else if(vol_value <= 2600)
		{
			reg_value |= 0x00;
		}
		else
		{
			reg_value |= 0x07;
		}
		if(axp_i2c_write(AXP20_ADDR, reg_addr, reg_value))
    	{
			printf("boot power:unable to set voff vol\n");
		}
		else
		{
			return 0;
		}
	}

	return -1;
}

__s32 axp20_set_power_off_vol(void) //设置关机之后，PMU硬件下次开机电压为3.3V
{
	__u8  reg_addr;
	__u8  reg_value;
	__s32 vol_value, ret;

	axp20_config_charge_current(1);//config shutdown charge current
	ret = script_parser_fetch("pmu_para", "pmu_pwroff_vol", &vol_value, 1);
	if(ret)
	{
		printf("boot power:unable to find power on vol set\n");
		vol_value = 3300;
	}
	reg_addr = BOOT_POWER20_VOFF_SET;
	if(!axp_i2c_read(AXP20_ADDR, reg_addr, &reg_value))
	{
		reg_value &= 0xf8;
		if(vol_value >= 2600 && vol_value <= 3300)
		{
			reg_value |= (vol_value - 2600)/100;
		}
		else if(vol_value <= 2600)
		{
			reg_value |= 0x00;  // 2.9v
		}
		else
		{
			reg_value |= 0x07;  // 3.3v
		}
		if(axp_i2c_write(AXP20_ADDR, reg_addr, reg_value))
    	{
			printf("boot power:unable to set power on vol\n");
		}
		else
		{
			//设置VBUS不限流
			//eGon2_power_vbus_cur_limit();
			//设置完成，同时关机
	    	reg_addr = BOOT_POWER20_OFF_CTL;
	    	if(axp_i2c_read(AXP20_ADDR, reg_addr, &reg_value))
	    	{
	        	return -1;
	    	}
	    	reg_value |= 1 << 7;
	    	if(axp_i2c_write(AXP20_ADDR, reg_addr, reg_value))
	        {
	            return -1;
	        }

			return 0;
		}
	}

	return -1;
}

