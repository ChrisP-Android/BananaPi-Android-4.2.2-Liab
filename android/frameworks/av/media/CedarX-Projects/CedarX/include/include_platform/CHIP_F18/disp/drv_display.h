/*
**********************************************************************************************************************
*                                                    ePDK
*                                    the Easy Portable/Player Develop Kits
*                                              eMOD Sub-System
*
*                                   (c) Copyright 2007-2009, Steven.ZGJ.China
*                                             All Rights Reserved
*
* Moudle  : display driver
* File    : drv_display.c
*
* By      : William
* Version : v1.0
* Date    : 2008-1-8 9:46:23
**********************************************************************************************************************
*/

#ifndef _DRV_DISPLAY_H
#define _DRV_DISPLAY_H

#include "de_type.h"


typedef struct tag_COLORKEY
{
   __color_t   ck_max;
   __color_t   ck_min;
   __u8        ck_match;//
}__colorkey_t;

typedef enum
{
    RGB_PS,
    BGR_PS
}__br_swap_mod_t;

/*
************************************************************
* global definition
************************************************************
*/
/* work mode select                                 */
typedef enum tag_DISP_WORK_MODE
{
    DISP_WORK_MODE_DE_TV=0,                         /* display engine+tv                                            */
    DISP_WORK_MODE_DE_LCD_TV,                       /* display engine+lcd+tv                                        */
    DISP_WORK_MODE_DE_LCD,                    	    /* display engine+lcd                                           */
    DISP_WORK_MODE_LCD_ONLY                         /* LCD                                                          */
}__disp_work_mode_t;

/* output mode select                               */
typedef enum tag_DISP_OUTPUT_MOD
{
    DISP_OUTPUT_MODE_LCD_TV_CH1,                    /* LCD TV use display engine channel1                           */
    DISP_OUTPUT_MODE_LCD_TV_CH2,                    /* LCD TV use display engine channel2                           */
    DISP_OUTPUT_MODE_LCD_CH1_TV_CH2,                /* LCD use channel1,TV use channel2                             */
    DISP_OUTPUT_MODE_LCD_CH2_TV_CH1,                /* LCD use channel2,TV use channel1                             */
    DISP_OUTPUT_MODE_LCD_DMA
}__disp_output_mode_t;

typedef enum tag_DISP_OUTPUT_TYPE
{
	DISP_OUTPUT_TYPE_LCD,
	DISP_OUTPUT_TYPE_TV,
	DISP_OUTPUT_TYPE_HDMI,
	DISP_OUTPUT_TYPE_VGA,
	DISP_OUTPUT_TYPE_NONE
}__disp_output_type_t;

typedef enum tag_DISP_TV_MODE
{
    DISP_TV_MOD_480I,
    DISP_TV_MOD_576I,
    DISP_TV_MOD_480P,
    DISP_TV_MOD_576P,
    DISP_TV_MOD_720P_50HZ,
    DISP_TV_MOD_720P_60HZ,
    DISP_TV_MOD_1080I_50HZ,
    DISP_TV_MOD_1080I_60HZ,
    DISP_TV_MOD_1080P_24HZ,
    DISP_TV_MOD_PAL,
    DISP_TV_MOD_NTSC,
    DISP_TV_MOD_1080P_50HZ,
    DISP_TV_MOD_1080P_60HZ,
    DISP_TV_MOD_PAL_SVIDEO,
    DISP_TV_MOD_NTSC_SVIDEO,
    DISP_TV_MOD_PAL_CVBS_SVIDEO,
    DISP_TV_MOD_NTSC_CVBS_SVIDEO,
    DISP_TV_MOD_PAL_M,
    DISP_TV_MOD_PAL_M_SVIDEO,
    DISP_TV_MOD_PAL_M_CVBS_SVIDEO,
    DISP_TV_MOD_PAL_NC,
    DISP_TV_MOD_PAL_NC_SVIDEO,
    DISP_TV_MOD_PAL_NC_CVBS_SVIDEO,
    DISP_TV_MOD_DEFAULT = 0xff
}__disp_tv_mode_t;

typedef enum tag_DISP_TV_OUTPUT
{
    DISP_TV_CVBS,
    DISP_TV_YPBPR,
    DISP_TV_SVIDEO,
    DISP_TV_NONE,
    DISP_TV_OTHER
}__disp_tv_output_t;

typedef enum tag_DISP_VGA_MODE
{
		DISP_VGA_H1680_V1050 = 0,
		DISP_VGA_H1440_V900,
		DISP_VGA_H1360_V768,
		DISP_VGA_H1280_V1024,
		DISP_VGA_H1024_V768,
		DISP_VGA_H800_V600,
		DISP_VGA_H640_V480,
		DISP_VGA_H1440_V900_RB,
		DISP_VGA_H1680_V1050_RB,
		DISP_VGA_H1920_V1080_RB,
		DISP_VGA_H1920_V1080,
		DISP_VGA_DEFAULT = 0xff
}__disp_vga_mode_t;


typedef enum tag_DISP_LCDC_SRC
{
     DISP_LCDC_INSRC_DE_CH1 = 0,
     DISP_LCDC_INSRC_DE_CH2,
     DISP_LCDC_INSRC_DMA,
     DISP_LCDC_INWHITE_DATAS
}__disp_lcdc_src_t;

typedef enum tag_DISP_LCDC_DMA_FMT
{
    DISP_LCDC_RGB24_888,
    DISP_LCDC_RGB32_0888,
    DISP_LCDC_RGB16_565
}__disp_lcdc_dma_fmt_t;

typedef enum tag_DISP_LCD_BRIGHT
{
	DISP_LCD_BRIGHT_LEVEL0 = 0,
	DISP_LCD_BRIGHT_LEVEL1,
	DISP_LCD_BRIGHT_LEVEL2,
	DISP_LCD_BRIGHT_LEVEL3,
	DISP_LCD_BRIGHT_LEVEL4,
	DISP_LCD_BRIGHT_LEVEL5,
	DISP_LCD_BRIGHT_LEVEL6,
	DISP_LCD_BRIGHT_LEVEL7,
	DISP_LCD_BRIGHT_LEVEL8,
	DISP_LCD_BRIGHT_LEVEL9,
	DISP_LCD_BRIGHT_LEVEL10,
	DISP_LCD_BRIGHT_LEVEL11,
	DISP_LCD_BRIGHT_LEVEL12,
	DISP_LCD_BRIGHT_LEVEL13,
	DISP_LCD_BRIGHT_LEVEL14,
	DISP_LCD_BRIGHT_LEVEL15
}__disp_lcd_bright_t;


/*
************************************************************
* layer definition
************************************************************
*/
typedef enum tag_DISP_LAYER_WORK_MODE        		/*layer work mode enum definition                               */
{
    DISP_LAYER_WORK_MODE_NORMAL=0,                  /*normal work mode                                              */
    DISP_LAYER_WORK_MODE_PALETTE,                   /*palette work mode                                             */
    DISP_LAYER_WORK_MODE_INTER_BUF,                 /*internal frame buffer work mode                               */
    DISP_LAYER_WORK_MODE_GAMMA,                     /*gamma correction mode                                         */
    DISP_LAYER_WORK_MODE_SCALER = 4,
}__disp_layer_work_mode_t;

typedef enum tag_DISP_LAYER_OUTPUT_CHN
{
    DISP_LAYER_OUTPUT_CHN_DE_CH1,
    DISP_LAYER_OUTPUT_CHN_DE_CH2
}__disp_layer_output_chn_t;


typedef struct tag_DISP_LAYER_PARA
{
    __disp_layer_work_mode_t       mode;			/*set layer work mode                                           */
    __bool                         ck_mode;         /*layer color mode                                              */
    __bool                         alpha_en;        /*layer alpha enable mode switch                                */
    __u16                          alpha_val;       /*layer alpha value                                             */
    __u8                           pipe;            /*layer pipe                                                    */
    __u8                           prio;            /* layer priority                                               */
    __rect_t					   scn_win;         /* sceen region                                                 */
    __rect_t                       src_win;         /* framebuffer source window                                    */
    __disp_layer_output_chn_t      channel;         /*only use by internal framebuffer mode 0:  DE_CH1, 1:DE_CH2    */
    FB                            *fb;
}__disp_layer_para_t;


typedef struct tag_DISP_LAYER_ATTR
{
    __bool   flags;
    __bool   alpha_en;
    __u8     alpha_val;
    __bool   ck_en;
    __u8     pipe;
    __u8     prio;
    __u32    hid;
}__disp_layer_attr_t;

/*video layer frame mode definition*/
typedef enum tag_DISP_VIDEO_FRM_MOD
{
    DISP_VIDEO_FRM_MOD_ODD_EVEN=0,
    DISP_VIDEO_FRM_MOD_ONLY_ODD=1
}__disp_video_frm_mod_e;

/*video scanning mode definition*/
typedef enum tag_DISP_VIDEO_SCAN_MOD
{
    DISP_NON_INTERLACE=0,
    DISP_INTERLACE
}__disp_video_scan_mod_t;


typedef enum tag_DISP_VIDEO_SMOOTH
{
	DISP_VIDEO_NATUAL 		= 0,
	DISP_VIDEO_SOFT 		= 3,
	DISP_VIDEO_VERYSOFT 	= 6,
	DISP_VIDEO_SHARP    	= 0x42,
	DISP_VIDEO_VERYSHARP    = 0x44
}__disp_video_smooth_t;

#define INVALID_FRAME_RATE 0
typedef struct tag_DISP_LAYER_FB_ADDR
{
	__s32   id;
    __s32   addr[3];
    __bool  interlace;
    __bool  top_field_first;
    __s32   frame_rate; // *FRAME_RATE_BASE(现在定为1000)
    __bool	first_frame;
}__disp_layer_fb_addr_t;

/*
************************************************************
* hardware cursor attribute
*
************************************************************
*/
typedef enum tag_DISP_HWC_MODE
{
    DISP_HWC_MOD_H32_V32_8BPP,
    DISP_HWC_MOD_H64_V64_2BPP,
    DISP_HWC_MOD_H64_V32_4BPP,
    DISP_HWC_MOD_H32_V64_4BPP
}__disp_hwc_mode_t;

typedef struct
{
    __disp_hwc_mode_t     pat_mode;                 /*pattern memory format                                         */
    __u16                 x_offset;                 /*pattern memory x offset                                       */
    __u16                 y_offset;                 /*pattern memory y offset                                       */
    void *                pat_mem;                  /*pattern memory virtual address                                */
}__disp_hwc_pattern_t;

typedef struct tag_DISP_HWC_PARA
{
    __pos_t               sc_pos;                   /*hardware cursor sceen pos                                     */
    __disp_hwc_pattern_t  pattern;                  /*hardware cursor pattern data information                      */
    void  *               palette;                  /*hardware cursor palette virtual address                       */
    __u32                 palette_size;             /*hardware cursor palette size                                  */
}__disp_hwc_para_t;
/*
************************************************************
* scaler attribute definition
*
************************************************************
*/
typedef struct tag_
{
    FB                  * input_fb;
    __rect_t                  source_regn;
    FB                  * output_fb;
}__disp_scaler_para_t;


/*
************************************************************
* general display attribute definition
*
************************************************************
*/
typedef enum tag_DISP_CAPSCN_MODE
{
    DISP_CAPSCN_MODE_REAL,
    DISP_CAPSCN_MODE_VIRTUAL
}__disp_capscn_mode_t;

/*sceen parameter*/
typedef struct  tag_DISP_SCN_PARA
{
   __color_t                bkcolor;
   __colorkey_t             gcolorkey;
   __disp_layer_para_t    * layer[4];
   __disp_hwc_para_t      * hwc;
   __u32                    SWidth;
   __u32                    SHeight;
}__disp_scn_para_t;

typedef struct  tag_DISP_CAPTURE_SCN_PARA
{
     __disp_scn_para_t    * scn_para;
     FB                   * outfb;
}__disp_capture_scn_para_t;


typedef struct tag_DISP_SPRITE_BLOCK_PARA
{
	FB	*fb;
	__rect_t src_win;	/*source region,only care x,y*/
	__rect_t	scn_win;         /* sceen region     */
}__disp_sprite_block_para_t;

typedef struct tag_DISP_FB_CREATE_PATA
{
	__disp_layer_work_mode_t mode;
	__u32 width;
	__u32 height;
	__u32 line_length;
	__u32 smem_len;
	__u32 ch1_offset;
	__u32 ch2_offset;
	__u32 visual;// 0:FB_VISUAL_MONO01, 1:FB_VISUAL_MONO10, 2:FB_VISUAL_TRUECOLOR, 
					//3:FB_VISUAL_PSEUDOCOLOR, 4:FB_VISUAL_DIRECTCOLOR, 5:FB_VISUAL_STATIC_PSEUDOCOLOR
}__disp_fb_create_para_t;

/*
************************************************************
* display driver ioctl cmd definition
*
************************************************************
*/

/*define display driver command*/
typedef enum tag_DISP_CMD
{
    /* command cache on/off                         */
    DISP_CMD_START_CMD_CACHE =0,
    DISP_CMD_EXECUTE_CMD_AND_STOP_CACHE=1,

    /*gobal display attribute operation             */
    DISP_CMD_SET_BKCOLOR = 0x40,                           /*set background color                                          */
    DISP_CMD_GET_BKCOLOR = 0x41,                           /*get background color                                          */
    DISP_CMD_SET_COLORKEY = 0x42,                          /*set color key match condition                                 */
    DISP_CMD_GET_COLORKEY = 0x43,                          /*get color key match condition                                 */
    DISP_CMD_SET_PALETTE_TBL = 0x44,                       /*set sys palette tbl,which is used by lyr in palette mode      */
    DISP_CMD_SET_GAMMA_TBL = 0x45,                         /*set sys gamma tbl,used by lyr in gamma correction mode        */
    DISP_CMD_SET_INTERFB_TBL = 0x46,                       /*set sys internal fb data,used by lyr in internal fb mode      */
    DISP_CMD_SET_INTER_PALETTE_TABLE = 0x47,               /*set internal framebuffer palette table                        */
    DISP_CMD_GET_PALETTE_TBL = 0x48,                       /*get palette table color*/
    DISP_CMD_GET_INTER_PALETTE_TABLE = 0x49,               /*get internal palette table*/
    DISP_CMD_WAIT_VBLANKING = 0x4a,

    /*layer op                                      */
    DISP_CMD_LAYER_REQUEST = 0x80,                         /*request a layer object                                        */
    DISP_CMD_LAYER_RELEASE = 0x81,                         /*release a layer object                                        */
    DISP_CMD_LAYER_OPEN = 0x82,                            /*open a requested layer object                                 */
    DISP_CMD_LAYER_CLOSE = 0x83,                           /*close a requested layer object                                */
    DISP_CMD_LAYER_SET_FB = 0x84,                          /*set a layer object framebuffer info(address,format,size......)*/
    DISP_CMD_LAYER_GET_FB = 0x85,                          /*get lyr object framebuffer information                        */
    DISP_CMD_LAYER_SET_SRC_WINDOW = 0x86,                  /*set lyr current fb source windows(in pixels)                  */
    DISP_CMD_LAYER_GET_SRC_WINDOW = 0x87,                  /*get lyr current fb source windows information(in pixels)      */
    DISP_CMD_LAYER_SET_SCN_WINDOW = 0x88,                  /*set lyr current sceen windows (in pixels)                     */
    DISP_CMD_LAYER_GET_SCN_WINDOW = 0x89,                  /*get lyr current sceen windows information (in pixels)         */
    DISP_CMD_LAYER_UPDATE_INTERFB = 0x8a,                  /*update layer internal framebuffer data                        */
    DISP_CMD_LAYER_SET_PARA = 0x8b,                        /*set layer parameter and sceen size                            */
    DISP_CMD_LAYER_TOP = 0x8c,                             /*set layer top on sceen                                        */
    DISP_CMD_LAYER_BOTTOM = 0x8d,                          /*set layer bottom on sceen                                     */
    DISP_CMD_LAYER_CK_ON = 0x8e,                           /*set background color                                          */
    DISP_CMD_LAYER_CK_OFF = 0x8f,                          /*set background color                                          */
    DISP_CMD_LAYER_ALPHA_ON = 0x90,                        /*set color key match condition                                 */
    DISP_CMD_LAYER_ALPHA_OFF = 0x91,                       /*set color key match condition                                 */
    DISP_CMD_LAYER_SET_ALPHA_VALUE = 0x92,                 /*set sys palette tbl,which is used by lyr in palette mode      */
    DISP_CMD_LAYER_SET_PIPE = 0x93,                        /*set sys gamma tbl,which is used by lyr in gamma correction mod*/
    DISP_CMD_LAYER_GET_PRIO = 0x94,                        /*get layer prio*/
    DISP_CMD_LAYER_GET_ALPHA = 0x95,                        /*get layer alpha value*/
    DISP_CMD_LAYER_GET_ALPHA_EN = 0x96,                    /*get layer alpha enable */
    DISP_CMD_LAYER_GET_CK_EN = 0x97,                       /*get layer colorkey mode */
    DISP_CMD_LAYER_GET_PIPE = 0x98,                        /*get layer */
    DISP_CMD_LAYER_GET_PARA = 0x99,
    
    /*scaler manager operation cmd*/
    DISP_CMD_SCALER_REQUEST = 0xc0,                        /*request scaler object                                         */
    DISP_CMD_SCALER_RELEASE = 0xc1,                        /*release scaler object                                         */
    DISP_CMD_SCALER_EXECUTE = 0xc2,                        /*scaler start                                                  */

	/*hardware cursor cmd*/
    DISP_CMD_HWC_INIT = 0x100,                              /*init hardware cursor object                                   */
    DISP_CMD_HWC_SHOW = 0x101,                              /*show hardware cursor                                          */
    DISP_CMD_HWC_HIDE = 0x102,                              /*hide hardware cursor                                          */
    DISP_CMD_HWC_RELEASE = 0x103,                           /*release hardware cursor object                                */
    DISP_CMD_HWC_SET_POS = 0x104,                           /*set hardware cursor sceen coordinate                          */
    DISP_CMD_HWC_GET_POS = 0x105,                           /*get hareware cursor sceen coordinate                          */
    DISP_CMD_HWC_SET_PATMEM = 0x106,                        /*set hardware cursor pattern memory block information          */
    DISP_CMD_HWC_SET_PALETTE_TABLE = 0x107,             	/*set hardware cursor palette table                             */

    /*current sceen use information*/
    DISP_CMD_SCN_GET_WIDTH = 0x140,                         /*get current sceen width                                       */
    DISP_CMD_SCN_GET_HEIGHT = 0x141,                        /*get current sceen height                                      */

    /*output device mamage cmd*/
    DISP_CMD_LCD_ON = 0x180,                                /*lcd on                                                        */
    DISP_CMD_LCD_OFF = 0x181,                               /*lcd off                                                       */
    DISP_CMD_LCD_SET_BRIGHTNESS = 0x182,                    /*set lcd brightness                                            */
    DISP_CMD_LCD_GET_BRIGHTNESS = 0x183,                    /*get lcd brightness                                            */
    DISP_CMD_LCD_SET_CONTRAST = 0x184,                      /*set lcd contrast                                              */
    DISP_CMD_LCD_GET_CONTRAST = 0x185,                      /*get lcd contrast                                              */
    DISP_CMD_LCD_SET_COLOR = 0x186,                         /*set lcd color                                                 */
    DISP_CMD_LCD_GET_COLOR = 0x187,                         /*get lcd color                                                 */
    DISP_CMD_LCD_SET_SRC = 0x188,                           /*lcd select input source                                       */
    DISP_CMD_TV_SET_SRC = 0x189,                            /*tv select inout source                                        */
    DISP_CMD_TV_ON = 0x18a,                                 /*tv on                                                         */
    DISP_CMD_TV_OFF = 0x18b,                                /*tv off                                                        */
    DISP_CMD_TV_AUTOCHECK_ON = 0x18c,                       /*tv autocheck on                                               */
    DISP_CMD_TV_AUTOCHECK_OFF = 0x18d,                      /*tv autocheck off                                              */
    DISP_CMD_TV_SET_MODE = 0x18e,                           /*set tv display mode                                           */
    DISP_CMD_TV_GET_MODE = 0x18f,                           /*get tv current display mode                                   */
    DISP_CMD_TV_GET_INTERFACE = 0x190,                      /*get tv current interface                                      */
    DISP_CMD_LCD_CPUIF_XY_SWITCH = 0x191,                   /*lcd driver command for cpu panel to swith              	    */
    DISP_CMD_LCD_SET_GAMMA_TABLE = 0x192,
    DISP_CMD_LCD_GAMMA_CORRECTION_ON = 0x193,
    DISP_CMD_LCD_GAMMA_CORRECTION_OFF = 0x194,
    
    
    /*video manager operation*/
    DISP_CMD_VIDEO_INSTALL_CALLBACK = 0x1c0,
    DISP_CMD_VIDEO_UNSTALL_CALLBACK = 0x1c1,
    DISP_CMD_VIDEO_SET_FB_ADDR = 0x1c2,
    DISP_CMD_VIDEO_CAPTURE = 0x1c3,
    DISP_CMD_VIDEO_START = 0x1c4,
    DISP_CMD_VIDEO_STOP = 0x1c5,
    DISP_CMD_VIDEO_GET_FRAME_ID = 0x1c6,

    /*sceen management operation*/
    DISP_CMD_SCN_CAPTURE = 0x200,
	DISP_CMD_GET_OUTPUT_TYPE = 0x201,

	DISP_CMD_HDMI_ON = 0x240,
	DISP_CMD_HDMI_OFF = 0x241,
	DISP_CMD_SET_HDMI_MOD = 0x242,
	DISP_CMD_GET_HDMI_MOD = 0x243,
	DISP_CMD_HDMI_SUPPORT_MOD = 0x244,
	DISP_CMD_HDMI_AUDIO_ENABLE = 0x245,
	DISP_CMD_HDMI_GET_HPD_STATUS = 0x256,

	DISP_CMD_START_VIDEO_MODE = 0x280,
	DISP_CMD_STOP_VIDEO_MODE = 0x281,

	/*v-blanking event callback*/
	DISP_CMD_EXECUTE_CALLBACK = 0x2c0,
	DISP_CMD_SCALER_SET_SMOOTH = 0x2c1,
	DISP_CMD_SCALER_GET_SMOOTH = 0x2c2,


	/*vga management operation*/
	DISP_CMD_VGA_ON = 0x300,
	DISP_CMD_VGA_OFF = 0x301,
	DISP_CMD_VGA_SET_MODE = 0x302,
	DISP_CMD_VGA_GET_MODE = 0x303,
	DISP_CMD_VGA_SET_SRC = 0x304,

	/*sprite operation*/
	DISP_CMD_SPRITE_OPEN = 0x340,
	DISP_CMD_SPRITE_CLOSE = 0x341,
	DISP_CMD_SPRITE_SET_FORMAT = 0x342,
	DISP_CMD_SPRITE_GLOBAL_ALPHA_ENABLE = 0x343,
	DISP_CMD_SPRITE_GLOBAL_ALPHA_DISABLE = 0x344,
	DISP_CMD_SPRITE_SET_GLOBAL_ALPHA_VALUE = 0x345,
	DISP_CMD_SPRITE_SET_ORDER = 0x346,
	DISP_CMD_SPRITE_PALETTE_TBL = 0x347,
	DISP_CMD_SPRITE_BLOCK_REQUEST = 0x348,
	DISP_CMD_SPRITE_BLOCK_RELEASE = 0x349,
	DISP_CMD_SPRITE_BLOCK_SET_SCREEN_WINDOW = 0x34a,
	DISP_CMD_SPRITE_BLOCK_SET_FB = 0x34b,
	DISP_CMD_SPRITE_BLOCK_GET_SCREEN_WINDOW = 0x34c,
	DISP_CMD_SPRITE_BLOCK_GET_FB = 0x34d,
	DISP_CMD_SPRITE_BLOCK_SET_TOP = 0x34e,
	DISP_CMD_SPRITE_BLOCK_SET_BOTTOM = 0x34f,
	DISP_CMD_SPRITE_GET_TOP_BLOCK = 0x350,
	DISP_CMD_SPRITE_GET_BOTTOM_BLOCK = 0x351,
	DISP_CMD_SPRITE_GET_GLOBAL_ALPHA_ENABLE = 0x352,
	DISP_CMD_SPRITE_GET_GLOBAL_ALPHA_VALUE = 0x353,
	DISP_CMD_SPRITE_BLOCK_GET_PREV_BLOCK = 0x354,
	DISP_CMD_SPRITE_BLOCK_GET_NEXT_BLOCK = 0x355,
	DISP_CMD_SPRITE_BLOCK_GET_PRIO = 0x356,
	DISP_CMD_SPRITE_BLOCK_OPEN = 0x357,
	DISP_CMD_SPRITE_BLOCK_CLOSE = 0x358,
	DISP_CMD_SPRITE_GET_BLOCK_NUM = 0x359,
	DISP_CMD_SPRITE_BLOCK_SET_SOURCE_WINDOW = 0x35a,
	DISP_CMD_SPRITE_BLOCK_GET_SOURCE_WINDOW = 0x35b,
	DISP_CMD_SPRITE_BLOCK_SET_PARA = 0x35c,
	DISP_CMD_SPRITE_BLOCK_GET_PARA = 0x35d,

	DISP_CMD_FB_REQUEST = 380,
	DISP_CMD_FB_RELEASE = 381,
	
}__disp_cmd_t;

#define FBIOGET_LAYER_HDL 0x4700

#endif  /* _DRV_DISPLAY_H */
