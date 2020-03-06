LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE:= libomxr_utility
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_ARM_MODE := arm
LOCAL_MULTILIB := 64

LOCAL_SRC_FILES := \
    config/omxr_utility_config.c \
    hw_dep/omxr_debug_func.c \
    hw_dep/omxr_dll_loader.c \
    hw_dep/omxr_file_loader.c \
    os_dep/omxr_mem_osdep_func.c \
    os_dep/omxr_os_wrapper.c \
    os_dep/omxr_string_osdep_func.c \
    os_dep/omxr_workbuffer_func.c

LOCAL_C_INCLUDES := \
    $(TOP)/vendor/renesas/utils/mmngr/libmmngr/mmngr/include \
    $(TOP)/hardware/renesas/modules/mmngr/mmngr_drv/mmngr/mmngr-module/files/mmngr/include \
    $(OMX_TOP)/include \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/os_dep \
    $(LOCAL_PATH)/hw_dep

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    libmmngr

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
LOCAL_LDLIBS += -llog
endif
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_LDLIBS += -llog
endif

# XXX: Disable MMNGR build until we'll get MMNGR for Gen3.
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
    -DOMXR_BUILD_OS_LINUX \
    -DOMXR_BUILD_LINUX_NON_ROOT \
    -DOMXR_DEFAULT_CONFIG_FILE_NAME=\"/vendor/etc/omxr_config_base.txt\" \
    -DOMXR_WORKBUFFER_FUNC_DISABLE_USING_VA_SUPPORT

LOCAL_SANITIZE := \
    unsigned-integer-overflow \
    signed-integer-overflow \
    cfi

LOCAL_SANITIZE_DIAG := cfi
include $(BUILD_SHARED_LIBRARY)
