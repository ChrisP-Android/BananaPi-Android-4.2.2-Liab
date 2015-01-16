ifeq ($(BOARD_HAVE_BLUETOOTH), true)
LOCAL_PATH := $(call my-dir)

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), rtl8723as)
	include $(LOCAL_PATH)/rtl8723as/Android.mk	
endif	

ifeq ($(SW_BOARD_HAVE_BLUETOOTH_NAME), rtl8723au)
	include $(LOCAL_PATH)/rtl8723au/Android.mk
endif
endif

