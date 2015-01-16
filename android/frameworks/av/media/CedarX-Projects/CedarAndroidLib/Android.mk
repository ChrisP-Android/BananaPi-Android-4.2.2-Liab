LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/../Config.mk

ifneq ($(CEDARX_DEBUG_CEDARV),Y)
ifeq ($(CEDARX_ANDROID_VERSION),4)
include $(LOCAL_PATH)/LIB_F23/Android.mk
else
include $(LOCAL_PATH)/LIB_$(CEDARX_ANDROID_CODE)_$(CEDARX_CHIP_VERSION)/Android.mk
endif
endif
