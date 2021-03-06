LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    SprdOMXPlugin.cpp \
	SprdOMXComponent.cpp \
	SprdSimpleOMXComponent.cpp

LOCAL_CFLAGS := $(PV_CFLAGS_MINUS_VISIBILITY)

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/native/include/media/hardware \
	$(TOP)/hardware/sprd/libstagefrighthw/include	\
	$(TOP)/hardware/sprd/libstagefrighthw/include/openmax	\
	$(TOP)/hardware/sprd/libmemion

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)

LOCAL_SHARED_LIBRARIES :=       \
        libmemion        \
        libutils                \
        libcutils               \
        libui                   \
        libdl			\
	libstagefright_foundation
LOCAL_MODULE := libstagefrighthw

LOCAL_CFLAGS:= -DLOG_TAG=\"$(TARGET_BOARD_PLATFORM).libstagefright\"

include $(BUILD_SHARED_LIBRARY)
