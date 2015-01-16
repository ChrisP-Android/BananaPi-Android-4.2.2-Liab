
#LOCAL_PATH_T := $(call my-dir)
#include $(call all-subdir-makefiles)
#LOCAL_PATH := $(LOCAL_PATH_T)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := lib/libmagcalibrate.a
LOCAL_MODULE_TAGS := eng        
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := magd.cpp util.cpp
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_STATIC_LIBRARIES += libmagcalibrate
LOCAL_SHARED_LIBRARIES := liblog libcutils libdl
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE := magd
include $(BUILD_EXECUTABLE)