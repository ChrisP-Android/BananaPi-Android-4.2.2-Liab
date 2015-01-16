LOCAL_PATH:= $(call my-dir)

# the library
# ============================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
            $(call all-subdir-java-files) \
		android/net/ethernet/IEthernetManager.aidl

LOCAL_MODULE:= ethernet
LOCAL_MODULE_TAGS := optional
LOCAL_JAVACFLAGS := $(local_javac_flags)

LOCAL_NO_STANDARD_LIBRARIES := true

LOCAL_JAVA_LIBRARIES := bouncycastle core core-junit ext framework
#
# List of classes and interfaces which should be loaded by the Zygote.
#LOCAL_JAVA_RESOURCE_FILES += $(LOCAL_PATH)/preloaded-classes

LOCAL_NO_EMMA_INSTRUMENT := true
LOCAL_NO_EMMA_COMPILE := true

LOCAL_DX_FLAGS := --core-library

include $(BUILD_JAVA_LIBRARY)


