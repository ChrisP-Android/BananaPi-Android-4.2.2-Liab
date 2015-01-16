
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../Config.mk

LOCAL_SRC_FILES := \
    AwOMXPlugin.cpp                      \

LOCAL_CFLAGS := $(PV_CFLAGS_MINUS_VISIBILITY)
LOCAL_CFLAGS += $(AW_OMX_EXT_CFLAGS)
LOCAL_MODULE_TAGS := optional


LOCAL_C_INCLUDES :=
ifeq ($(AW_OMX_PLATFORM_VERSION),6)
LOCAL_C_INCLUDES += \
        $(TOP)/frameworks/base/include/media/stagefright \
        $(TOP)/frameworks/base/include/media/stagefright/openmax
else
LOCAL_C_INCLUDES += \
        $(TOP)/frameworks/native/include/media/hardware \
        $(TOP)/frameworks/native/include/media/openmax
endif

LOCAL_C_INCLUDES += \
        $(TOP)/hardware/aw

LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libutils                \
        libcutils               \
        libdl                   \
        libui                   \

LOCAL_MODULE := libstagefrighthw

include $(BUILD_SHARED_LIBRARY)
