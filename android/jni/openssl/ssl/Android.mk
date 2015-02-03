LOCAL_PATH:= $(call my-dir)

common_SRC_FILES:= \
    ssl_algs.c \
    d1_both.c \
    t1_trce.c \
    t1_lib.c \
    t1_reneg.c \
    s23_pkt.c \
    s3_pkt.c \
    d1_clnt.c \
    ssl_err.c \
    s23_clnt.c \
    s3_lib.c \
    t1_ext.c \
    d1_meth.c \
    d1_srvr.c \
    t1_srvr.c \
    s2_meth.c \
    s3_both.c \
    t1_enc.c \
    s3_enc.c \
    d1_srtp.c \
    ssl_asn1.c \
    s2_pkt.c \
    ssl_ciph.c \
    s2_lib.c \
    t1_clnt.c \
    ssl_rsa.c \
    s2_srvr.c \
    s3_srvr.c \
    s23_meth.c \
    kssl.c \
    ssl_txt.c \
    s23_srvr.c \
    ssl_sess.c \
    d1_pkt.c \
    ssl_lib.c \
    tls_srp.c \
    s3_meth.c \
    s3_cbc.c \
    s23_lib.c \
    ssl_err2.c \
    bio_ssl.c \
    s3_clnt.c \
    ssl_conf.c \
    ssl_stat.c \
    s2_clnt.c \
    d1_lib.c \
    ssl_cert.c \
    ssl_utst.c \
    s2_enc.c \
    t1_meth.c

common_C_INCLUDES += \
	openssl \
	openssl/include \
	openssl/crypto

# static library
# =====================================================

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= $(common_SRC_FILES)
include $(LOCAL_PATH)/../android-config.mk
LOCAL_C_INCLUDES:= $(common_C_INCLUDES)
#LOCAL_PRELINK_MODULE:= false
#LOCAL_STATIC_LIBRARIES += libcrypto-static
LOCAL_MODULE:= libssl-static
include $(BUILD_STATIC_LIBRARY)
