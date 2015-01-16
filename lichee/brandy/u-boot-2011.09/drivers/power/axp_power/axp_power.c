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

DECLARE_GLOBAL_DATA_PTR;

//static struct axp_power* this_power;


//*****************************************************************************
//没有实现的power驱动占位函数。
//*****************************************************************************
int axp_null_init(struct axp_power *axp_power_instance)
{
    //do some initialize for axp209
    //reuturn 0 if success, -1 if failed.
    return 0;
}

int axp_null_ioctl(struct axp_power *axp_power_instance,int cmd, void *arg)
{
    //ioctl the axp209.
    //return 0 if success, -1 if failed.
    return 0;
}
int axp_null_exit(struct axp_power *axp_power_instance)
{
    //exit the axp209 before boot to kernel.
    //return 0 if success , -1 if failed.
    return 0;
}

axp_power_register(axpnull,AXP_BUS_TYPE_I2C,0,0,\
                   axp_null_init,axp_null_ioctl,axp_null_exit,0);


//*****************************************************************************
//定义实例
//*****************************************************************************

extern struct axp_power axp_power_instance_axp209;
extern struct axp_power axp_power_instance_axp221;

static struct axp_power *axp_power_instance[AXP_POWER_INSTANCE_MAX]=
{
	&axp_power_instance_axpnull,     //axp152
	&axp_power_instance_axp209,	  	 //axp209
	&axp_power_instance_axp221,	  	 //axp221
	&axp_power_instance_axpnull,     //axp221s
	&axp_power_instance_axpnull,     //axp229
	&axp_power_instance_axpnull,     //axp_null

};


static axp_power_id_t axp_power_probe_id(void)
{
	//一个硬件ID对应一个SOFT ID ，
	// probe正确返回SOFT ID.否则返回-1
	__u8 power_hard_id;
	if(0 > pm_bus_read(0x68,0x03,&power_hard_id))
	{
        printf("axp probe id error!\n");
    }
    power_hard_id &= 0xf;

    if(power_hard_id == 0x01)
    {
        gd->axp_power_soft_id = AXP_POWER_ID_AXP209;
    }else
    if(power_hard_id == 0x06)
    {
        gd->axp_power_soft_id = AXP_POWER_ID_AXP221;
    }else
    {
        gd->axp_power_soft_id = AXP_POWER_ID_NULL;
    }
	return gd->axp_power_soft_id;
}

int axp_power_init(void)
{
	axp_power_id_t power_id;

	//check power id
	power_id = axp_power_probe_id();
	if(power_id<0||power_id>=AXP_POWER_ID_NULL)
	{
		//没有探测到axp power
	    printf("no axp power probed!\n");
        return -1 ;
	}
	//this_power = axp_power_instance[power_id];
	printf("power:%s probed!\n",axp_power_instance[gd->axp_power_soft_id]->name);
    if(axp_power_instance[gd->axp_power_soft_id])
    {
        return axp_power_instance[gd->axp_power_soft_id]->init(axp_power_instance[gd->axp_power_soft_id]);
    }else
    {
        return -1;
    }

}

int axp_power_ioctl(int cmd,void *arg)
{

    if(axp_power_instance[gd->axp_power_soft_id]->ioctl)
    {
        return axp_power_instance[gd->axp_power_soft_id]\
                ->ioctl(axp_power_instance[gd->axp_power_soft_id],cmd,arg);
    }else
    {
        return -1;
    }

}
int axp_power_exit(void)
{

    if(axp_power_instance[gd->axp_power_soft_id]->exit)
    {
        return axp_power_instance[gd->axp_power_soft_id]\
            ->exit(axp_power_instance[gd->axp_power_soft_id]);
    }else
    {
        return -1;
    }

}

 inline int pm_bus_read(unsigned char chip, unsigned char addr, unsigned char* data)
{
    #ifdef CONFIG_PM_BUS_P2WI
	return p2wi_read(&addr, data, 1);
    #elif  CONFIG_PM_BUS_I2C
    return i2c_read(0,&addr, data, 1);
    #elif  CONFIG_PM_BUS_RSB
    return rsb_write(0,&addr, data, 1);
    #else
    return 0;
    //#error no valid pm bus configed
    #endif
}

inline int pm_bus_write(unsigned char chip, unsigned char addr, unsigned char data)
{
    #ifdef CONFIG_PM_BUS_P2WI
   	return p2wi_write(&addr, &data, 1);
    #elif  CONFIG_PM_BUS_I2C
    return i2c_write(0,&addr, &data, 1);
    #elif  CONFIG_PM_BUS_RSB
    return rsb_write(0,&addr, &data, 1);
    #else
    return 0;
    //#error no valid pm bus configed
    #endif
}

