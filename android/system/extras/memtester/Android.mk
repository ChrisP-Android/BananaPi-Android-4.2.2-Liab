# Copyright 2012 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= memtester.c tests.c

LOCAL_CFLAGS += -std=c99

LOCAL_MODULE:= memtester

LOCAL_MODULE_TAGS:= eng


include $(BUILD_EXECUTABLE)
