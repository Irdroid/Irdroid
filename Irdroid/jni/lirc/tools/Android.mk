LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := lirc

LOCAL_MODULE    := irsend
LOCAL_SRC_FILES := irsend.c
LOCAL_CFLAGS := -DHAVE_CONFIG_H

include $(BUILD_STATIC_LIBRARY)
