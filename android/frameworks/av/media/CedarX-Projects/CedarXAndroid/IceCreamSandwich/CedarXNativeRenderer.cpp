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

#define LOG_TAG "CedarXNativeRenderer"
#include <CDX_Debug.h>

#include "CedarXNativeRenderer.h"
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MetaData.h>

#include <binder/MemoryHeapBase.h>
#if (CEDARX_ANDROID_VERSION < 7)
#include <binder/MemoryHeapPmem.h>
#include <surfaceflinger/Surface.h>
#include <ui/android_native_buffer.h>
#endif

#include <ui/GraphicBufferMapper.h>
#include <gui/ISurfaceTexture.h>

namespace android {

CedarXNativeRenderer::CedarXNativeRenderer(const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta)
    : mNativeWindow(nativeWindow)
{

    int32_t halFormat,screenID;
    size_t bufWidth, bufHeight;

    CHECK(meta->findInt32(kKeyScreenID, &screenID));
    CHECK(meta->findInt32(kKeyColorFormat, &halFormat));
    CHECK(meta->findInt32(kKeyWidth, &mWidth)); //width and height is display_width and display_height.
    CHECK(meta->findInt32(kKeyHeight, &mHeight));

    int32_t rotationDegrees;
    if (!meta->findInt32(kKeyRotation, &rotationDegrees))
        rotationDegrees = 0;
    
    //when YV12:
    //vdec_buffer's Y must be 16 pixel align in width, 16 lines align in height. (16*16)
    //vdec_buffer's V now is 16 pixel align in width, 8 lines align in height(16*8). we want to change to 8pixel*8line(8*8)!
    
    //when MB32: one Y_MB is 32pixel * 32 lines. spec is 16*16, but hw decoder extend to 32*32!
    //vdec_buffer's Y must be 32pixel * 32line(32*32)
    //vdec_buffer's uv must be 32byte * 32line(32*32). uv combined to store, so if u is a byte, v is a byte, then 32byte for 16 pixel_point. 
    //  uv may be large than y, so uv MB will partly be filled with stuf data.
    
//    bufWidth = (mWidth + 15) & ~15;  //the vdec_buffer's width and height which will be told to display device .
//    bufHeight = (mHeight + 15) & ~15;
    bufWidth = mWidth;  //the vdec_buffer's width and height which will be told to display device .
    bufHeight = mHeight;
    if(bufWidth != ((mWidth + 15) & ~15))
    {
        LOGW("(f:%s, l:%d) bufWidth[%d]!=display_width[%d]", __FUNCTION__, __LINE__, ((mWidth + 15) & ~15), mWidth);
    }
    if(bufHeight != ((mHeight + 15) & ~15))
    {
        LOGW("(f:%s, l:%d) bufHeight[%d]!=display_height[%d]", __FUNCTION__, __LINE__, ((mHeight + 15) & ~15), mHeight);
    }

    CHECK(mNativeWindow != NULL);

    if(halFormat == HAL_PIXEL_FORMAT_YV12)
    {
        native_window_set_usage(mNativeWindow.get(),
        		                GRALLOC_USAGE_SW_READ_NEVER  |
        		                GRALLOC_USAGE_SW_WRITE_OFTEN |
        		                GRALLOC_USAGE_HW_TEXTURE     |
        		                GRALLOC_USAGE_EXTERNAL_DISP);

//        native_window_set_scaling_mode(mNativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);

        // Width must be multiple of 32???
        native_window_set_buffers_geometry(mNativeWindow.get(),
        								   bufWidth,
        								   bufHeight,
                                           HWC_FORMAT_YUV420PLANAR);

        uint32_t transform;
        switch (rotationDegrees)
        {
            case 0: transform = 0; break;
            case 90: transform = HAL_TRANSFORM_ROT_90; break;
            case 180: transform = HAL_TRANSFORM_ROT_180; break;
            case 270: transform = HAL_TRANSFORM_ROT_270; break;
            default: transform = 0; break;
        }

        if (transform)
            native_window_set_buffers_transform(mNativeWindow.get(), transform);
    }
    else
    {
        native_window_set_usage(mNativeWindow.get(),
        		                GRALLOC_USAGE_SW_READ_NEVER  |
        		                GRALLOC_USAGE_SW_WRITE_OFTEN |
        		                GRALLOC_USAGE_HW_TEXTURE     |
        		                GRALLOC_USAGE_EXTERNAL_DISP);
#if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51)) //a10 do this, a10'sdk is old.
        native_window_set_scaling_mode(mNativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);

        // Width must be multiple of 32???
        native_window_set_buffers_geometryex(mNativeWindow.get(),
        		                             bufWidth,
                                             bufHeight,
                                             halFormat,
                                             screenID);
#elif defined(__CHIP_VERSION_F33)
        //        native_window_set_scaling_mode(mNativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);

        // Width must be multiple of 32???
        native_window_set_buffers_geometry(mNativeWindow.get(),
            		                       bufWidth,
                                           bufHeight,
                                           halFormat);
#else
        #error "Unknown chip type!"
#endif

        uint32_t transform;
        switch (rotationDegrees)
        {
            case 0: transform = 0; break;
            case 90: transform = HAL_TRANSFORM_ROT_90; break;
            case 180: transform = HAL_TRANSFORM_ROT_180; break;
            case 270: transform = HAL_TRANSFORM_ROT_270; break;
            default: transform = 0; break;
        }

        if (transform)
            native_window_set_buffers_transform(mNativeWindow.get(), transform);
    }

    mLayerShowed = 0;

//    pCedarXNativeRendererAdapter = new CedarXNativeRendererAdapter(this);
//    if(NULL == pCedarXNativeRendererAdapter)
//    {
//        LOGW("create CedarXNativeRendererAdapter fail");
//    }
}

CedarXNativeRenderer::~CedarXNativeRenderer()
{
//    if(pCedarXNativeRendererAdapter)
//    {
//        delete pCedarXNativeRendererAdapter;
//        pCedarXNativeRendererAdapter = NULL;
//    }
}

void CedarXNativeRenderer::render(const void *data, size_t size, void *platformPrivate)
{
    Virtuallibhwclayerpara *pVirtuallibhwclayerpara = (Virtuallibhwclayerpara*)data;
    libhwclayerpara_t   overlay_para;
    convertlibhwclayerpara_NativeRendererVirtual2Arch(&overlay_para, pVirtuallibhwclayerpara);
    mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SETPARAMETER, HWC_LAYER_SETFRAMEPARA, (uint32_t)(&overlay_para));
}

static int convertVirtualVideo3DInfo2video3Dinfo_t(video3Dinfo_t *pDes, VirtualVideo3DInfo *pSrc)
{
#if (CEDARX_ANDROID_VERSION == 6) || (((CEDARX_ANDROID_VERSION == 8)||(CEDARX_ANDROID_VERSION == 9)) && (defined (__CHIP_VERSION_F23)))
    memset(pDes, 0, sizeof(video3Dinfo_t));
    pDes->width = pSrc->width;
	pDes->height = pSrc->height;
	pDes->format = pSrc->format;
    pDes->src_mode = pSrc->src_mode;
	pDes->display_mode = pSrc->display_mode;
#elif (CEDARX_ANDROID_VERSION >= 7)
    memset(pDes, 0, sizeof(video3Dinfo_t));
    pDes->width = pSrc->width;
	pDes->height = pSrc->height;
	pDes->format = (e_hwc_format_t)pSrc->format;
    pDes->src_mode = (e_hwc_3d_src_mode_t)pSrc->src_mode;
	pDes->display_mode = (e_hwc_3d_out_mode_t)pSrc->display_mode;
#else
    #error "Unknown chip type!"
#endif
    return OK;
}
int CedarXNativeRenderer::control(int cmd, int para)
{
#if 1
	switch(cmd){
	case VIDEORENDER_CMD_ROTATION_DEG    :  
	case VIDEORENDER_CMD_DITHER          :  
	case VIDEORENDER_CMD_SETINITPARA     :  
	case VIDEORENDER_CMD_SETVIDEOPARA    :  
	case VIDEORENDER_CMD_SETFRAMEPARA    :  
	case VIDEORENDER_CMD_GETCURFRAMEPARA :  
	case VIDEORENDER_CMD_QUERYVBI        :  
	case VIDEORENDER_CMD_SETSCREEN       :  
	case VIDEORENDER_CMD_SHOW            :  
	case VIDEORENDER_CMD_RELEASE         :  
    {
    	if(cmd == VIDEORENDER_CMD_GETCURFRAMEPARA && mLayerShowed == 0)
    		return -2;

    	if(cmd == VIDEORENDER_CMD_SHOW)
    	{
    		if(para == 0)
    			mLayerShowed = 0;
    		else
    			mLayerShowed = 1;
    	}

        return mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SETPARAMETER, cmd, para);
	}
	case VIDEORENDER_CMD_SET3DMODE       :  
    {
        //para = &VirtualVideo3DInfo
        video3Dinfo_t _3d_info;
        convertVirtualVideo3DInfo2video3Dinfo_t(&_3d_info, (VirtualVideo3DInfo*)para);
	    return mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SETPARAMETER, cmd, &_3d_info);
	}
	case VIDEORENDER_CMD_SETFORMAT       :  
	case VIDEORENDER_CMD_VPPON           :  
	case VIDEORENDER_CMD_VPPGETON        :  
	case VIDEORENDER_CMD_SETLUMASHARP    :  
	case VIDEORENDER_CMD_GETLUMASHARP    :  
	case VIDEORENDER_CMD_SETCHROMASHARP  :  
	case VIDEORENDER_CMD_GETCHROMASHARP  :  
	//case VIDEORENDER_CMD_SETWHITEEXTEN   :
	//case VIDEORENDER_CMD_GETWHITEEXTEN   :
	case VIDEORENDER_CMD_SETBLACKEXTEN   :
	case VIDEORENDER_CMD_GETBLACKEXTEN   :
		return mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SETPARAMETER, cmd, para);
	case VIDEORENDER_CMD_SET_CROP		:
		return mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SET_CROP,para);
	default:
		LOGW("undefined command!");
		break;
	}

    return 0;
#else
    return pCedarXNativeRendererAdapter->CedarXNativeRendererAdapterIoCtrl(cmd, para, NULL);
#endif
}

}  // namespace android

