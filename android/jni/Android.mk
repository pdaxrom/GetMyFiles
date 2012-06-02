# Copyright (C) 2010 The Android Open Source Project
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
LOCAL_PATH	:= $(call my-dir)
VERSION		:= $(shell ../getversion.sh ../)

include $(CLEAR_VARS)

LOCAL_MODULE    := getmyfiles
#LOCAL_MODULE    := libgetmyfiles.so
LOCAL_SRC_FILES := \
    ../../client.c \
    ../../urldecode.c \
    ../../http.c \
    ../../utils.c \
    ../../httpd.c \
    ../../getaddr.c \
    ../../connctrl.c \
    ../../lib/tcp.c \
    ../../lib/udp.c \
    ../../lib/aes.c \

LOCAL_CFLAGS	:= -DVERSION=$(VERSION)
LOCAL_C_INCLUDES := external/openssl/include ../ ../lib
#LOCAL_LDLIBS    := -llog
LOCAL_STATIC_LIBRARIES := libssl-static libcrypto-static

include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/../external/openssl/Android.mk
