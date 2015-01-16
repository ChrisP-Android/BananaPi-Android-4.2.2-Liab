LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    onload.cpp \
    md5.cpp \
    softwinner_os_RecoverySystemEx.cpp

LOCAL_SHARED_LIBRARIES := \
    libc \
    libandroid_runtime \
    libnativehelper \
    libutils \
    libbinder \
    libui \
    libcutils

LOCAL_C_INCLUDES += \
    frameworks/base/core/jni \
    $(JNI_H_INCLUDE)

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libjni_swos

include $(BUILD_SHARED_LIBRARY)
