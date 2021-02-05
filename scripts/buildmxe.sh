#!/bin/sh

./configure \
  $PKGCFG \
  --host=i686-w64-mingw32.static \
  --with-ptw32=$PREFIX/i686-w64-mingw32.static \
  --enable-static \
  PTW32_LIBS="-lpthread -lpcreposix -lpcre" \
  FLTK_CONFIG=$PREFIX/bin/i686-w64-mingw32.static-fltk-config

make -j 3

$PREFIX/bin/i686-w64-mingw32.static-strip src/flnet.exe
make nsisinst
mv src/*setup*exe .
