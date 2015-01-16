LOCAL_PATH:= $(call my-dir)

###########################################

include $(CLEAR_VARS)
LOCAL_MODULE := librotation

LOCAL_MODULE_TAGS := optional 

LOCAL_SRC_FILES := rotation.cpp 

LOCAL_SHARED_LIBRARIES := libutils libbinder

include $(BUILD_STATIC_LIBRARY)
###########################################

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    ActivityManager.cpp         \
    Crypto.cpp                  \
    HDCP.cpp                    \
    MediaPlayerFactory.cpp      \
    MediaPlayerService.cpp      \
    MediaRecorderClient.cpp     \
    MetadataRetrieverClient.cpp \
    MidiFile.cpp                \
    MidiMetadataRetriever.cpp   \
    RemoteDisplay.cpp           \
    CedarPlayer.cpp       		\
    StagefrightPlayer.cpp       \
    StagefrightRecorder.cpp     \
    TestPlayerStub.cpp          \
    CedarAPlayerWrapper.cpp		\
    SimpleMediaFormatProbe.cpp	\
    MovAvInfoDetect.cpp

LOCAL_SHARED_LIBRARIES :=       \
    libbinder                   \
    libcamera_client            \
    libcutils                   \
    libdl                       \
    libgui                      \
    libmedia                    \
    libmedia_native             \
    libsonivox                  \
    libCedarX           	    \
	libCedarA           	    \
    libstagefright              \
    libstagefright_foundation   \
    libstagefright_omx          \
    libstagefright_wfd          \
    libutils                    \
    libvorbisidec               \

LOCAL_STATIC_LIBRARIES :=       \
    libstagefright_nuplayer     \
    libstagefright_rtsp         \
    librotation

LOCAL_C_INCLUDES :=                                                 \
    $(call include-path-for, graphics corecg)                       \
    $(TOP)/frameworks/av/media/CedarX-Projects/CedarXAndroid/IceCreamSandwich \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_audio \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_cedarv \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarA \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarA/include \
    $(TOP)/frameworks/av/media/libstagefright/include               \
    $(TOP)/frameworks/av/media/libstagefright/rtsp                  \
    $(TOP)/frameworks/av/media/libstagefright/wifi-display          \
    $(TOP)/frameworks/native/include/media/openmax                  \
    $(TOP)/external/tremolo/Tremolo                                 \

LOCAL_CFLAGS +=-DCEDARX_ANDROID_VERSION=9

LOCAL_MODULE:= libmediaplayerservice

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
