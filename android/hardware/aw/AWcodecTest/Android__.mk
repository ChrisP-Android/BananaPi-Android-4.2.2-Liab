LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Config.mk

LOCAL_CFLAGS += $(AW_OMX_EXT_CFLAGS)
LOCAL_CFLAGS += -D__OS_ANDROID
LOCAL_CFLAGS += -D__CHIP_VERSION_F23
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= AWcodecTest.cpp \
			../CameraSource/CameraHardware.cpp \

LOCAL_C_INCLUDES := \
	$(TOP)/hardware/aw \
	$(TOP)/hardware/aw/include \
	$(TOP)/hardware/aw/CameraSource \
	$(CEDARX_TOP)/include	\
	$(CEDARX_TOP)/include/include_cedarv \
	$(CEDARX_TOP)/include/include_vencoder \

#	$(TOP)/hardware/aw/vdecoder/libcedarv/ \
#	$(TOP)/hardware/aw/vdecoder/libvecore/ \

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
  	libui       \
	libcedarxbase \
	libaw_h264enc \
	libcedarxosal \
	libcedarv


#libvdecoder
LOCAL_MODULE:= AWcodecTest

include $(BUILD_EXECUTABLE)
