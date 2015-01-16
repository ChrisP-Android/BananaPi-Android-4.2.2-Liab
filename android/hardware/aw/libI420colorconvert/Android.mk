
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    ColorConvert.cpp

LOCAL_C_INCLUDES:= \
        $(TOP)/hardware/aw/omxcore/inc \
        $(TOP)/frameworks/native/include/media/editor
        

LOCAL_SHARED_LIBRARIES :=\
    libutils \

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libI420colorconvert

include $(BUILD_SHARED_LIBRARY)

