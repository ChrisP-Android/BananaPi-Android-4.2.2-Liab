/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fb.h>
#include <linux/kd.h>
#include <drv_display_sun7i.h>
#include "log.h"

int  mFD_disp = 0;

struct init_disppara_t
{
    __fb_mode_t                 fb_mode;
    __disp_layer_work_mode_t    layer_mode;
    int                         width;
    int                         height;
    int                         output_width;
    int                         output_height;
    int                         valid_width;
    int                         valid_height;
    int                         bufno;
    int                         format;
};

static int  init_dispgettvwidth(int format)
{
    switch (format)
    {
        case DISP_TV_MOD_480I:                      return 720;
        case DISP_TV_MOD_480P:                      return 720;
        case DISP_TV_MOD_576I:                      return 720;
        case DISP_TV_MOD_576P:                      return 720;
        case DISP_TV_MOD_720P_50HZ:                 return 1280;
        case DISP_TV_MOD_720P_60HZ:                 return 1280;
        case DISP_TV_MOD_1080I_50HZ:                return 1920;
        case DISP_TV_MOD_1080I_60HZ:                return 1920;
        case DISP_TV_MOD_1080P_50HZ:                return 1920;
        case DISP_TV_MOD_1080P_60HZ:                return 1920;
        case DISP_TV_MOD_1080P_24HZ:                return 1920;
        case DISP_TV_MOD_NTSC:                      return 720;
        case DISP_TV_MOD_PAL:                       return 720;
        case DISP_TV_MOD_PAL_M:                     return 720;
        case DISP_TV_MOD_PAL_NC:                    return 720;
        default:                                    break;
    }

    return -1;
}

static int  init_dispgetvgawidth(int format)
{
    switch (format)
    {
        case DISP_VGA_H1680_V1050:                   return 1680;
        case DISP_VGA_H1440_V900:                    return 1440;
        case DISP_VGA_H1360_V768:                    return 1360;
        case DISP_VGA_H1280_V1024:                   return 1280;
        case DISP_VGA_H1024_V768:                    return 1024;
        case DISP_VGA_H800_V600:                     return 800;
        case DISP_VGA_H640_V480:                     return 640;
        case DISP_VGA_H1440_V900_RB:                 return 1440;
        case DISP_VGA_H1680_V1050_RB:                return 1680;
        case DISP_VGA_H1920_V1080_RB:                return 1920;
        case DISP_VGA_H1920_V1080:                   return 1920;
        case DISP_VGA_H1280_V720:                    return 1280;
        default:                                     break;

    }

    return -1;
}

static int  init_dispgettvvalidwidth(int format)
{
    switch (format)
    {
        case DISP_TV_MOD_480I:                      return 660;
        case DISP_TV_MOD_480P:                      return 660;
        case DISP_TV_MOD_576I:                      return 660;
        case DISP_TV_MOD_576P:                      return 660;
        case DISP_TV_MOD_720P_50HZ:                 return 1220;
        case DISP_TV_MOD_720P_60HZ:                 return 1220;
        case DISP_TV_MOD_1080I_50HZ:                return 1840;
        case DISP_TV_MOD_1080I_60HZ:                return 1840;
        case DISP_TV_MOD_1080P_50HZ:                return 1840;
        case DISP_TV_MOD_1080P_60HZ:                return 1840;
        case DISP_TV_MOD_1080P_24HZ:                return 1840;
        case DISP_TV_MOD_NTSC:                      return 660;
        case DISP_TV_MOD_PAL:                       return 660;
        case DISP_TV_MOD_PAL_M:                     return 660;
        case DISP_TV_MOD_PAL_NC:                    return 660;
        default:                                    break;

    }

    return -1;
}

static int  init_dispgetvgavalidwidth(int format)
{
    switch (format)
    {
        case DISP_VGA_H1680_V1050:                   return 1680;
        case DISP_VGA_H1440_V900:                    return 1440;
        case DISP_VGA_H1360_V768:                    return 1360;
        case DISP_VGA_H1280_V1024:                   return 1280;
        case DISP_VGA_H1024_V768:                    return 1024;
        case DISP_VGA_H800_V600:                     return 800;
        case DISP_VGA_H640_V480:                     return 640;
        case DISP_VGA_H1440_V900_RB:                 return 1440;
        case DISP_VGA_H1680_V1050_RB:                return 1680;
        case DISP_VGA_H1920_V1080_RB:                return 1920;
        case DISP_VGA_H1920_V1080:                   return 1920;
        case DISP_VGA_H1280_V720:                    return 1280;
        default:                                     break;
    }

    return -1;
}

static int  init_dispgettvheight(int format)
{
    switch (format)
    {
        case DISP_TV_MOD_480I:                      return 480;
        case DISP_TV_MOD_480P:                      return 480;
        case DISP_TV_MOD_576I:                      return 576;
        case DISP_TV_MOD_576P:                      return 576;
        case DISP_TV_MOD_720P_50HZ:                 return 720;
        case DISP_TV_MOD_720P_60HZ:                 return 720;
        case DISP_TV_MOD_1080I_50HZ:                return 1080;
        case DISP_TV_MOD_1080I_60HZ:                return 1080;
        case DISP_TV_MOD_1080P_50HZ:                return 1080;
        case DISP_TV_MOD_1080P_60HZ:                return 1080;
        case DISP_TV_MOD_1080P_24HZ:                return 1080;
        case DISP_TV_MOD_NTSC:                      return 480;
        case DISP_TV_MOD_PAL:                       return 576;
        case DISP_TV_MOD_PAL_M:                     return 576;
        case DISP_TV_MOD_PAL_NC:                    return 576;
        default:                                    break;
    }

    return  -1;
}

static int  init_dispgetvgaheight(int format)
{
    switch (format)
    {
        case DISP_VGA_H1680_V1050:                   return 1050;
        case DISP_VGA_H1440_V900:                    return 900;
        case DISP_VGA_H1360_V768:                    return 768;
        case DISP_VGA_H1280_V1024:                   return 1024;
        case DISP_VGA_H1024_V768:                    return 768;
        case DISP_VGA_H800_V600:                     return 600;
        case DISP_VGA_H640_V480:                     return 480;
        case DISP_VGA_H1440_V900_RB:                 return 900;
        case DISP_VGA_H1680_V1050_RB:                return 1050;
        case DISP_VGA_H1920_V1080_RB:                return 1080;
        case DISP_VGA_H1920_V1080:                   return 1080;
        case DISP_VGA_H1280_V720:                    return 720;
        default:                                     break;
    }

    return -1;
}



static int  init_dispgettvvalidheight(int format)
{
    switch (format)
    {
        case DISP_TV_MOD_480I:                      return 440;
        case DISP_TV_MOD_480P:                      return 440;
        case DISP_TV_MOD_576I:                      return 536;
        case DISP_TV_MOD_576P:                      return 536;
        case DISP_TV_MOD_720P_50HZ:                 return 680;
        case DISP_TV_MOD_720P_60HZ:                 return 680;
        case DISP_TV_MOD_1080I_50HZ:                return 1040;
        case DISP_TV_MOD_1080I_60HZ:                return 1040;
        case DISP_TV_MOD_1080P_50HZ:                return 1040;
        case DISP_TV_MOD_1080P_60HZ:                return 1040;
        case DISP_TV_MOD_1080P_24HZ:                return 1040;
        case DISP_TV_MOD_NTSC:                      return 440;
        case DISP_TV_MOD_PAL:                       return 536;
        case DISP_TV_MOD_PAL_M:                     return 536;
        case DISP_TV_MOD_PAL_NC:                    return 536;
        default:                                    break;

    }

    return -1;
}

static int  init_dispgetvgavalidheight(int format)
{
    switch (format)
    {
        case DISP_VGA_H1680_V1050:                   return 1050;
        case DISP_VGA_H1440_V900:                    return 900;
        case DISP_VGA_H1360_V768:                    return 768;
        case DISP_VGA_H1280_V1024:                   return 1024;
        case DISP_VGA_H1024_V768:                    return 768;
        case DISP_VGA_H800_V600:                     return 600;
        case DISP_VGA_H640_V480:                     return 480;
        case DISP_VGA_H1440_V900_RB:                 return 900;
        case DISP_VGA_H1680_V1050_RB:                return 1050;
        case DISP_VGA_H1920_V1080_RB:                return 1080;
        case DISP_VGA_H1920_V1080:                   return 1080;
        case DISP_VGA_H1280_V720:                    return 720;
        default:                                        break;
    }

    return -1;
}

static int  init_dispon(int displayno,int outputtype)
{
    unsigned long   args[4];
    int             ret = -1;

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    args[0]     = displayno;
    args[1]     = 0;
    args[2]     = 0;
    args[3]     = 0;
    if(outputtype == DISP_OUTPUT_TYPE_LCD)
    {
        ret = ioctl(mFD_disp,DISP_CMD_LCD_ON,(unsigned long)args);
    }
    else if(outputtype == DISP_OUTPUT_TYPE_TV)
    {
        ret = ioctl(mFD_disp,DISP_CMD_TV_ON,(unsigned long)args);
    }
    else if(outputtype == DISP_OUTPUT_TYPE_HDMI)
    {
        ret = ioctl(mFD_disp,DISP_CMD_HDMI_ON,(unsigned long)args);
    }
    else if(outputtype == DISP_OUTPUT_TYPE_VGA)
    {
        ret = ioctl(mFD_disp,DISP_CMD_VGA_ON,(unsigned long)args);
    }

    return   ret;
}

static int  init_dispoff(int displayno,int outputtype)
{
    unsigned long   args[4];
    int             ret = -1;

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    args[0]     = displayno;
    args[1]     = 0;
    args[2]     = 0;
    args[3]     = 0;
    if(outputtype == DISP_OUTPUT_TYPE_LCD)
    {
        ret = ioctl(mFD_disp,DISP_CMD_LCD_OFF,(unsigned long)args);
    }
    else if(outputtype == DISP_OUTPUT_TYPE_TV)
    {
        ret = ioctl(mFD_disp,DISP_CMD_TV_OFF,(unsigned long)args);
    }
    else if(outputtype == DISP_OUTPUT_TYPE_HDMI)
    {
        ret = ioctl(mFD_disp,DISP_CMD_HDMI_OFF,(unsigned long)args);
    }
    else if(outputtype == DISP_OUTPUT_TYPE_VGA)
    {
        ret = ioctl(mFD_disp,DISP_CMD_VGA_OFF,(unsigned long)args);
    }

    return   ret;
}

static int  init_dispoutput(int displayno, int out_type, int mode)
{
    unsigned long   arg[4];
    int             ret = 0;

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    arg[0] = displayno;

    if(out_type == DISP_OUTPUT_TYPE_LCD)
    {
        ret = ioctl(mFD_disp,DISP_CMD_LCD_ON,(unsigned long)arg);
    }
    else if(out_type == DISP_OUTPUT_TYPE_TV)
    {
        arg[1] = (__disp_tv_mode_t)mode;
        ret = ioctl(mFD_disp,DISP_CMD_TV_SET_MODE,(unsigned long)arg);

        ret = ioctl(mFD_disp,DISP_CMD_TV_ON,(unsigned long)arg);
    }
    else if(out_type == DISP_OUTPUT_TYPE_HDMI)
    {
        arg[1] = (__disp_tv_mode_t)mode;
        ret = ioctl(mFD_disp,DISP_CMD_HDMI_SET_MODE,(unsigned long)arg);

        ret = ioctl(mFD_disp,DISP_CMD_HDMI_ON,(unsigned long)arg);
    }
    else if(out_type == DISP_OUTPUT_TYPE_VGA)
    {
        arg[1] = (__disp_vga_mode_t)mode;
        ret = ioctl(mFD_disp,DISP_CMD_VGA_SET_MODE,(unsigned long)arg);

        ret = ioctl(mFD_disp,DISP_CMD_VGA_ON,(unsigned long)arg);
    }

    return   ret;
}

static int init_dispgethdmistatus()
{
    unsigned long args[4];

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    args[0] = 0;

    return ioctl(mFD_disp,DISP_CMD_HDMI_GET_HPD_STATUS,args);
}

static int init_dispgettvdacstatus()
{
    int                       status;
    unsigned long               args[4];

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    args[0] = 0;
    status = ioctl(mFD_disp,DISP_CMD_TV_GET_INTERFACE,args);

    return status;
}

static int init_dispgethdmimaxmode()
{
    unsigned long args[4];

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    args[0] = 0;
    args[1] = DISP_TV_MOD_1080P_60HZ;

    if(ioctl(mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
    {
        return DISP_TV_MOD_1080P_60HZ;
    }
    else
    {
        args[1] = DISP_TV_MOD_1080P_50HZ;

        if(ioctl(mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
        {
            return DISP_TV_MOD_1080P_50HZ;
        }
        else
        {
            args[1] = DISP_TV_MOD_720P_60HZ;

            if(ioctl(mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
            {
                return DISP_TV_MOD_720P_60HZ;
            }
            else
            {
                args[1] = DISP_TV_MOD_720P_50HZ;

                if(ioctl(mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
                {
                    return DISP_TV_MOD_720P_50HZ;
                }
                else
                {
                    args[1] = DISP_TV_MOD_1080I_60HZ;

                    if(ioctl(mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
                    {
                        return DISP_TV_MOD_1080I_60HZ;
                    }
                    else
                    {
                        args[1] = DISP_TV_MOD_1080I_50HZ;

                        if(ioctl(mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
                        {
                            return DISP_TV_MOD_1080I_50HZ;
                        }
                        else
                        {
                            return DISP_TV_MOD_720P_50HZ;
                        }
                    }
                }
            }
        }
    }

    return DISP_TV_MOD_720P_50HZ;
}

static int init_istvmodesupport(int mode)
{
    unsigned long args[4];

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return 0;
        }
    }

    args[0] = 0;
    args[1] = mode;

    if(ioctl(mFD_disp,DISP_CMD_HDMI_SUPPORT_MODE,args))
    {
        return 1;
    }

    return 0;
}

static int  init_disprequestlayerscreen(int fb_id,struct init_disppara_t *displaypara)
{
    unsigned long               arg[4];
    char                        node[20];
    int                         ret = -1;
    int                         bpp;
    int                         screen;
    unsigned long               fb_layer_hdl;
    __disp_colorkey_t           ck;
    __disp_rect_t               scn_rect;
    int                         fbfd = 0;

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return 0;
        }
    }

    sprintf(node, "/dev/graphics/fb%d", fb_id);

    fbfd            = open(node,O_RDWR,0);
    if(fbfd <= 0)
    {
        ERROR("open fb%d fail!\n",fb_id);

        return  -1;
    }


    if(displaypara->fb_mode == FB_MODE_SCREEN1)
    {
        screen              = 1;
        ioctl(fbfd,FBIOGET_LAYER_HDL_1,&fb_layer_hdl);
    }
    else
    {
        screen              = 0;
        ioctl(fbfd,FBIOGET_LAYER_HDL_0,&fb_layer_hdl);
    }

    if((displaypara->output_height != displaypara->valid_height) || (displaypara->output_width != displaypara->valid_width))
    {
        scn_rect.x          = (displaypara->output_width - displaypara->valid_width)>>1;
        scn_rect.y          = (displaypara->output_height - displaypara->valid_height)>>1;
        scn_rect.width      = displaypara->valid_width;
        scn_rect.height     = displaypara->valid_height;

        //ERROR("scn_rect.width = %d,scn_rect.height = %d,screen = %d,fb_layer_hdl = %d,fb_id = %d\n",scn_rect.width,scn_rect.height,screen,fb_layer_hdl,fb_id);
    }
    else
    {
        scn_rect.x          = 0;
        scn_rect.y          = 0;
        scn_rect.width      = displaypara->valid_width;
        scn_rect.height     = displaypara->valid_height;
    }

    arg[0]              = screen;
    arg[1]              = fb_layer_hdl;
    arg[2]              = (unsigned long)(&scn_rect);

    ioctl(mFD_disp,DISP_CMD_LAYER_SET_SCN_WINDOW,(void*)arg);//pipe1, different with video layer's pipe

    close(fbfd);

    return 0;
}

static int init_dispgetoutputtype(int displayno)
{
    int                     ret;
    unsigned long           args[4];

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    args[0] = displayno;
    ret = ioctl(mFD_disp,DISP_CMD_GET_OUTPUT_TYPE,args);

    return  ret;
}

static int init_dispgettvformat(int displayno)
{
    int                     ret;
    unsigned long           args[4];

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    args[0] = displayno;
    ret = ioctl(mFD_disp,DISP_CMD_TV_GET_MODE,args);

    return  ret;
}

static int init_dispgethdmiformat(int displayno)
{
    int                     ret;
    unsigned long           args[4];

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    args[0] = displayno;
    ret = ioctl(mFD_disp,DISP_CMD_HDMI_GET_MODE,args);

    return  ret;
}

static int init_dispgetvgamode(int displayno)
{
    int                     ret;
    unsigned long           args[4];

    if(mFD_disp == 0)
    {
        mFD_disp = open("/dev/disp", O_RDWR, 0);
        if (mFD_disp < 0)
        {
            ERROR("open display device failed!\n");

            mFD_disp = 0;

            return -1;
        }
    }

    args[0] = displayno;
    ret = ioctl(mFD_disp,DISP_CMD_VGA_GET_MODE,args);

    return  ret;
}

static int init_swtichdisplay(int displayno,int type,int format)
{
    int                         status = 0;
    struct init_disppara_t      para;
    int                         tvformat = 0;
    int                         curtype = 0;

    curtype = init_dispgetoutputtype(displayno);
    if(curtype == DISP_OUTPUT_TYPE_TV)
    {
        tvformat = init_dispgettvformat(displayno);
    }
    else if(curtype == DISP_OUTPUT_TYPE_VGA)
    {
        tvformat = init_dispgetvgamode(displayno);
    }
    else if(curtype == DISP_OUTPUT_TYPE_HDMI)
    {
        tvformat = init_dispgethdmiformat(displayno);
    }

    //ERROR("[william]curtype = %d,tvformat = %d\n",curtype,tvformat);

    //ERROR("[william]type = %d,format = %d\n",type,format);

    if((type != curtype) || ((type == curtype) && format != tvformat))
    {
        init_dispoff(displayno,curtype);

        para.fb_mode        = FB_MODE_SCREEN0;
        if(type == DISP_OUTPUT_TYPE_TV || type == DISP_OUTPUT_TYPE_HDMI)
        {
            para.height             = init_dispgettvheight(format);
            para.width              = init_dispgettvwidth(format);
            para.output_height      = init_dispgettvheight(format);
            para.output_width       = init_dispgettvwidth(format);
            para.valid_height       = init_dispgettvvalidheight(format);
            para.valid_width        = init_dispgettvvalidwidth(format);
        }
        else if(type == DISP_OUTPUT_TYPE_VGA)
        {
            para.height             = init_dispgetvgaheight(format);
            para.width              = init_dispgetvgawidth(format);
            para.output_height      = init_dispgetvgaheight(format);
            para.output_width       = init_dispgetvgawidth(format);
            para.valid_height       = init_dispgetvgavalidheight(format);
            para.valid_width        = init_dispgetvgavalidwidth(format);
        }

        init_disprequestlayerscreen(0,&para);

        init_dispoutput(displayno,type,format);

        return  1;
    }

    return 0;
}

int init_initdisplay()
{
    int child = fork();
    //parent process
    if(child > 0 || child < 0)
        return child > 0 ? 0 : -1;
    //child process
    INFO("To select the valid output device to out put.");
    int hdmistatus = 0;
    int tvstatus = 0;

    int loop = 0;
    for(loop = 0;loop < 40 && !hdmistatus;loop++){
        hdmistatus = init_dispgethdmistatus();
        usleep(50000);
    }
    if(hdmistatus)
    {
        init_swtichdisplay(0,DISP_OUTPUT_TYPE_HDMI,DISP_TV_MOD_720P_60HZ);
    }
    else
    {
        tvstatus = init_dispgettvdacstatus();
        ERROR("[william] tvstatus = %d!\n",tvstatus);
        if(tvstatus == DISP_TV_CVBS)
        {
            init_swtichdisplay(0,DISP_OUTPUT_TYPE_TV,DISP_TV_MOD_NTSC);
        }
        else if(tvstatus == DISP_TV_YPBPR)
        {
           // init_swtichdisplay(0, DISP_OUTPUT_TYPE_VGA, DISP_VGA_H1024_V768);
	   int mode = init_dispgethdmimaxmode();
	   init_swtichdisplay(0,DISP_OUTPUT_TYPE_HDMI,mode);
        }
    }

    ERROR("HDMI status = %d, try %d times!TV status = %d\n",hdmistatus, loop, tvstatus);
    if(mFD_disp)
    {
        close(mFD_disp);

        mFD_disp = 0;
    }
    exit(0);
}

