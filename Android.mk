#
# Copyright (C) 2019 GlobalLogic
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

# Include only for Renesas ones.
ifneq (,$(filter $(TARGET_PRODUCT),salvator ulcb kingfisher))

# Decide what codecs should be included
ifeq ($(TARGET_PRODUCT),salvator)
    # M3N
    ifeq ($(TARGET_BOARD_PLATFORM),r8a77965)
        RCAR_ENABLE_VIDEO_VP8D ?= true
        RCAR_ENABLE_VIDEO_VP9D ?= true
        RCAR_ENABLE_VIDEO_VP8E ?= true
    endif

    # H3
    ifeq ($(TARGET_BOARD_PLATFORM),r8a7795)
        RCAR_ENABLE_VIDEO_VP8D ?= true
        RCAR_ENABLE_VIDEO_VP8E ?= true
    endif

    # M3
    ifeq ($(TARGET_BOARD_PLATFORM),r8a7796)
        RCAR_ENABLE_VIDEO_VP8D ?= true
        RCAR_ENABLE_VIDEO_VP9D ?= true
        RCAR_ENABLE_VIDEO_VP8E ?= true
    endif
endif

RCAR_ENABLE_VIDEO_H263D  ?= true
RCAR_ENABLE_VIDEO_H264D  ?= true
RCAR_ENABLE_VIDEO_H265D  ?= true
RCAR_ENABLE_VIDEO_MPEG4D ?= true
RCAR_ENABLE_VIDEO_VP8D   ?= false
RCAR_ENABLE_VIDEO_VP9D   ?= false
RCAR_ENABLE_VIDEO_H264E  ?= true
RCAR_ENABLE_VIDEO_VP8E   ?= false

MY_SAVED_PATH := $(call my-dir)
OMX_TOP := $(MY_SAVED_PATH)/omx

include $(call all-subdir-makefiles)

LOCAL_PATH := $(MY_SAVED_PATH)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.media.c2@1.0-service.renesas
LOCAL_INIT_RC := $(LOCAL_MODULE).rc
LOCAL_VINTF_FRAGMENTS := $(LOCAL_MODULE).xml
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := \
    src/C2VendorBaseComponent.cpp \
    src/C2VendorDecComponent.cpp \
    src/C2VendorEncComponent.cpp \
    src/C2VendorComponentInferface.cpp \
    src/OMXR_Adapter.cpp \
    src/OMXR_Core.cpp \
    src/C2VendorComponentStore.cpp \
    src/service.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(OMX_TOP)/include \
    hardware/renesas/hal/hwcomposer \
    system/core/libion/kernel-headers \
    hardware/renesas/modules/vspm/vspm-module/files/vspm/include \
    $(TOP)/vendor/renesas/utils/mmngr/libmmngr/mmngr/include \
    $(TOP)/hardware/renesas/modules/mmngr/mmngr_drv/mmngr/mmngr-module/files/mmngr/include

LOCAL_SHARED_LIBRARIES := \
    android.hardware.media.c2@1.0 \
    libcodec2_hidl@1.0 \
    libcodec2_vndk \
    libutils \
    libbase \
    libhidlbase \
    libhardware \
    libvspmif \
    libmmngr

LOCAL_CFLAGS := \
    -Wall \
    -Wextra \
    -Werror

LOCAL_SANITIZE := \
    signed-integer-overflow \
    unsigned-integer-overflow \
    cfi

LOCAL_REQUIRED_MODULES := \
    omxr_prebuilts_common \
    media_codecs_performance_renesas \
    media_codecs_renesas \
    media_codecs_renesas_h263d \
    media_codecs_renesas_h264d \
    media_codecs_renesas_h265d \
    media_codecs_renesas_mpeg4d \
    media_codecs_renesas_vp8d \
    media_codecs_renesas_vp9d \
    media_codecs_renesas_h264e \
    media_codecs_renesas_vp8e \
    libomxr_utility \
    libomxr_uvcs_udf \
    libomxr_cnvpfdp \
    libomxr_cnvosdep \
    libomxr_videoconverter

ifeq ($(RCAR_ENABLE_VIDEO_H263D),true)
LOCAL_CFLAGS += -DHAL_C2_VENDOR_ENABLE_VIDEO_H263D
LOCAL_REQUIRED_MODULES += omxr_prebuilts_h263d
endif

ifeq ($(RCAR_ENABLE_VIDEO_H264D),true)
LOCAL_CFLAGS += -DHAL_C2_VENDOR_ENABLE_VIDEO_H264D
LOCAL_REQUIRED_MODULES += omxr_prebuilts_h264d
endif

ifeq ($(RCAR_ENABLE_VIDEO_H265D),true)
LOCAL_CFLAGS += -DHAL_C2_VENDOR_ENABLE_VIDEO_H265D
LOCAL_REQUIRED_MODULES += omxr_prebuilts_h265d
endif

ifeq ($(RCAR_ENABLE_VIDEO_MPEG4D),true)
LOCAL_CFLAGS += -DHAL_C2_VENDOR_ENABLE_VIDEO_MPEG4D
LOCAL_REQUIRED_MODULES += omxr_prebuilts_mpeg4d
endif

ifeq ($(RCAR_ENABLE_VIDEO_VP8D),true)
LOCAL_CFLAGS += -DHAL_C2_VENDOR_ENABLE_VIDEO_VP8D
LOCAL_REQUIRED_MODULES += omxr_prebuilts_vp8d
endif

ifeq ($(RCAR_ENABLE_VIDEO_VP9D),true)
LOCAL_CFLAGS += \
    -DHAL_C2_VENDOR_ENABLE_VIDEO_VP9D \
    -DHAL_C2_VENDOR_ENABLE_SUPPORTING_VP9_REFERENCE_SCALING
LOCAL_REQUIRED_MODULES += omxr_prebuilts_vp9d
endif

ifeq ($(RCAR_ENABLE_VIDEO_H264E),true)
LOCAL_CFLAGS += -DHAL_C2_VENDOR_ENABLE_VIDEO_H264E
LOCAL_REQUIRED_MODULES += omxr_prebuilts_h264e
endif

ifeq ($(RCAR_ENABLE_VIDEO_VP8E),true)
LOCAL_CFLAGS += -DHAL_C2_VENDOR_ENABLE_VIDEO_VP8E
LOCAL_REQUIRED_MODULES += omxr_prebuilts_vp8e
endif

include $(BUILD_EXECUTABLE)

endif # $(TARGET_PRODUCT) salvator ulcb kingfisher
