include $(call all-subdir-makefiles)
LOCAL_PATH := $(call my-dir)
LOCAL_C_INCLUDES := lirc
include $(CLEAR_VARS)

LOCAL_MODULE    := androlirc
LOCAL_SRC_FILES := androlirc.c
LOCAL_CFLAGS := -DHAVE_CONFIG_H
LOCAL_STATIC_LIBRARIES := lircd

include $(BUILD_SHARED_LIBRARY)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := androlirc
LOCAL_SRC_FILES := androlirc.c
LOCAL_C_INCLUDES := lirc
LOCAL_CFLAGS := -DHAVE_CONFIG_H
LOCAL_STATIC_LIBRARIES := lircd

include $(BUILD_SHARED_LIBRARY)



LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := androlirc
LOCAL_SRC_FILES := androlirc.c
LOCAL_STATIC_LIBRARIES := lircd
LOCAL_C_INCLUDES := lirc lirc/daemons
LOCAL_CFLAGS := -DHAVE_CONFIG_H

include $(BUILD_SHARED_LIBRARY)
include $(call all-subdir-makefiles)
