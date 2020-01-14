LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := libomxr_cnvosdep libvspmif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

LOCAL_SRC_FILES := \
    ../cmn/cnvp_cmn.c \
    cnvp_fdp_core.c \
    cnvp_fdp_interface.c

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
    $(OMX_TOP)/omxr_converter/include \
    $(OMX_TOP)/omxr_converter/plugin/cmn \
    $(OMX_TOP)/omxr_converter/osal \
	$(OMX_TOP)/include \
	$(TOP)/hardware/renesas/modules/vspm/vspm-module/files/vspm/include
	
LOCAL_CFLAGS := -fno-strict-aliasing -fstack-protector-strong -fomit-frame-pointer -ffunction-sections -fdata-sections -funwind-tables -no-canonical-prefixes -Winit-self -Wpointer-arith -Werror=return-type -Werror=address -Werror=sequence-point -Werror=date-time -fsanitize-trap=all -ftrap-function=abort -Wno-error=deprecated-declarations -Wall -DOMXR_BUILD_OS_LINUX -DCNV_BUILD_FDPM_ENABLE
LOCAL_SANITIZE := unsigned-integer-overflow signed-integer-overflow cfi
LOCAL_SANITIZE_DIAG := cfi

LOCAL_MODULE:= libomxr_cnvpfdp
LOCAL_MULTILIB := 64
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
