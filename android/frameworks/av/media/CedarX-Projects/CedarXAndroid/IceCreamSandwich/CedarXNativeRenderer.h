/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef CEDARXNATIVE_RENDERER_H_

#define CEDARXNATIVE_RENDERER_H_


#include <utils/RefBase.h>
#if (CEDARX_ANDROID_VERSION < 7)
#include <ui/android_native_buffer.h>
#endif
#include <hardware/hwcomposer.h>
#include "virtual_hwcomposer.h"

namespace android {

#if (CEDARX_ANDROID_VERSION == 6 && CEDARX_ADAPTER_VERSION == 4)
typedef enum tag_VirtualHWC3DSrcMode  
{
    VIRTUAL_HWC_3D_SRC_MODE_TB  = HWC_3D_OUT_MODE_TB,//top bottom, 
    VIRTUAL_HWC_3D_SRC_MODE_FP  = HWC_3D_OUT_MODE_FP,//frame packing
    VIRTUAL_HWC_3D_SRC_MODE_SSF = HWC_3D_OUT_MODE_SSF,//side by side full
    VIRTUAL_HWC_3D_SRC_MODE_SSH = HWC_3D_OUT_MODE_SSH,//side by side half
    VIRTUAL_HWC_3D_SRC_MODE_LI  = HWC_3D_OUT_MODE_LI,//line interleaved

    VIRTUAL_HWC_3D_SRC_MODE_NORMAL = HWC_3D_OUT_MODE_NORMAL, //2d
    VIRTUAL_HWC_3D_SRC_MODE_UNKNOWN = -1,
}VirtualHWC3DSrcMode;   //virtual to HWC_3D_OUT_MODE_TB
typedef enum tag_VirtualHWCDisplayMode  
{
    VIRTUAL_HWC_3D_OUT_MODE_2D 		            = HWC_DISP_MODE_2D,//left picture
    VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_1080P24_FP 	= HWC_DISP_MODE_3D,
    VIRTUAL_HWC_3D_OUT_MODE_ANAGLAGH 	        = HWC_DISP_MODE_ANAGLAGH,//
    VIRTUAL_HWC_3D_OUT_MODE_ORIGINAL 	        = HWC_DISP_MODE_ORIGINAL,//original pixture

    VIRTUAL_HWC_3D_OUT_MODE_LI                  = HWC_3D_OUT_MODE_LI,//line interleaved
    VIRTUAL_HWC_3D_OUT_MODE_CI_1                = HWC_3D_OUT_MODE_CI_1,//column interlaved 1
    VIRTUAL_HWC_3D_OUT_MODE_CI_2                = HWC_3D_OUT_MODE_CI_2,//column interlaved 2
    VIRTUAL_HWC_3D_OUT_MODE_CI_3                = HWC_3D_OUT_MODE_CI_3,//column interlaved 3
    VIRTUAL_HWC_3D_OUT_MODE_CI_4                = HWC_3D_OUT_MODE_CI_4,//column interlaved 4

    VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_720P50_FP   = HWC_DISP_MODE_2D,
    VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_720P60_FP   = HWC_DISP_MODE_2D,
    VIRTUAL_HWC_3D_OUT_MODE_UNKNOWN     = -1,
}VirtualHWCDisplayMode;   //virtual to HWC_DISP_MODE_ANAGLAGH
#elif ((CEDARX_ANDROID_VERSION == 6) || (CEDARX_ANDROID_VERSION == 7) || (CEDARX_ANDROID_VERSION == 8) || (CEDARX_ANDROID_VERSION == 9))
typedef enum tag_VirtualHWC3DSrcMode  
{
    VIRTUAL_HWC_3D_SRC_MODE_TB  = HWC_3D_SRC_MODE_TB,//top bottom, 
    VIRTUAL_HWC_3D_SRC_MODE_FP  = HWC_3D_SRC_MODE_FP,//frame packing
    VIRTUAL_HWC_3D_SRC_MODE_SSF = HWC_3D_SRC_MODE_SSF,//side by side full
    VIRTUAL_HWC_3D_SRC_MODE_SSH = HWC_3D_SRC_MODE_SSH,//side by side half
    VIRTUAL_HWC_3D_SRC_MODE_LI  = HWC_3D_SRC_MODE_LI,//line interleaved

    VIRTUAL_HWC_3D_SRC_MODE_NORMAL = HWC_3D_SRC_MODE_NORMAL, //2d
    VIRTUAL_HWC_3D_SRC_MODE_UNKNOWN = -1,
}VirtualHWC3DSrcMode;   //virtual to HWC_3D_SRC_MODE_TB
typedef enum tag_VirtualHWCDisplayMode  
{
    VIRTUAL_HWC_3D_OUT_MODE_2D 		            = HWC_3D_OUT_MODE_2D,//left picture
    VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_1080P24_FP 	= HWC_3D_OUT_MODE_HDMI_3D_1080P24_FP,
    VIRTUAL_HWC_3D_OUT_MODE_ANAGLAGH 	        = HWC_3D_OUT_MODE_ANAGLAGH,//
    VIRTUAL_HWC_3D_OUT_MODE_ORIGINAL 	        = HWC_3D_OUT_MODE_ORIGINAL,//original pixture

    VIRTUAL_HWC_3D_OUT_MODE_LI                  = HWC_3D_OUT_MODE_LI,//line interleaved
    VIRTUAL_HWC_3D_OUT_MODE_CI_1                = HWC_3D_OUT_MODE_CI_1,//column interlaved 1
    VIRTUAL_HWC_3D_OUT_MODE_CI_2                = HWC_3D_OUT_MODE_CI_2,//column interlaved 2
    VIRTUAL_HWC_3D_OUT_MODE_CI_3                = HWC_3D_OUT_MODE_CI_3,//column interlaved 3
    VIRTUAL_HWC_3D_OUT_MODE_CI_4                = HWC_3D_OUT_MODE_CI_4,//column interlaved 4

    VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_720P50_FP   = HWC_3D_OUT_MODE_HDMI_3D_720P50_FP,
    VIRTUAL_HWC_3D_OUT_MODE_HDMI_3D_720P60_FP   = HWC_3D_OUT_MODE_HDMI_3D_720P60_FP,
    VIRTUAL_HWC_3D_OUT_MODE_UNKNOWN     = -1,
}VirtualHWCDisplayMode;   //virtual to HWC_3D_OUT_MODE_ANAGLAGH
#else
    #error "Unknown chip type!"
#endif

typedef struct tag_VirtualVideo3DInfo
{
	unsigned int        width;
	unsigned int        height;
	unsigned int        format; // HWC_FORMAT_MBYUV422(hwcomposer.h) or HAL_PIXEL_FORMAT_YV12(graphics.h).
	VirtualHWC3DSrcMode     src_mode;   //virtual to HWC_3D_SRC_MODE_TB
	VirtualHWCDisplayMode   display_mode;   //virtual to HWC_3D_OUT_MODE_ANAGLAGH(hwcomposer.h)
}VirtualVideo3DInfo;    //virtual to video3Dinfo_t

enum VIDEORENDER_CMD
{
	VIDEORENDER_CMD_UNKOWN = 0,
    VIDEORENDER_CMD_ROTATION_DEG    = HWC_LAYER_ROTATION_DEG, //HWC_LAYER_ROTATION_DEG    ,
    VIDEORENDER_CMD_DITHER          = HWC_LAYER_DITHER, //HWC_LAYER_DITHER          ,
    VIDEORENDER_CMD_SETINITPARA     = HWC_LAYER_SETINITPARA, //HWC_LAYER_SETINITPARA     ,
    VIDEORENDER_CMD_SETVIDEOPARA    = HWC_LAYER_SETVIDEOPARA, //HWC_LAYER_SETVIDEOPARA    ,
    VIDEORENDER_CMD_SETFRAMEPARA    = HWC_LAYER_SETFRAMEPARA, //HWC_LAYER_SETFRAMEPARA    ,
    VIDEORENDER_CMD_GETCURFRAMEPARA = HWC_LAYER_GETCURFRAMEPARA, //HWC_LAYER_GETCURFRAMEPARA ,
    VIDEORENDER_CMD_QUERYVBI        = HWC_LAYER_QUERYVBI, //HWC_LAYER_QUERYVBI        ,
#if (CEDARX_ANDROID_VERSION == 6 && CEDARX_ADAPTER_VERSION == 4)
    VIDEORENDER_CMD_SETSCREEN       = HWC_LAYER_SETSCREEN, //HWC_LAYER_SETMODE       ,
#elif ((CEDARX_ANDROID_VERSION == 6) || (CEDARX_ANDROID_VERSION == 7) || (CEDARX_ANDROID_VERSION == 8) || (CEDARX_ANDROID_VERSION == 9))
    VIDEORENDER_CMD_SETSCREEN       = HWC_LAYER_SETMODE,
#else
    #error "Unknown chip type!"
#endif
    VIDEORENDER_CMD_SHOW            = HWC_LAYER_SHOW, //HWC_LAYER_SHOW            ,
    VIDEORENDER_CMD_RELEASE         = HWC_LAYER_RELEASE, //HWC_LAYER_RELEASE         ,
    VIDEORENDER_CMD_SET3DMODE       = HWC_LAYER_SET3DMODE, //HWC_LAYER_SET3DMODE       ,
    VIDEORENDER_CMD_SETFORMAT       = HWC_LAYER_SETFORMAT, //HWC_LAYER_SETFORMAT       ,
    VIDEORENDER_CMD_VPPON           = HWC_LAYER_VPPON, //HWC_LAYER_VPPON           ,
    VIDEORENDER_CMD_VPPGETON        = HWC_LAYER_VPPGETON, //HWC_LAYER_VPPGETON        ,
    VIDEORENDER_CMD_SETLUMASHARP    = HWC_LAYER_SETLUMASHARP,//HWC_LAYER_SETLUMASHARP    ,
    VIDEORENDER_CMD_GETLUMASHARP    = HWC_LAYER_GETLUMASHARP, //HWC_LAYER_GETLUMASHARP    ,
    VIDEORENDER_CMD_SETCHROMASHARP  = HWC_LAYER_SETCHROMASHARP, //HWC_LAYER_SETCHROMASHARP  ,
    VIDEORENDER_CMD_GETCHROMASHARP  = HWC_LAYER_GETCHROMASHARP,//HWC_LAYER_GETCHROMASHARP  ,
    VIDEORENDER_CMD_SETWHITEEXTEN   = HWC_LAYER_SETWHITEEXTEN, //HWC_LAYER_SETWHITEEXTEN   ,
    VIDEORENDER_CMD_GETWHITEEXTEN   = HWC_LAYER_GETWHITEEXTEN, //HWC_LAYER_GETWHITEEXTEN   ,
    VIDEORENDER_CMD_SETBLACKEXTEN   = HWC_LAYER_SETBLACKEXTEN, //HWC_LAYER_SETBLACKEXTEN   ,
    VIDEORENDER_CMD_GETBLACKEXTEN   = HWC_LAYER_GETBLACKEXTEN, //HWC_LAYER_GETBLACKEXTEN   ,

    VIDEORENDER_CMD_SETLAYERORDER   = 0x100,                   //DISP_CMD_LAYER_TOP, DISP_CMD_LAYER_BOTTOM ,
    VIDEORENDER_CMD_SET_CROP		= 0x1000         ,
};

struct MetaData;
//class CedarXNativeRendererAdapter;

class CedarXNativeRenderer
{
public:
    CedarXNativeRenderer(const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta);

    ~CedarXNativeRenderer();

    void render(const void *data, size_t size, void *platformPrivate);

    int control(int cmd, int para);

private:
    //OMX_COLOR_FORMATTYPE mColorFormat;
    sp<ANativeWindow> mNativeWindow;
    int32_t mWidth, mHeight;
    int32_t mCropLeft, mCropTop, mCropRight, mCropBottom;
    int32_t mCropWidth, mCropHeight;
    int32_t mLayerShowed;

    CedarXNativeRenderer(const CedarXNativeRenderer &);
    CedarXNativeRenderer &operator=(const CedarXNativeRenderer &);
    
    //friend class CedarXNativeRendererAdapter;
    //CedarXNativeRendererAdapter *pCedarXNativeRendererAdapter;
};


//class CedarXNativeRendererAdapter  //base adapter, its implementation is put in different AdapterDirectories, selected to compile by makefile according to android version.
//{
//public:
//    CedarXNativeRendererAdapter(CedarXNativeRenderer *vRender);
//    virtual ~CedarXNativeRendererAdapter();
//    int CedarXNativeRendererAdapterIoCtrl(int cmd, int aux, void *pbuffer);  //cmd = CedarXNativeRendererAdapterCmd
//    
//private:
//    CedarXNativeRenderer* const pCedarXNativeRenderer; //CedarXNativeRenderer pointer
//};

}  // namespace android

#endif  // SOFTWARE_RENDERER_H_
