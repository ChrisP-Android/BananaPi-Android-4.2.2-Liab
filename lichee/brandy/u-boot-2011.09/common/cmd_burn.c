/*
 * Copyright 2008 Freescale Semiconductor, Inc.
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
#include <config.h>
#include <command.h>
#include <sunxi_mbr.h>
#include <boot_type.h>
#include <sprite.h>
#include <sunxi_usb.h>

extern  void sunxi_usb_main_loop(int delaytime);


uint burn_private_start, burn_private_len;

int do_burn_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char mbr_buffer[SUNXI_MBR_SIZE * SUNXI_MBR_COPY_NUM];
	sunxi_mbr_t  *mbr;
	int  i;

	printf("usb burn test\n");

	if(!sunxi_sprite_read(0, SUNXI_MBR_SIZE * SUNXI_MBR_COPY_NUM/512, mbr_buffer))
	{
		printf("usb burn fail: unable to read mbr on local flash\n");

		return -1;
	}
	if(sunxi_sprite_verify_mbr(mbr_buffer))
	{
		printf("usb burn fail: the mbr on local flash is invalid\n");

		return -1;
	}
	mbr = (sunxi_mbr_t *)mbr_buffer;
	for(i=0;i<mbr->PartCount;i++)
	{
		if(!strcmp((const char *)mbr->array[i].name, "private"))
		{
			printf("private part exist\n");

			burn_private_start = mbr->array[i].addrlo;
			burn_private_len   = mbr->array[i].lenlo;

			break;
		}
	}
	if(!burn_private_len)
	{
		printf("there is not private exist\n");

		return -1;
	}
	if(sunxi_usb_dev_register(4) < 0)
	{
		printf("usb burn fail: not support burn private data\n");

		return -1;
	}
	sunxi_usb_main_loop(0);

	return 0;
}



U_BOOT_CMD(
	pburn, CONFIG_SYS_MAXARGS, 1, do_burn_test,
	"do a burn test",
	"pburn [mode]"
	"default 0, self burn"
	"mode=1: sprite burn"
	"NULL"
);
