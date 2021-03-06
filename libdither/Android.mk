#
# Copyright (C) 2010 ARM Limited. All rights reserved.
#
# Copyright (C) 2016 The Android Open Source Project
#
# Copyright (C) 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= libdither

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS = \
	-g \
	-mfpu=neon \
	-O3 \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \

LOCAL_EXPORT_C_INCLUDE_DIRS := \
	$(LOCAL_PATH) \
	$(LOCAL_C_INCLUDES) \

LOCAL_SRC_FILES := \
	image_dither.cpp \

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libcutils \
	libbinder \
	libdl \
	liblog \

include $(BUILD_SHARED_LIBRARY)
