LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../Config.mk

LOCAL_PREBUILT_LIBS := libcedarxosal.so libcedarv.so libcedarv_base.so libcedarv_adapter.so libve.so libthirdpartstream.so libcedarxsftstream.so
LOCAL_PREBUILT_LIBS += libfacedetection.so libsunxi_alloc.so libjpgenc.so libaw_h264enc.so
ifeq ($(CEDARX_DEBUG_ENABLE),N)
LOCAL_PREBUILT_LIBS += libcedarxbase.so libaw_audio.so libaw_audioa.so libswdrm.so libstagefright_soft_cedar_h264dec.so
#LOCAL_PREBUILT_LIBS += libswa1.so libswa2.so
endif
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
