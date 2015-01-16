LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#include frameworks/${AV_BASE_PATH}/media/libstagefright/codecs/common/Config.mk
include $(LOCAL_PATH)/../../Config.mk

LOCAL_SRC_FILES:=                         \
		CedarXAudioPlayer.cpp			  \
        CedarXMetadataRetriever.cpp		  \
        CedarXMetaData.cpp				  \
        CedarXSoftwareRenderer.cpp		  \
        CedarXRecorder.cpp				  \
		CedarXPlayer.cpp				  \
		CedarXNativeRenderer.cpp		\
		virtual_hwcomposer.cpp


LOCAL_C_INCLUDES:= \
	$(JNI_H_INCLUDE) \
	$(TOP)/frameworks/${AV_BASE_PATH}/include/media/stagefright \
    $(CEDARX_TOP)/include \
    $(CEDARX_TOP)/libutil \
    $(CEDARX_TOP)/include/include_cedarv \
    $(CEDARX_TOP)/include/include_audio \
    ${CEDARX_TOP}/include/include_camera \
    ${CEDARX_TOP}/include/include_omx \
    $(TOP)/frameworks/${AV_BASE_PATH}/media/libstagefright/include \
    $(TOP)/frameworks/${AV_BASE_PATH} \
    $(TOP)/frameworks/${AV_BASE_PATH}/include \
    $(TOP)/external/openssl/include
    
ifeq ($(PLATFORM_VERSION),4.1.1)
    LOCAL_C_INCLUDES += $(TOP)/frameworks/native/include/media/hardware
endif
ifeq ($(PLATFORM_VERSION),4.2.1)
    LOCAL_C_INCLUDES += $(TOP)/frameworks/native/include/media/hardware
endif
ifeq ($(PLATFORM_VERSION),4.2.2)
	LOCAL_C_INCLUDES += $(TOP)/frameworks/native/include/media/hardware
endif


LOCAL_SHARED_LIBRARIES := \
        libcedarxosal	  \
        libbinder         \
        libmedia          \
        libutils          \
        libcutils         \
        libui             \
        libgui			  \
        libcamera_client \
        libstagefright_foundation \
        libicuuc \
		libskia 

ifeq ($(PLATFORM_VERSION),2.3.4)
   LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
endif
ifeq ($(PLATFORM_VERSION),4.0.1)
   LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
endif
ifeq ($(PLATFORM_VERSION),4.0.3)
   LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
endif
ifeq ($(PLATFORM_VERSION),4.0.4)
   LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
endif
#ifneq ($(PLATFORM_VERSION),4.1.1)
#LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
#endif
#ifneq ($(PLATFORM_VERSION),4.2.1)
#LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
#endif

ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_STATIC_LIBRARIES += \
	libcedarxplayer \
	libcedarxcomponents \
	libcedarxdemuxers \
	libcedarxsftdemux \
	libcedarxrender \
	libsub \
	libsub_inline \
	libmuxers \
	libmp4_muxer \
	libawts_muxer \
	libraw_muxer \
	libmpeg2ts_muxer \
	libjpgenc	\
	libuserdemux \
	libcedarxwvmdemux \
	libcedarxdrmsniffer

LOCAL_STATIC_LIBRARIES += \
	libcedarxstream \

LOCAL_STATIC_LIBRARIES += \
	libcedarx_rtsp
	
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxplayer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxcomponents.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxdemuxers.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxsftdemux.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxwvmdemux.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxrender.a	\
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libsub.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libsub_inline.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libiconv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libmuxers.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libmp4_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libawts_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libraw_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libmpeg2ts_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libjpgenc.a	\
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libuserdemux.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxdrmsniffer.a \

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxstream.a 

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarx_rtsp.a
endif

#audio postprocess lib
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libPostProcessOBJ.a

ifeq ($(CEDARX_DEBUG_DEMUXER),Y)
LOCAL_STATIC_LIBRARIES += \
	libdemux_asf \
	libdemux_avi \
	libdemux_flv \
	libdemux_mkv \
	libdemux_ogg \
	libdemux_mov \
	libdemux_mpg \
	libdemux_ts \
	libdemux_pmp \
	libdemux_idxsub \
	libdemux_awts
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_asf.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_avi.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_flv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_mkv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_ogg.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_mov.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_mpg.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_ts.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_pmp.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_idxsub.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libdemux_awts.a
endif
	
ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_SHARED_LIBRARIES += libcedarxbase libswdrm libthirdpartstream libcedarxsftstream
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxbase.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxosal.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libswdrm.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libthirdpartstream.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxsftstream.so
endif

ifeq ($(CEDARX_DEBUG_CEDARV),Y)
LOCAL_SHARED_LIBRARIES += libcedarv libcedarxosal libve libcedarv_base libcedarv_adapter libaw_h264enc
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarv.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarxosal.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libve.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarv_base.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libcedarv_adapter.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/libaw_h264enc.so
endif

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libaacenc.a


ifeq ($(CEDARX_ENABLE_MEMWATCH),Y)
LOCAL_STATIC_LIBRARIES += libmemwatch
endif

LOCAL_SHARED_LIBRARIES += libstagefright_foundation libstagefright libdrmframework

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
        LOCAL_LDLIBS += -lpthread -ldl
        LOCAL_SHARED_LIBRARIES += libdvm
        LOCAL_CPPFLAGS += -DANDROID_SIMULATOR
endif

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
        LOCAL_LDLIBS += -lpthread
endif

LOCAL_CFLAGS += -Wno-multichar 

ifeq ($(CEDARX_ANDROID_VERSION),3)
LOCAL_CFLAGS += -D__ANDROID_VERSION_2_3
else
LOCAL_CFLAGS += -D__ANDROID_VERSION_2_3_4
endif
LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libCedarX

include $(BUILD_SHARED_LIBRARY)

#include $(call all-makefiles-under,$(LOCAL_PATH))
