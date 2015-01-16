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
#include "axp209.h"
int axp209_init(struct axp_power *axp_power_instance)
{
    //do some initialize for axp209
    //reuturn >=0 if success, -1 if failed.
    return 0;
}

int axp209_ioctl(struct axp_power *axp_power_instance,int cmd, void *arg)
{
    //ioctl the axp209.
    //return 0 if success, -1 if failed.
    return 0;
}
int axp209_exit(struct axp_power *axp_power_instance)
{
    //exit the axp209 before boot to kernel.
    //return >=0 if success , -1 if failed.
    return 0;
}


axp_power_register(axp209,AXP_BUS_TYPE_I2C,0,0,\
                   axp209_init,axp209_ioctl,axp209_exit,0);

