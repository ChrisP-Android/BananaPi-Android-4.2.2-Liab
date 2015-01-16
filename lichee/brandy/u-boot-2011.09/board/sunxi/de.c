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
#include <malloc.h>
#include <asm/arch/drv_display.h>
#include <sys_config.h>

DECLARE_GLOBAL_DATA_PTR;


extern __s32 disp_delay_ms(__u32 ms);
int board_display_layer_request(void)
{
	__u32 arg[4];

	arg[0] = 0;
	arg[1] = DISP_LAYER_WORK_MODE_NORMAL;

	gd->layer_hd = disp_ioctl(NULL, DISP_CMD_LAYER_REQUEST, (void*)arg);
	if(gd->layer_hd == 0)
	{
        tick_printf("sunxi display error : display request layer failed\n");

        return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int board_display_layer_release(void)
{
	__u32 arg[4];

	if(gd->layer_hd == 0)
	{
        tick_printf("sunxi display error : display layer is NULL\n");

        return -1;
	}

	arg[0] = 0;
	arg[1] = gd->layer_hd;

	return disp_ioctl(NULL, DISP_CMD_LAYER_RELEASE, (void*)arg);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int board_display_wait_lcd_open(void)
{
	int ret;
	int timedly = 5000;
	int check_time = timedly/50;
	uint arg[4] = { 0 };

	do
	{
    	ret = disp_ioctl(NULL, DISP_CMD_LCD_CHECK_OPEN_FINISH, (void*)arg);
		if(ret == 1)		//open already
		{
			break;
		}
		else if(ret == -1)  //open falied
		{
			return -1;
		}
		__msdelay(50);
		check_time --;
		if(check_time <= 0)
		{
			return -1;
		}
	}
	while(1);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int board_display_wait_lcd_close(void)
{
	int ret;
	int timedly = 5000;
	int check_time = timedly/50;
	uint arg[4] = { 0 };

	do
	{
    	ret = disp_ioctl(NULL, DISP_CMD_LCD_CHECK_CLOSE_FINISH, (void*)arg);
		if(ret == 1)		//open already
		{
			break;
		}
		else if(ret == -1)  //open falied
		{
			return -1;
		}
		__msdelay(50);
		check_time --;
		if(check_time <= 0)
		{
			return -1;
		}
	}
	while(1);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int board_display_set_exit_mode(int lcd_off_only)
{
	uint arg[4] = { 0 };

	if(lcd_off_only)
	{
		arg[0] = DISP_EXIT_MODE_CLEAN_PARTLY;
		disp_ioctl(NULL, DISP_CMD_SET_EXIT_MODE, (void *)arg);
	}
	else
	{
		disp_ioctl(NULL, DISP_CMD_LCD_OFF, (void *)arg);
		board_display_wait_lcd_close();
	}

	return 0;
}
/*
*******************************************************************************
*                     board_display_layer_open
*
* Description:
*    ´ò¿ªÍ¼²ã
*
* Parameters:
*    Layer_hd    :  input. Í¼²ã¾ä±ú
*
* Return value:
*    0  :  ³É¹¦
*   !0  :  Ê§°Ü
*
* note:
*    void
*
*******************************************************************************
*/
int board_display_layer_open(void)
{
    uint arg[4];

	arg[0] = 0;
	arg[1] = gd->layer_hd;
	arg[2] = 0;
	arg[3] = 0;
	disp_ioctl(NULL,DISP_CMD_LAYER_OPEN,(void*)arg);

    return 0;
}


/*
*******************************************************************************
*                     board_display_layer_close
*
* Description:
*    ¹Ø±ÕÍ¼²ã
*
* Parameters:
*    Layer_hd    :  input. Í¼²ã¾ä±ú
*
* Return value:
*    0  :  ³É¹¦
*   !0  :  Ê§°Ü
*
* note:
*    void
*
*******************************************************************************
*/
int board_display_layer_close(void)
{
    uint arg[4];

	arg[0] = 0;
	arg[1] = gd->layer_hd;
	arg[2] = 0;
	arg[3] = 0;
	disp_ioctl(NULL,DISP_CMD_LAYER_CLOSE,(void*)arg);

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int board_display_layer_para_set(void)
{
    uint arg[4];

	arg[0] = 0;
	arg[1] = gd->layer_hd;
	arg[2] = gd->layer_para;
	arg[3] = 0;
	disp_ioctl(NULL,DISP_CMD_LAYER_SET_PARA,(void*)arg);

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int board_display_show_until_lcd_open(int display_source)
{
    if(!display_source)
    {
    	board_display_wait_lcd_open();
    }
    board_display_layer_para_set();
	board_display_layer_open();

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int board_display_show(int display_source)
{
    board_display_layer_para_set();
	board_display_layer_open();

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int board_display_framebuffer_set(int width, int height, int bitcount, void *buffer)
{
    __disp_layer_info_t *layer_para;
	uint screen_width, screen_height;
	uint arg[4];

	if(!gd->layer_para)
	{
		layer_para = (__disp_layer_info_t *)malloc(sizeof(__disp_layer_info_t));
		if(!layer_para)
		{
			tick_printf("sunxi display error: unable to malloc memory for layer\n");

			return -1;
		}
	}
	else
	{
		layer_para = (__disp_layer_info_t *)gd->layer_para;
	}
	arg[0] = 0;
	screen_width = disp_ioctl(NULL, DISP_CMD_SCN_GET_WIDTH, (void*)arg);
	screen_height = disp_ioctl(NULL, DISP_CMD_SCN_GET_HEIGHT, (void*)arg);
	debug("screen_width =%d, screen_height =%d\n", screen_width, screen_height);
	memset((void *)layer_para, 0, sizeof(__disp_layer_info_t));
	layer_para->fb.addr[0]		= (uint)buffer;
	debug("frame buffer address %x\n", (uint)buffer);
	layer_para->fb.size.width	= width;
	layer_para->fb.size.height	= height;
	layer_para->fb.mode			= DISP_MOD_INTERLEAVED;
	layer_para->fb.format		= (bitcount == 24)? DISP_FORMAT_RGB888:DISP_FORMAT_ARGB8888;
	debug("bitcount = %d\n", bitcount);
	layer_para->fb.br_swap		= 0;
	layer_para->fb.seq			= DISP_SEQ_ARGB;
	layer_para->fb.b_trd_src 	= 0;
	layer_para->fb.trd_mode		= 0;
	layer_para->ck_enable		= 0;
	layer_para->mode            = DISP_LAYER_WORK_MODE_NORMAL;
	layer_para->alpha_en 		= 1;
	layer_para->alpha_val		= 0xff;
	layer_para->pipe 			= 0;
	layer_para->src_win.x		= 0;
	layer_para->src_win.y		= 0;
	layer_para->src_win.width	= width;
	layer_para->src_win.height	= height;
	layer_para->scn_win.x		= (screen_width - width) / 2;
	layer_para->scn_win.y		= (screen_height - height) / 2;
	layer_para->scn_win.width	= width;
	layer_para->scn_win.height	= height;
	layer_para->b_trd_out		= 0;
	layer_para->out_trd_mode 	= 0;

	gd->layer_para = (uint)layer_para;

	return 0;
}

int board_display_framebuffer_change(void *buffer)
{
    uint arg[4];
	__disp_fb_t disp_fb;
	__disp_layer_info_t *layer_para = (__disp_layer_info_t *)gd->layer_para;

	arg[0] = 0;
	arg[1] = gd->layer_hd;
	arg[2] = (uint)&disp_fb;
	arg[3] = 0;

	if(disp_ioctl(NULL, DISP_CMD_LAYER_GET_FB, (void*)arg))
	{
		tick_printf("sunxi display error :get framebuffer failed\n");

		return -1;
	}
	disp_fb.addr[0] = (uint)buffer;
	arg[0] = 0;
    arg[1] = gd->layer_hd;
    arg[2] = (uint)&disp_fb;
    arg[3] = 0;
	//debug("try to set framebuffer %x\n", (uint)buffer);
    if(disp_ioctl(NULL, DISP_CMD_LAYER_SET_FB, (void*)arg))
    {
        tick_printf("sunxi display error :set framebuffer failed\n");

		return -1;
	}
	layer_para->fb.addr[0] = (uint)buffer;

	return 0;
}

int board_display_device_open(void)
{
	int  value = 1;
	int  ret = 0;
	__u32 output_type = 0;
	__u32 output_mode = 0;
	__u32 auto_hpd = 0;
	__u32 err_count = 0;
	unsigned long arg[4] = {0};
	int i;

	debug("De_OpenDevice\n");
//screen0_output_type
	if(script_parser_fetch("boot_disp", "output_type", &value, 1) < 0)
	{
		tick_printf("fetch script data boot_disp.output_type fail\n");
		err_count ++;
		value = 0;
	}
	else
	{
		tick_printf("boot_disp.output_type=%d\n", value);
	}

	if(value == 0)
	{
		output_type = DISP_OUTPUT_TYPE_NONE;
	}
	else if(value == 1)
	{
		output_type = DISP_OUTPUT_TYPE_LCD;
	}
	else if(value == 2)
	{
		output_type = DISP_OUTPUT_TYPE_TV;
	}
	else if(value == 3)
	{
		output_type = DISP_OUTPUT_TYPE_HDMI;
	}
	else if(value == 4)
	{
		output_type = DISP_OUTPUT_TYPE_VGA;
	}
	else
	{
		tick_printf("invalid screen0_output_type %d\n", value);
		return -1;
	}
//screen0_output_mode
	if(script_parser_fetch("boot_disp", "output_mode", &value, 1) < 0)
	{
		tick_printf("fetch script data boot_disp.output_mode fail\n");
		err_count ++;
		value = 0;
	}
	else
	{
		tick_printf("boot_disp.output_mode=%d\n", value);
	}

	if(output_type == DISP_OUTPUT_TYPE_TV || output_type == DISP_OUTPUT_TYPE_HDMI)
	{
		output_mode = (__disp_tv_mode_t)value;
	}
	else if(output_type == DISP_OUTPUT_TYPE_VGA)
	{
		output_mode = (__disp_vga_mode_t)value;
	}

//auto hot plug detect
	if(script_parser_fetch("boot_disp", "auto_hpd", &value, 1) < 0)
	{
		tick_printf("fetch script data boot_disp.auto_hpd fail\n");
		err_count ++;
		value = 0;
	}else
	{
		auto_hpd = value;
		tick_printf("boot_disp.auto_hpd=%d\n", value);
	}


	if(err_count == 3)//no boot_disp config
	{
		if(script_parser_fetch("lcd0_para", "lcd_used", &value, 1) < 0)
		{
			tick_printf("fetch script data lcd0_para.lcd_used fail\n");
			value = 0;
		}else
		{
			tick_printf("lcd0_para.lcd_used=%d\n", value);
		}

		if(value == 1) //lcd available
		{
			output_type = DISP_OUTPUT_TYPE_LCD;
		}
		else
		{
			arg[0] = 0;
			arg[1] = 0;
			arg[2] = 0;
			ret = 0;
			for(i=0; (i<3)&&(ret==0); i++)
			{
				ret = disp_ioctl(NULL, DISP_CMD_HDMI_GET_HPD_STATUS, (void*)arg);
			}
			if(ret == 1)
			{
				output_type = DISP_OUTPUT_TYPE_HDMI;
				//output_mode = (output_mode == -1)? DISP_TV_MOD_720P_50HZ:output_mode;
				output_mode = DISP_TV_MOD_720P_50HZ;
			}
		}

	}
	else//has boot_disp config
	{
		if(output_type == DISP_OUTPUT_TYPE_LCD)
		{

		}
		else if(auto_hpd == 1)
		{
			arg[0] = 0;
			arg[1] = 0;
			for(i=0; (i<3)&&(ret==0); i++)
			{
				ret=disp_ioctl(NULL, DISP_CMD_HDMI_GET_HPD_STATUS, (void*)arg);
                        debug("hdmi pending%d\n",i);
                       // disp_delay_ms(200);


			}
			if (ret == 1)
			{
				output_type = DISP_OUTPUT_TYPE_HDMI;
				output_mode = (output_mode == -1)? DISP_TV_MOD_720P_50HZ:output_mode;
				//output_mode = DISP_TV_MOD_1080P_60HZ;
				debug("------DISP_OUTPUT_TYPE_HDMI-----\n");

			}
			else
			{
                    ret = 0;
                    arg[0] = 0;
                    arg[1] = 0;
			for(i=0; (i<6)&&(ret==0); i++)
                    {
                        debug("%d\n",i);
                        ret = disp_ioctl(NULL, DISP_CMD_TV_GET_INTERFACE, (void*)arg);
                        disp_delay_ms(200);
                    }
                       debug("tv detect, ret = %d\n", ret);
                    if((ret & DISP_TV_CVBS) == DISP_TV_CVBS)
                    {
                        output_type = DISP_OUTPUT_TYPE_TV;
                        output_mode = DISP_TV_MOD_PAL;
			debug("------DISP_TV_CVBS-----\n");
                    }else if((ret & DISP_TV_YPBPR) == DISP_TV_YPBPR)
                    {
                        
                            output_type = DISP_OUTPUT_TYPE_VGA;
                            output_mode = DISP_VGA_H1024_V768;
			debug("------DISP_TV_YPBPR-----\n");

                        
                    }else
                    {
                        output_type = DISP_OUTPUT_TYPE_NONE;
			debug("------DISP_OUTPUT_TYPE_NONE-----\n");

                    }
                }
		}
	}

	if(output_type == DISP_OUTPUT_TYPE_LCD)
	{
		debug("lcd open\n");
		arg[0] = 0;
		arg[1] = 0;
		arg[2] = 0;
		ret = disp_ioctl(NULL, DISP_CMD_LCD_ON, (void*)arg);
		debug("lcd open,ret=%d\n",ret);
	}
	else if(output_type == DISP_OUTPUT_TYPE_HDMI)
	{
		debug("hdmi open\n");
		arg[0] = 0;
		arg[1] = output_mode;
		arg[2] = 0;
		disp_ioctl(NULL, DISP_CMD_HDMI_SET_MODE, (void *)arg);
		ret = disp_ioctl(NULL, DISP_CMD_HDMI_ON, (void *)arg);
	}
	else if(output_type == DISP_OUTPUT_TYPE_TV)
	{
		debug("tv open\n");
		//udelay(1000*1000*10);
		arg[0] = 0;
		arg[1] = output_mode;
		arg[2] = 0;
		disp_ioctl(NULL, DISP_CMD_TV_SET_MODE, (void *)arg);
		ret = disp_ioctl(NULL, DISP_CMD_TV_ON, (void *)arg);
	}
	else if(output_type == DISP_OUTPUT_TYPE_VGA)
	{
		debug("vga open\n");
		//udelay(1000*200);
		arg[0] = 0;
		arg[1] = output_mode;
		arg[2] = 0;
		disp_ioctl(NULL, DISP_CMD_VGA_SET_MODE, (void *)arg);
		ret = disp_ioctl(NULL, DISP_CMD_VGA_ON, (void *)arg);
	}
	return ret;
}

