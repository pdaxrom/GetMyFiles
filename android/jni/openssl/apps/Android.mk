# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    ec.c \
    req.c \
    smime.c \
    asn1pars.c \
    enc.c \
    sess_id.c \
    genrsa.c \
    cms.c \
    dsa.c \
    app_rand.c \
    pkcs7.c \
    spkac.c \
    crl2p7.c \
    engine.c \
    ocsp.c \
    crl.c \
    openssl.c \
    ciphers.c \
    dgst.c \
    s_socket.c \
    version.c \
    srp.c \
    passwd.c \
    dhparam.c \
    s_server.c \
    pkcs12.c \
    rsautl.c \
    rand.c \
    dh.c \
    gendh.c \
    nseq.c \
    dsaparam.c \
    gendsa.c \
    ecparam.c \
    genpkey.c \
    ca.c \
    s_cb.c \
    x509.c \
    s_time.c \
    pkey.c \
    ts.c \
    apps.c \
    prime.c \
    pkcs8.c \
    rsa.c \
    s_client.c \
    verify.c \
    pkeyparam.c \
    speed.c \
    errstr.c \
    pkeyutl.c

LOCAL_SHARED_LIBRARIES := \
	libssl \
	libcrypto 

LOCAL_C_INCLUDES := \
	openssl \
	openssl/include

LOCAL_CFLAGS := -DMONOLITH

LOCAL_CFLAGS += -DOPENSSL_NO_ECDH

include $(LOCAL_PATH)/../android-config.mk


# These flags omit whole features from the commandline "openssl".
# However, portions of these features are actually turned on.
LOCAL_CFLAGS += -DOPENSSL_NO_EC -DOPENSSL_NO_ECDSA -DOPENSSL_NO_DTLS1


LOCAL_MODULE:= openssl

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
