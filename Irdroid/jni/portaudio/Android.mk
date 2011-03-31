LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := portaudio/common portaudio/include portaudio/os/unix

LOCAL_MODULE    := portaudio
LOCAL_SRC_FILES := common/pa_allocation.c\
		common/pa_cpuload.c\
		common/pa_dither.c\
		common/pa_process.c\
		common/pa_skeleton.c\
		common/pa_trace.c\
		common/pa_converters.c\
		common/pa_debugprint.c\
		common/pa_front.c\
		common/pa_ringbuffer.c\
		common/pa_stream.c\
		os/unix/pa_unix_util.c\
		os/unix/pa_unix_hostapis.c\
		hostapi/oss/pa_unix_oss.c\
		hostapi/oss/recplay.c
		#hostapi/alsa/pa_linux_alsa.c

LOCAL_CFLAGS := -DHAVE_LINUX_SOUNDCARD_H\
		-DPA_LOG_API_CALLS\
		-DPA_ENABLE_DEBUG_OUTPUT\
		-DPA_USE_OSS

include $(BUILD_STATIC_LIBRARY)

