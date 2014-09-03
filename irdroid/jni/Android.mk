
include $(call all-subdir-makefiles)

include $(CLEAR_VARS)
LOCAL_PATH := /home/zokama/Prog/Workspace/AndroLirc/jni
LOCAL_C_INCLUDES := lirc

LOCAL_MODULE    := androlirc
LOCAL_SRC_FILES := androlirc.c
LOCAL_STATIC_LIBRARIES := lircd portaudio irsend
LOCAL_LDLIBS := -llog
LOCAL_CFLAGS := -DHAVE_CONFIG_H

include $(BUILD_SHARED_LIBRARY)
