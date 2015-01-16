/*
********************************************************************************
*                                    Android
*                               CedarX sub-system
*                               CedarXAndroid module
*
*          (c) Copyright 2010-2013, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : virtual_hwcomposer.h
* Version: V1.0
* By     : Eric_wang
* Date   : 2012-2-26
* Description:
********************************************************************************
*/
#ifndef _VIRTUAL_HWCOMPOSER_H_
#define _VIRTUAL_HWCOMPOSER_H_

#include <CDX_PlayerAPI.h>
#include <libcedarv.h>

namespace android {

#if 1

/*****************************************************************************/

/**
 * The id of this module
 */
#define HWC_HARDWARE_MODULE_ID "hwcomposer"

/**
 * Name of the sensors device to open
 */
#define HWC_HARDWARE_COMPOSER   "composer"


/* possible overlay formats */
typedef enum e_hwc_format
{
    HWC_FORMAT_MINVALUE     = 0x50,
    HWC_FORMAT_RGBA_8888    = 0x51,
    HWC_FORMAT_RGB_565      = 0x52,
    HWC_FORMAT_BGRA_8888    = 0x53,
    HWC_FORMAT_YCbYCr_422_I = 0x54,
    HWC_FORMAT_CbYCrY_422_I = 0x55,
    HWC_FORMAT_MBYUV420		= 0x56,
    HWC_FORMAT_MBYUV422		= 0x57,
    HWC_FORMAT_YUV420PLANAR	= 0x58,
    HWC_FORMAT_DEFAULT      = 0x99,    // The actual color format is determined
    HWC_FORMAT_MAXVALUE     = 0x100
}e_hwc_format_t;

typedef enum e_hwc_3d_src_mode
{
    HWC_3D_SRC_MODE_TB = 0x0,//top bottom
    HWC_3D_SRC_MODE_FP = 0x1,//frame packing
    HWC_3D_SRC_MODE_SSF = 0x2,//side by side full
    HWC_3D_SRC_MODE_SSH = 0x3,//side by side half
    HWC_3D_SRC_MODE_LI = 0x4,//line interleaved

    HWC_3D_SRC_MODE_NORMAL = 0xFF//2d
}e_hwc_3d_src_mode_t;

/* names for setParameter() */
typedef enum e_hwc_3d_out_mode{
    HWC_3D_OUT_MODE_2D 		            = 0x0,//left picture
    HWC_3D_OUT_MODE_HDMI_3D_1080P24_FP 	= 0x1,
    HWC_3D_OUT_MODE_ANAGLAGH 	        = 0x2,//·ÖÉ«
    HWC_3D_OUT_MODE_ORIGINAL 	        = 0x3,//original pixture

    HWC_3D_OUT_MODE_LI                  = 0x4,//line interleaved
    HWC_3D_OUT_MODE_CI_1                = 0x5,//column interlaved 1
    HWC_3D_OUT_MODE_CI_2                = 0x6,//column interlaved 2
    HWC_3D_OUT_MODE_CI_3                = 0x7,//column interlaved 3
    HWC_3D_OUT_MODE_CI_4                = 0x8,//column interlaved 4

    HWC_3D_OUT_MODE_HDMI_3D_720P50_FP   = 0x9,
    HWC_3D_OUT_MODE_HDMI_3D_720P60_FP   = 0xa
}e_hwc_3d_out_mode_t;

/* names for setParameter() */
typedef enum e_hwc_layer_cmd{
    /* rotation of the source image in degrees (0 to 359) */
    HWC_LAYER_ROTATION_DEG  	= 1,
    /* enable or disable dithering */
    HWC_LAYER_DITHER        	= 3,
    /* transformation applied (this is a superset of COPYBIT_ROTATION_DEG) */
    HWC_LAYER_SETINITPARA = 4,
    /* set videoplayer init overlay parameter */
    HWC_LAYER_SETVIDEOPARA = 5,
    /* set videoplayer play frame overlay parameter*/
    HWC_LAYER_SETFRAMEPARA = 6,
    /* get videoplayer play frame overlay parameter*/
    HWC_LAYER_GETCURFRAMEPARA = 7,
    /* query video blank interrupt*/
    HWC_LAYER_QUERYVBI = 8,
    /* set overlay screen id*/
    HWC_LAYER_SETMODE = 9,

    HWC_LAYER_SHOW = 0xa,
    HWC_LAYER_RELEASE = 0xb,
    HWC_LAYER_SET3DMODE = 0xc,
    HWC_LAYER_SETFORMAT = 0xd,
    HWC_LAYER_VPPON = 0xe,
    HWC_LAYER_VPPGETON = 0xf,
    HWC_LAYER_SETLUMASHARP = 0x10,
    HWC_LAYER_GETLUMASHARP = 0x11,
    HWC_LAYER_SETCHROMASHARP = 0x12,
    HWC_LAYER_GETCHROMASHARP = 0x13,
    HWC_LAYER_SETWHITEEXTEN = 0x14,
    HWC_LAYER_GETWHITEEXTEN = 0x15,
    HWC_LAYER_SETBLACKEXTEN = 0x16,
    HWC_LAYER_GETBLACKEXTEN = 0x17,
    HWC_LAYER_SET_3D_PARALLAX = 0x18,
    HWC_LAYER_SET_SCREEN_PARA = 0x19,

	HWC_LAYER_SETTOP		= 0x1a,
	HWC_LAYER_SETBOTTOM		= 0x1b,
}e_hwc_layer_cmd_t;

typedef enum e_hwc_mode
{
	HWC_MODE_SCREEN0                = 0,
	HWC_MODE_SCREEN1                = 1,
	HWC_MODE_SCREEN0_TO_SCREEN1     = 2,
	HWC_MODE_SCREEN0_AND_SCREEN1    = 3,
	HWC_MODE_SCREEN0_BE             = 4,
	HWC_MODE_SCREEN0_GPU            = 5,
}e_hwc_mode_t;

typedef struct tag_HWCLayerInitPara
{
	uint32_t		w;
	uint32_t		h;
	uint32_t		format;
	uint32_t		screenid;
}layerinitpara_t;

typedef struct tag_Video3DInfo
{
	unsigned int width;
	unsigned int height;
	e_hwc_format_t format;
	e_hwc_3d_src_mode_t src_mode;
	e_hwc_3d_out_mode_t display_mode;
}video3Dinfo_t;

typedef struct tag_LIBHWCLAYERPARA
{
    unsigned long   number;

    unsigned long   top_y;              // the address of frame buffer, which contains top field luminance
    unsigned long   top_c;              // the address of frame buffer, which contains top field chrominance
    unsigned long   bottom_y;           // the address of frame buffer, which contains bottom field luminance
    unsigned long   bottom_c;           // the address of frame buffer, which contains bottom field chrominance

    signed char     bProgressiveSrc;    // Indicating the source is progressive or not
    signed char     bTopFieldFirst;     // VPO should check this flag when bProgressiveSrc is FALSE
    unsigned long   flag_addr;          //dit maf flag address
    unsigned long   flag_stride;        //dit maf flag line stride
    unsigned char   maf_valid;
    unsigned char   pre_frame_valid;
}libhwclayerpara_t;


typedef struct screen_para
{
    unsigned int width[2];//screen total width
    unsigned int height[2];//screen total height
    unsigned int valid_width[2];//screen width that can be seen
    unsigned int valid_height[2];//screen height that can be seen
    unsigned int app_width[2];//the width that app use
    unsigned int app_height[2];//the height that app use
}screen_para_t;
#endif

typedef struct tag_VIRTUALLIBHWCLAYERPARA
{
    unsigned long   number;             //frame_id

    cedarv_3d_mode_e            source3dMode;   //cedarv_3d_mode_e, CEDARV_3D_MODE_DOUBLE_STREAM
    cedarx_display_3d_mode_e    displayMode;    //cedarx_display_3d_mode_e, CEDARX_DISPLAY_3D_MODE_3D
    cedarv_pixel_format_e       pixel_format;   //cedarv_pixel_format_e, CEDARV_PIXEL_FORMAT_MB_UV_COMBINE_YUV420
    
    unsigned long   top_y;              // the address of frame buffer, which contains top field luminance
    unsigned long   top_u;  //top_c;              // the address of frame buffer, which contains top field chrominance
    unsigned long   top_v;
    unsigned long   top_y2;
    unsigned long   top_u2;
    unsigned long   top_v2;

    unsigned long   size_top_y;
    unsigned long   size_top_u;
    unsigned long   size_top_v;
    unsigned long   size_top_y2;
    unsigned long   size_top_u2;
    unsigned long   size_top_v2;
    
    signed char     bProgressiveSrc;    // Indicating the source is progressive or not
    signed char     bTopFieldFirst;     // VPO should check this flag when bProgressiveSrc is FALSE
    unsigned long   flag_addr;          //dit maf flag address
    unsigned long   flag_stride;        //dit maf flag line stride
    unsigned char   maf_valid;
    unsigned char   pre_frame_valid;
}Virtuallibhwclayerpara;   //libhwclayerpara_t, [hwcomposer.h]

extern int convertlibhwclayerpara_NativeRendererVirtual2Arch(libhwclayerpara_t *pDes, Virtuallibhwclayerpara *pSrc);
extern int convertlibhwclayerpara_SoftwareRendererVirtual2Arch(libhwclayerpara_t *pDes, Virtuallibhwclayerpara *pSrc);

}
#endif  /* _VIRTUAL_HWCOMPOSER_H_ */

