#!/bin/sh
# Build flwm clasico x86_64 para flinux-jc64, siguiendo compileit de TC
# (solo el binario clasico; sin sstrip en el host -> strip normal).
set -e
cd "$(dirname "$0")/src"
CXXFLAGS="-flto -fuse-linker-plugin -mtune=generic -Os -pipe -fno-exceptions -fno-rtti"
LDFLAGS="-Wl,-O1"
CPPFLAGS=""
if fltk-config --ldflags | grep -q Xft; then CPPFLAGS="-DHAVE_XFT"; fi
CXXFLAGS="$CXXFLAGS $(fltk-config --cxxflags) -Wall -ffunction-sections -fdata-sections -Wno-strict-aliasing"
LDFLAGS="$LDFLAGS $(fltk-config --ldflags --use-images) -Wl,-gc-sections"
echo "CXXFLAGS=$CXXFLAGS"; echo "LDFLAGS=$LDFLAGS"; echo "CPPFLAGS=$CPPFLAGS"
g++ -o flwm *.C $CXXFLAGS $LDFLAGS $CPPFLAGS
strip flwm
file flwm
