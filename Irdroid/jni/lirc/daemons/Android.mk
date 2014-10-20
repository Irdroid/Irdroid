LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := lirc

LOCAL_MODULE    := lircd
LOCAL_SRC_FILES := lircd.c\
		hw-types.c\
		input_map.c\
		ir_remote.c\
		config_file.c\
		release.c\
		transmit.c\
		receive.c\
		hw_audio.c
LOCAL_CFLAGS := -DHAVE_CONFIG_H
LOCAL_STATIC_LIBRARIES := portaudio

include $(BUILD_STATIC_LIBRARY)
