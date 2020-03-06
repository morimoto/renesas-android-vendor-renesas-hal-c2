LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE:= libomxr_cnvosdep
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_ARM_MODE := arm
LOCAL_MULTILIB := 64

LOCAL_SRC_FILES := cnv_osdep.c

LOCAL_C_INCLUDES := \
    $(OMX_TOP)/omxr_converter/include \
    $(OMX_TOP)/include

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    libomxr_utility

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

LOCAL_CFLAGS := \
    -fno-strict-aliasing \
    -fstack-protector-strong \
    -fomit-frame-pointer \
    -ffunction-sections \
    -fdata-sections \
    -funwind-tables \
    -no-canonical-prefixes \
    -Winit-self \
    -Wpointer-arith \
    -Werror=return-type \
    -Werror=address \
    -Werror=sequence-point \
    -Werror=date-time \
    -fsanitize-trap=all \
    -ftrap-function=abort \
    -Wno-error=deprecated-declarations \
    -Wall \
    -DOMXR_BUILD_OS_LINUX

LOCAL_SANITIZE := \
    signed-integer-overflow \
    unsigned-integer-overflow \
    cfi

LOCAL_SANITIZE_DIAG := cfi
include $(BUILD_SHARED_LIBRARY)
