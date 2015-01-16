LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Config.mk

LOCAL_CFLAGS += $(AW_OMX_EXT_CFLAGS)
LOCAL_CFLAGS += -D__OS_ANDROID
LOCAL_CFLAGS += -D__CHIP_VERSION_F23
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= omx_venc.cpp

LOCAL_C_INCLUDES := \
	$(TOP)/hardware/aw/omxcore/inc/ \
	$(TOP)/hardware/aw/venc \
	$(TOP)/hardware/aw/vencode/h264encLib/include \
	$(TOP)/hardware/aw/include \
	$(TOP)/hardware/aw \
	$(CEDARX_TOP)/include/include_vencoder \
	$(CEDARX_TOP)/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	liblog \
	libdl \
	libbinder \
	libcedarxosal \
	libaw_h264enc \
	libcedarxbase \
				

LOCAL_MODULE:= libOmxVenc

include $(BUILD_SHARED_LIBRARY)
