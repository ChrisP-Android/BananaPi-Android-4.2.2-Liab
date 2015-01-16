LOCAL_PATH := $(call my-dir)

ifneq ($(BOARD_HAVE_BLUETOOTH_BCM),)

include $(CLEAR_VARS)

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), ap6210)
    LOCAL_CFLAGS += -DUSE_AP6210_BT_MODULE    
endif

BDROID_DIR := $(TOP_DIR)external/bluetooth/bluedroid

LOCAL_SRC_FILES := \
        src/bt_vendor_brcm.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/upio.c \
        src/conf.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include

LOCAL_SHARED_LIBRARIES := \
        libcutils

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := broadcom
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_PRODUCT), full_maguro)
    include $(LOCAL_PATH)/conf/samsung/maguro/Android.mk
endif
ifeq ($(TARGET_PRODUCT), full_crespo)
    include $(LOCAL_PATH)/conf/samsung/crespo/Android.mk
endif
ifeq ($(TARGET_PRODUCT), full_crespo4g)
    include $(LOCAL_PATH)/conf/samsung/crespo4g/Android.mk
endif
ifeq ($(TARGET_PRODUCT), full_wingray)
    include $(LOCAL_PATH)/conf/moto/wingray/Android.mk
endif

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), bcm40183)
    include $(LOCAL_PATH)/conf/bcm40183/Android.mk
endif

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), ap6210)    
    include $(LOCAL_PATH)/conf/ap6210/Android.mk
endif

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), ap6330)
    include $(LOCAL_PATH)/conf/ap6330/Android.mk
endif

endif # BOARD_HAVE_BLUETOOTH_BCM
