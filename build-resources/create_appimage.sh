#!/bin/bash
rm -r AppDir
export LD_LIBRARY_PATH=~/Qt/6.8.0/gcc_64/lib
echo $LD_LIBRARY_PATH
export PATH=$HOME/Qt/6.8.0/gcc_64/bin:$PATH
./linuxdeploy-x86_64.AppImage --appdir=AppDir --executable=./gbp --plugin=qt --desktop-file=gbp.desktop  --icon-file=gbp.png
cp *.qm AppDir/usr/bin
./linuxdeploy-x86_64.AppImage --appdir=AppDir --output=appimage
