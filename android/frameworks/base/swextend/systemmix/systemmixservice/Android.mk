LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main_systemmixservice.cpp 

LOCAL_SHARED_LIBRARIES := \
	libsystemmixservice \
	libutils \
	libbinder

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../libsystemmix

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= systemmixservice

include $(BUILD_EXECUTABLE)