#!/bin/sh

OLD_PATH=$PATH

export PATH=/opt/fltk/1.3.0-darwin-i386/bin:/opt/i686-apple-darwin10/toolchain/bin:$PATH
make SYSTEM=Darwin package
make SYSTEM=Darwin clean

sleep 1

export PATH=/opt/fltk/1.3.0-mingw32/bin:/opt/i686-mingw32/toolchain/bin:$OLD_PATH
make SYSTEM=mingw32 package
make SYSTEM=mingw32 clean

export PATH=$OLD_PATH

sleep 1

make package
make clean
