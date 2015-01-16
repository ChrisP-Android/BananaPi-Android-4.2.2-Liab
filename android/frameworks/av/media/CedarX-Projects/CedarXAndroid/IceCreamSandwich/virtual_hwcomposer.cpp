/*
********************************************************************************
*                                    Android
*                               CedarX sub-system
*                               CedarXAndroid module
*
*          (c) Copyright 2010-2013, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : virtual_hwcomposer.cpp
* Version: V1.0
* By     : Eric_wang
* Date   : 2012-2-26
* Description:
********************************************************************************
*/
#include <utils/Errors.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <hardware/hwcomposer.h>
#include "virtual_hwcomposer.h"

//#if (CEDARX_ANDROID_VERSION == 6)
//#elif (CEDARX_ANDROID_VERSION == 7)
namespace android {

#if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51))
int convertlibhwclayerpara_NativeRendererVirtual2Arch(libhwclayerpara_t *pDes, Virtuallibhwclayerpara *pSrc)
{
    memset(pDes, 0, sizeof(libhwclayerpara_t));
    pDes->number            = pSrc->number;
    pDes->bProgressiveSrc   = pSrc->bProgressiveSrc;
    pDes->bTopFieldFirst    = pSrc->bTopFieldFirst;
    pDes->flag_addr         = pSrc->flag_addr;
    pDes->flag_stride       = pSrc->flag_stride;    
    pDes->maf_valid         = pSrc->maf_valid;      
    pDes->pre_frame_valid   = pSrc->pre_frame_valid;
    if(pSrc->source3dMode == CEDARV_3D_MODE_DOUBLE_STREAM && pSrc->displayMode == CEDARX_DISPLAY_3D_MODE_3D)
    {
        pDes->top_y         = pSrc->top_y;
		pDes->top_c         = pSrc->top_u;
		pDes->bottom_y      = pSrc->top_y2;
		pDes->bottom_c      = pSrc->top_u2;
    }
    else if(pSrc->displayMode == CEDARX_DISPLAY_3D_MODE_ANAGLAGH)
	{
        pDes->top_y         = pSrc->top_u2;
		pDes->top_c         = pSrc->top_y2;
		pDes->bottom_y      = pSrc->top_v2;
		pDes->bottom_c      = 0;
    }
    else
    {
        pDes->top_y             = pSrc->top_y;
        pDes->top_c             = pSrc->top_u;
        pDes->bottom_y          = 0;
        pDes->bottom_c          = 0;
    }
    return OK;
}
#elif defined(__CHIP_VERSION_F33)
int convertlibhwclayerpara_NativeRendererVirtual2Arch(libhwclayerpara_t *pDes, Virtuallibhwclayerpara *pSrc)
{
    memset(pDes, 0, sizeof(libhwclayerpara_t));
    pDes->number            = pSrc->number;
    pDes->bProgressiveSrc   = pSrc->bProgressiveSrc;
    pDes->bTopFieldFirst    = pSrc->bTopFieldFirst;
    pDes->flag_addr         = pSrc->flag_addr;
    pDes->flag_stride       = pSrc->flag_stride;    
    pDes->maf_valid         = pSrc->maf_valid;      
    pDes->pre_frame_valid   = pSrc->pre_frame_valid;
    if(pSrc->pixel_format != CEDARV_PIXEL_FORMAT_PLANNER_YVU420)    //MB32
    {
        if(pSrc->source3dMode == CEDARV_3D_MODE_DOUBLE_STREAM && pSrc->displayMode == CEDARX_DISPLAY_3D_MODE_3D)
        {
            pDes->addr[0] 		  = pSrc->top_y;
    		pDes->addr[1] 		  = pSrc->top_u;
    		pDes->addr[2] 		  = 0;
    		pDes->addr_3d_right[0] = pSrc->top_y2;
    		pDes->addr_3d_right[1] = pSrc->top_u2;
    		pDes->addr_3d_right[2] = 0;
        }
        else if(pSrc->displayMode == CEDARX_DISPLAY_3D_MODE_ANAGLAGH)
    	{
            pDes->addr[0] = pSrc->top_u2;    //* R
    		pDes->addr[1] = pSrc->top_y2;    //* G
    		pDes->addr[2] = pSrc->top_v2;    //* B
    		pDes->addr_3d_right[0] = 0;
    		pDes->addr_3d_right[1] = 0;
    		pDes->addr_3d_right[2] = 0;
        }
        else
        {
            pDes->addr[0] 		  = pSrc->top_y;
    		pDes->addr[1] 		  = pSrc->top_u;
    		pDes->addr[2] 		  = pSrc->top_v;
    		pDes->addr_3d_right[0] = 0;
    		pDes->addr_3d_right[1] = 0;
    		pDes->addr_3d_right[2] = 0;
        }
    }
    else
    {
        pDes->addr[0] 				= pSrc->top_y;
		pDes->addr[1] 				= pSrc->top_v;
		pDes->addr[2] 				= pSrc->top_u;
		pDes->addr_3d_right[0] 		= 0;
		pDes->addr_3d_right[1]	 	= 0;
		pDes->addr_3d_right[2]	 	= 0;
    }
    return OK;
}
int convertlibhwclayerpara_SoftwareRendererVirtual2Arch(libhwclayerpara_t *pDes, Virtuallibhwclayerpara *pSrc)
{
    memset(pDes, 0, sizeof(libhwclayerpara_t));
    pDes->number            = pSrc->number;
    pDes->bProgressiveSrc   = pSrc->bProgressiveSrc;
    pDes->bTopFieldFirst    = pSrc->bTopFieldFirst;
    pDes->flag_addr         = pSrc->flag_addr;
    pDes->flag_stride       = pSrc->flag_stride;    
    pDes->maf_valid         = pSrc->maf_valid;      
    pDes->pre_frame_valid   = pSrc->pre_frame_valid;
    pDes->addr[0] 				= pSrc->top_y;
	pDes->addr[1] 				= pSrc->top_v;
	pDes->addr[2] 				= pSrc->top_u;
	pDes->addr_3d_right[0] 		= 0;
	pDes->addr_3d_right[1]	 	= 0;
	pDes->addr_3d_right[2]	 	= 0;
    return OK;
}
#else
    #error "Unknown chip type!"
#endif

}; // namespace android
