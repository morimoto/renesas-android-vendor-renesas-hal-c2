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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := media_codecs_performance_renesas
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_STEM := media_codecs_performance.xml
LOCAL_PREBUILT_MODULE_FILE := \
    device/renesas/$(TARGET_PRODUCT)/media/media_codecs_performance_$(TARGET_BOARD_PLATFORM).xml
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := media_codecs_renesas_vp8
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_STEM := $(LOCAL_MODULE).xml
LOCAL_SRC_FILES := \
    $(if $(filter true,$(RCAR_ENABLE_VIDEO_VP8)),$(LOCAL_MODULE_STEM),media_codecs_renesas_empty.xml)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := media_codecs_renesas_vp9
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_STEM := $(LOCAL_MODULE).xml
LOCAL_SRC_FILES := \
    $(if $(filter true,$(RCAR_ENABLE_VIDEO_VP9)),$(LOCAL_MODULE_STEM),media_codecs_renesas_empty.xml)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := media_codecs_renesas
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_STEM := media_codecs.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
include $(BUILD_PREBUILT)
