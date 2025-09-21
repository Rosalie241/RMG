LOCAL_PATH := $(call my-dir)

###########################
#
# SDL_net shared library
#
###########################

include $(CLEAR_VARS)

LOCAL_MODULE := SDL3_net

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,,) \
	$(wildcard $(LOCAL_PATH)/src/*.c) \

LOCAL_CFLAGS =

# Warnings we haven't fixed (yet)
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-sign-compare

LOCAL_LDLIBS :=

LOCAL_LDFLAGS := -Wl,--no-undefined -Wl,--no-undefined-version -Wl,--version-script=$(LOCAL_PATH)/src/SDL_net.sym

ifeq ($(NDK_DEBUG),1)
    cmd-strip :=
endif

LOCAL_SHARED_LIBRARIES := SDL3

include $(BUILD_SHARED_LIBRARY)

###########################
#
# SDL_net static library
#
###########################

LOCAL_MODULE := SDL3_net_static

LOCAL_MODULE_FILENAME := libSDL3_net

LOCAL_LDLIBS :=

LOCAL_LDFLAGS :=

LOCAL_EXPORT_LDLIBS :=

LOCAL_STATIC_LIBRARIES := SDL3
LOCAL_SHARED_LIBRARIES :=

include $(BUILD_STATIC_LIBRARY)
