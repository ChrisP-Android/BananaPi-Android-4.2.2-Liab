LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    android_googleremote_VmouseController.cpp \
    onload.cpp

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libcutils \
	libnativehelper \
    libutils \

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libtvremote

include $(BUILD_SHARED_LIBRARY)
