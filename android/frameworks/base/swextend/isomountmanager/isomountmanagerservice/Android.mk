LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main_isomountmanagerservice.cpp 

LOCAL_SHARED_LIBRARIES := \
	libisomountmanagerservice \
	libutils \
	libbinder

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../libisomountmanagerservice

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= isomountmanagerservice

include $(BUILD_EXECUTABLE)
