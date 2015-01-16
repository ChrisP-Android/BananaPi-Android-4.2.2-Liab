LOCAL_PATH:= $(call my-dir)

# the library
# ============================================================
include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := bcprov:libs/bcprov.jar \
				polo:libs/polo.jar \
				protobuf:libs/protobuf.jar \
				anymote:libs/anymote.jar 

include $(BUILD_MULTI_PREBUILT) 

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_SRC_FILES := $(call all-subdir-java-files) 

LOCAL_STATIC_JAVA_LIBRARIES := bcprov polo protobuf anymote

LOCAL_JNI_SHARED_LIBRARIES := libtvremote

LOCAL_REQUIRED_MODULES := libtvremote

LOCAL_PACKAGE_NAME := tvremoteserver
LOCAL_CERTIFICATE := platform
include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
