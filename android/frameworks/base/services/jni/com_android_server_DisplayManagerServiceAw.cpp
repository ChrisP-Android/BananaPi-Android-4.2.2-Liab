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

#define LOG_TAG "DisplayManagerServiceAw"

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include <utils/misc.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/display.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayCommand.h>
#include <stdio.h>

namespace android
{    
	static void init_native(JNIEnv *env, jobject clazz)
	{
	}

	static void finalize_native(JNIEnv *env, jobject clazz)
	{
	}

    static jint openDisplay_native(JNIEnv *env, jobject clazz,int displayno)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_OPENDISP,displayno,0,0);
    }

    static jint closeDisplay_native(JNIEnv *env, jobject clazz, int displayno)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_CLOSEDISP,displayno,0,0);
    }

    static jint setDisplayMode_native(JNIEnv *env, jobject clazz,int mode)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SETDISPMODE,mode,0,0);
    }

	static jint changeMode_native(JNIEnv *env, jobject clazz,int displayno, int value0,int value1)
	{
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_CHANGEDISPMODE,displayno,value0,value1);
	}

    static jint setDisplayParameter_native(JNIEnv *env, jobject clazz,int displayno, int value0,int value1)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SETDISPPARA,displayno,value0,value1);
    }

    static jint getDisplayParameter_native(JNIEnv *env, jobject clazz,int displayno, int param)
    {
	    return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPPARA,displayno,param,0);
    }

    static jint getHdmiStatus_native(JNIEnv *env, jobject clazz)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETHDMISTATUS,0,0,0);
    }
    
    static jint getMaxHdmiMode_native(JNIEnv *env, jobject clazz)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETMAXHDMIMODE,0,0,0);
    }

    static jint getTvDacStatus_native(JNIEnv *env, jobject clazz)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETTVSTATUS,0,0,0);
    }

    static jint getDisplayMode_native(JNIEnv *env, jobject clazz)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPLAYMODE,0,0,0);
    }

    static jint getDisplayCount_native(JNIEnv *env, jobject clazz)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPCOUNT,0,0,0);
    }

    static jint setDisplayMaster_native(JNIEnv *env, jobject clazz,int displayno)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SETMASTERDISP,displayno,0,0);
    }

    static jint getDisplayMaster_native(JNIEnv *env, jobject clazz)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETMASTERDISP,0,0,0);
    }
    
    static jint getMaxWidthDisplay_native(JNIEnv *env, jobject clazz)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETMAXWIDTHDISP,0,0,0);
    }

    static jint getDisplayOutputType_native(JNIEnv *env, jobject clazz,int displayno)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPPARA,displayno,DISPLAY_OUTPUT_TYPE,0);
    }
    
    static jint getDisplayOutputFormat_native(JNIEnv *env, jobject clazz,int displayno)
    {
        return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPPARA,displayno,DISPLAY_OUTPUT_FORMAT,0);
    }
    
    static jint getDisplayOutputWidth_native(JNIEnv *env, jobject clazz,int displayno)
    {
    	return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPPARA,displayno,DISPLAY_OUTPUT_WIDTH,0);
    }
    
    static jint getDisplayOutputHeight_native(JNIEnv *env, jobject clazz,int displayno)
    {
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPPARA,displayno,DISPLAY_OUTPUT_HEIGHT,0);
    }
    
    static jint getDisplayOutputPixelFormat_native(JNIEnv *env, jobject clazz,int displayno)
    {                	            
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPPARA,displayno,DISPLAY_OUTPUT_PIXELFORMAT,0);
    }
    
    static jint getDisplayOutputOpen_native(JNIEnv *env, jobject clazz,int displayno)
    {                	            
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPPARA,displayno,DISPLAY_OUTPUT_ISOPEN,0);
    }
    
    static jint getDisplayOutputHotPlug_native(JNIEnv *env, jobject clazz,int displayno)
    {                
	    return  (jint)SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETDISPPARA,displayno,DISPLAY_OUTPUT_HOTPLUG,0);
    }
	
    static jint SetDisplayBacklihgtMode_native(JNIEnv *env, jobject clazz,int mode)
    {
		return	SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SETBACKLIGHTMODE,mode,0,0);
    }
    
    static jint setDisplayAreaPercent_native(JNIEnv *env, jobject clazz,int displayno,int percent)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SETAREAPERCENT,displayno,percent,0);
    }
    
    static jint getDisplayAreaPercent_native(JNIEnv *env, jobject clazz,int displayno)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETAREAPERCENT,displayno,0,0);
    }

    static jint isSupportHdmiMode_native(JNIEnv *env, jobject clazz,int mode)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_ISSUPPORTHDMIMODE,mode,0,0);
    }

    static jint isSupport3DMode_native(JNIEnv *env, jobject clazz)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_ISSUPPORTHDMIMODE,DISPLAY_TVFORMAT_1080P_24HZ_3D_FP,0,0);
    }
    
    static jint setDisplayBright_native(JNIEnv *env, jobject clazz,int displayno,int bright)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SETBRIGHT,displayno,bright,0);
    }

    static jint getDisplayBright_native(JNIEnv *env, jobject clazz,int displayno)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETBRIGHT,displayno,0,0);
    }
    
    static jint setDisplayContrast_native(JNIEnv *env, jobject clazz,int displayno,int contrast)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SETCONTRAST,displayno,contrast,0);
    }
    
    static jint getDisplayContrast_native(JNIEnv *env, jobject clazz,int displayno)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETCONTRAST,displayno,0,0);
    }
    
    static jint setDisplaySaturation_native(JNIEnv *env, jobject clazz,int displayno,int saturation)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SETSATURATION,displayno,saturation,0);
    }
    
    static jint getDisplaySaturation_native(JNIEnv *env, jobject clazz,int displayno)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETSATURATION,displayno,0,0);
    }

    static jint setDisplayHue_native(JNIEnv *env, jobject clazz,int displayno,int hue)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SETHUE,displayno,hue,0);
    }
    
    static jint getDisplayHue_native(JNIEnv *env, jobject clazz,int displayno)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_GETHUE,displayno,0,0);
    }
    static jint StartWifiDisplaySend_native(JNIEnv *env, jobject clazz,int displayno,int mode)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_STARTWIFIDISPLAYSEND,displayno,mode,0);
    }

    static jint EndWifiDisplaySend_native(JNIEnv *env, jobject clazz,int displayno)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_ENDWIFIDISPLAYSEND,displayno,0,0);
    }

    static jint StartWifiDisplayReceive_native(JNIEnv *env, jobject clazz,int displayno,int mode)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_STARTWIFIDISPLAYRECEIVE,displayno,mode,0);
    }

    static jint EndWifiDisplayReceive_native(JNIEnv *env, jobject clazz,int displayno)
    {
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_ENDWIFIDISPLAYRECEIVE,displayno,0,0);
    }

    static jint UpdateSendClient_native(JNIEnv *env, jobject clazz,int mode)
    {
   		return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_UPDATESENDCLIENT,mode,0,0);
    }

    static jint Set3DLayerOffset_native(JNIEnv *env, jobject clazz, int displayno, int left, int right){
        return  SurfaceComposerClient::setDisplayProp(DISPLAY_CMD_SET3DLAYEROFFSET,displayno,left,right);
    }


    static JNINativeMethod method_table[] = {
        { "nativeInit", "()V", (void*)init_native },
        { "nativeSetDisplayOutputType", "(III)I", (void*)changeMode_native },
        { "nativeSetDisplayParameter", "(III)I", (void*)setDisplayParameter_native },
        { "nativeSetDisplayMode", "(I)I", (void*)setDisplayMode_native },
        { "nativeOpenDisplay", "(I)I", (void*)openDisplay_native },
        { "nativeCloseDisplay", "(I)I", (void*)closeDisplay_native },
        { "nativeGetHdmiHotPlug", "()I", (void*)getHdmiStatus_native },
        { "nativeGetTvDacHotPlug", "()I", (void*)getTvDacStatus_native },
        { "nativeGetDisplayMode", "()I", (void*)getDisplayMode_native },
        { "nativeGetDisplayCount", "()I", (void*)getDisplayCount_native },
        { "nativeSetDisplayMaster", "(I)I", (void*)setDisplayMaster_native },
        { "nativeGetDisplayMaster", "()I", (void*)getDisplayMaster_native },
        { "nativeGetMaxWidthDisplay", "()I", (void*)getMaxWidthDisplay_native },
        { "nativeGetMaxHdmiMode", "()I", (void*)getMaxHdmiMode_native },
        { "nativeGetDisplayOutputType", "(I)I", (void*)getDisplayOutputType_native },
        { "nativeGetDisplayOutputFormat", "(I)I", (void*)getDisplayOutputFormat_native},
        { "nativeGetDisplayWidth", "(I)I", (void*)getDisplayOutputWidth_native },
        { "nativeGetDisplayHeight", "(I)I", (void*)getDisplayOutputHeight_native },
        { "nativeGetDisplayPixelFormat", "(I)I", (void*)getDisplayOutputPixelFormat_native },
        { "nativeGetDisplayOpen", "(I)I", (void*)getDisplayOutputOpen_native },
        { "nativeGetDisplayHotPlug", "(I)I", (void*)getDisplayOutputHotPlug_native },
        { "nativeSetDisplayBacklihgtMode", "(I)I", (void*)SetDisplayBacklihgtMode_native },
        { "nativeSetDisplayAreaPercent", "(II)I",(void *)setDisplayAreaPercent_native},
        { "nativeGetDisplayAreaPercent", "(I)I",(void *)getDisplayAreaPercent_native},
        { "nativeIsSupportHdmiMode", "(I)I",(void *)isSupportHdmiMode_native},
        { "nativeIsSupport3DMode", "()I",(void *)isSupport3DMode_native},
        { "nativeSetDisplayBright", "(II)I",(void *)setDisplayBright_native},
        { "nativeGetDisplayBright", "(I)I",(void *)getDisplayBright_native},
        { "nativeSetDisplayContrast", "(II)I",(void *)setDisplayContrast_native},
        { "nativeGetDisplayContrast", "(I)I",(void *)getDisplayContrast_native},
        { "nativeSetDisplaySaturation", "(II)I",(void *)setDisplaySaturation_native},
        { "nativeGetDisplaySaturation", "(I)I",(void *)getDisplaySaturation_native},
        { "nativeGetDisplayHue", "(I)I",(void *)getDisplayHue_native},
        { "nativeGetDisplayHue", "(I)I",(void *)getDisplayHue_native},
        { "nativeStartWifiDisplaySend", "(II)I", (void*)StartWifiDisplaySend_native },
        { "nativeEndWifiDisplaySend", "(I)I", (void*)EndWifiDisplaySend_native },
        { "nativeStartWifiDisplayReceive", "(II)I", (void*)StartWifiDisplayReceive_native },
        { "nativeEndWifiDisplayReceive", "(I)I", (void*)EndWifiDisplayReceive_native },
        { "nativeUpdateSendClient", "(I)I", (void*)UpdateSendClient_native },
        { "nativeSet3DLayerOffset", "(III)I", (void*)Set3DLayerOffset_native }
    };

    int register_android_server_DisplayManagerServiceAw(JNIEnv *env)
    {
    	jclass clazz = env->FindClass("com/android/server/DisplayManagerServiceAw");

        if (clazz == NULL) 
        {
            ALOGE("Can't find com/android/server/DisplayManagerServiceAw");
            
            return -1;
        }
            
        return jniRegisterNativeMethods(env, "com/android/server/DisplayManagerServiceAw",
                method_table, NELEM(method_table));
    }

};
