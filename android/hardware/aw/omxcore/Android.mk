LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../Config.mk

LOCAL_CFLAGS += $(AW_OMX_EXT_CFLAGS)
#LOCAL_CFLAGS += -DLOG_NDEBUG=0
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= src/omx_core_cmp.cpp src/aw_omx_core.c src/aw_registry_table.c

LOCAL_C_INCLUDES := \
	$(TOP)/hardware/aw/omxcore/inc/ \
	$(TOP)/hardware/aw

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libdl

LOCAL_MODULE:= libOmxCore

include $(BUILD_SHARED_LIBRARY)
