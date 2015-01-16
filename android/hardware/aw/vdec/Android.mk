LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Config.mk

LOCAL_CFLAGS += $(AW_OMX_EXT_CFLAGS)
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= omx_vdec.cpp transform_color_format.c

LOCAL_C_INCLUDES := \
	$(TOP)/hardware/aw/omxcore/inc/ \
	$(TOP)/hardware/aw/vdec/ \
	$(TOP)/frameworks/native/include/     \
	$(CEDARX_TOP)/include     \
	$(CEDARX_TOP)/include/include_cedarv     \
	$(TOP)/hardware/aw \
	$(TOP)/hardware/aw/include \

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libcedarxosal \
	libcedarv \
	libui       \
	libdl \
	libcedarxbase \


#libvdecoder
LOCAL_MODULE:= libOmxVdec

include $(BUILD_SHARED_LIBRARY)
