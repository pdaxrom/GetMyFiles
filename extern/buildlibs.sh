#!/bin/bash

error() {
    echo "ERROR: $@"
    exit 1
}

TOPDIR="$PWD"
INSTDIR=${TOPDIR}/inst

TARGET_OS=$1
TARGET_ARCH=$2

if test "$TARGET_OS" = ""; then
    TARGET_OS=`uname`
fi

if test "$TARGET_ARCH" = ""; then
    TARGET_ARCH=`uname -i`
fi

OPENSSL_VERSION=1.0.2
FLTK_VERSION=1.3.3

tar zxf openssl-${OPENSSL_VERSION}.tar.gz
tar zxf fltk-${FLTK_VERSION}-source.tar.gz

cd openssl-${OPENSSL_VERSION}

if test "$TARGET_OS" = "Linux" && test "$TARGET_ARCH" = "x86_64"; then
    ./Configure --prefix=${INSTDIR} linux-x86_64 || error "configure openssl"

    make || error "make openssl"
    make install || error "install openssl"
elif test "$TARGET_OS" = "Linux" && test "$TARGET_ARCH" = "i686"; then
    ./Configure --prefix=${INSTDIR} linux-elf    || error "configure openssl"

    make || error "make openssl"
    make install || error "install openssl"
elif test "$TARGET_OS" = "Linux" && test "$TARGET_ARCH" = "mips"; then
    ./Configure --prefix=${INSTDIR} linux-mips32    || error "configure openssl"

    make || error "make openssl"
    make install || error "install openssl"
elif test "$TARGET_OS" = "Windows" && test "$TARGET_ARCH" = "i686"; then
    ./Configure --prefix=${INSTDIR} mingw

    make CC=i686-w64-mingw32-gcc AR="i686-w64-mingw32-ar rcs" RANLIB=i686-w64-mingw32-ranlib
    make install
elif test "$TARGET_OS" = "Darwin"; then
    ./Configure --prefix=${INSTDIR} darwin64-x86_64-cc || error "configure openssl"

    make || error "make openssl"
    make install || error "install openssl"
else
    error "Unknown target $TARGET_OS $TARGET_ARCH"
fi


cd ..

cd fltk-${FLTK_VERSION}

if test "$TARGET_OS" = "Windows" && test "$TARGET_ARCH" = "i686"; then
    ./configure --prefix=${INSTDIR} --host=i686-w64-mingw32 || error "configure fltk"
    sed -i -e "s|all:|all:#|" test/Makefile
else
    ./configure --prefix=${INSTDIR} || error "configure fltk"
fi

make -j8 || error "make fltk"
make install || error "install fltk"

