LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include frameworks/${AV_BASE_PATH}/media/libstagefright/codecs/common/Config.mk
include frameworks/${AV_BASE_PATH}/media/CedarX-Projects/Config.mk

LOCAL_SRC_FILES:=                         \
		CedarXAudioPlayer.cpp			  \
        CedarXPlayer.cpp				  \
        CedarXMetadataRetriever.cpp		  \
        CedarXMetaData.cpp				  \
        CedarXRecorder.cpp


LOCAL_C_INCLUDES:= \
	$(JNI_H_INCLUDE) \
	$(TOP)/frameworks/${AV_BASE_PATH}/include/media/stagefright \
    $(TOP)/frameworks/${AV_BASE_PATH}/include/media/stagefright/openmax \
    $(CEDARX_TOP)/include \
    $(CEDARX_TOP)/libutil \
    $(CEDARX_TOP)/include/include_cedarv \
    $(CEDARX_TOP)/include/include_audio \
    ${CEDARX_TOP}/include/include_camera \
    $(TOP)/frameworks/${AV_BASE_PATH}/media/libstagefright/include \
    $(TOP)/frameworks/base \
    $(TOP)/frameworks/${AV_BASE_PATH}/include \
    $(TOP)/external/openssl/include

LOCAL_SHARED_LIBRARIES := \
        libbinder         \
        libmedia          \
        libutils          \
        libcutils         \
        libui             \
        libsurfaceflinger_client \
        libcamera_client \
        libstagefright_foundation \
        libstagefright \
        libicuuc \
		libsurfaceflinger_client \
		libskiagl \
		libskia 
        
	
ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_STATIC_LIBRARIES += \
	libcedarxplayer \
	libcedarxcomponents \
	libcedarxdemuxers \
	libcedarxsftdemux \
	libcedarxstream \
	libthirdpartstream \
	libcedarxrender \
	libdemux_cedarm \
	libsub \
	libsub_inline \
	libh264enc \
	libmp4_muxer \
	libjpgenc \
	libcedarx_rtsp	\
	libuserdemux
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarxplayer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarxcomponents.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarxdemuxers.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarxsftdemux.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarxstream.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libthirdpartstream.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarxrender.a	\
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_cedarm.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libsub.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libsub_inline.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libiconv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libh264enc.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libmp4_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libjpgenc.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarx_rtsp.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libuserdemux.a
endif

ifeq ($(CEDARX_DEBUG_DEMUXER),Y)
LOCAL_STATIC_LIBRARIES += \
	libdemux_asf \
	libdemux_avi \
	libdemux_flv \
	libdemux_mkv \
	libdemux_ogg \
	libdemux_mov \
	libdemux_mpg \
	libdemux_rmvb \
	libdemux_ts \
	libdemux_pmp \
	libdemux_idxsub
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_asf.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_avi.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_flv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_mkv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_ogg.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_mov.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_mpg.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_rmvb.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_ts.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_pmp.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libdemux_idxsub.a
endif
	
ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_SHARED_LIBRARIES += libcedarxbase libswdrm
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarxbase.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libswdrm.so
endif

ifeq ($(CEDARX_DEBUG_CEDARV),Y)
LOCAL_SHARED_LIBRARIES += libcedarv libcedarxosal 
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarv.so \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_$(CEDARX_CHIP_VERSION)/libcedarxosal.so
endif

LOCAL_LDFLAGS += \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libwma.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libaac.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libmp3.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libatrc.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libcook.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libsipr.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libamr.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libape.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libogg.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libflac.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libwav.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libra.a \
       $(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libaacenc.a

ifneq ($(CEDARX_PRODUCTOR),TVD_001)
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libac3_hw.a \
	$(CEDARX_TOP)/../CedarAndroidLib/$(CEDARX_PREBUILD_LIB_PATH)/libdts_hw.a
endif

ifeq ($(CEDARX_ENABLE_MEMWATCH),Y)
LOCAL_STATIC_LIBRARIES += libmemwatch
endif

#LOCAL_SHARED_LIBRARIES += libstagefright_foundation libstagefright

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

include $(call all-makefiles-under,$(LOCAL_PATH))
