#!/bin/bash
#
#  appimage.sh: Deploy SigDigger in AppImage format
#
#  The following environment variables adjust the behavior of this script:
#
#    SIGDIGGER_SKIPBUILD: Skip cloning and build.
#   
#    SIGDIGGER_EMBED_SOAPYSDR: Embeds SoapySDR to the resulting AppImage,
#      along with all the modules installed in the deployment system.
#
#  Copyright (C) 2020 Gonzalo José Carracedo Carballal
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program.  If not, see
#  <http://www.gnu.org/licenses/>
#
#

DISTROOT="$PWD"
SCRIPTPATH=`realpath $0`
SCRIPTDIR=`dirname "$SCRIPTPATH"`
APPIMAGEROOT="$DISTROOT/appimage-root"
BUILDROOT="$DISTROOT/appimage-buildroot"
THREADS=`cat /proc/cpuinfo | grep processor | wc -l`
SRC_APPIMAGE_NAME=SigDigger-`uname -m`.AppImage

function try()
{
    echo -en "[  ....  ] $1 "
    shift
    
    STDOUT="$DISTROOT/$1-$$-stdout.log"
    STDERR="$DISTROOT/$1-$$-stderr.log"
    "$@" > "$STDOUT" 2> "$STDERR"
    
    if [ $? != 0 ]; then
	echo -e "\r[ \033[1;31mFAILED\033[0m ]"
	echo
	echo "Standard output and error were saved respectively in:"
	echo " - $STDOUT"
	echo " - $STDERR"
	echo
	echo "Fix errors and try again"
	echo
	exit 1
    fi

    echo -e "\r[   \033[1;32mOK\033[0m   ]"
    rm "$STDOUT"
    rm "$STDERR"
}

function skip()
{
    echo -e "[  \033[1;33mSKIP\033[0m  ] $1"
}

function embed_soapysdr()
{
    SOAPYSDRVER=`ldd $APPIMAGEROOT/usr/bin/SigDigger | grep Soapy | sed 's/ =>.*$//g' | sed 's/^.*\.so\.//g'`
    EXCLUDED='libc\.so\.6|libpthread|libdl|libz\.so\.1|libm\.so\.6|\libusb-1\.0'
    try "Testing SoapySDR version..." [ "$SOAPYSDRVER" != "" ]
    try "Testing SoapySDR dir..." test -d "/usr/lib/`uname -m`-linux-gnu/SoapySDR/modules$SOAPYSDRVER"
    if [ ! -L  "$APPIMAGEROOT"/usr/lib/`uname -m`-linux-gnu ]; then
	try "Creating symlinks..." ln -s . "$APPIMAGEROOT"/usr/lib/`uname -m`-linux-gnu
    fi
    try "Creating SoapySDR module dir..." mkdir -p "$APPIMAGEROOT"/usr/lib/SoapySDR

    SOAPYDIRS="/usr/lib/`uname -m`-linux-gnu /usr/lib /usr/local/lib"

    for i in $SOAPYDIRS; do
	MODDIR="$i/SoapySDR/modules$SOAPYSDRVER"
	if [ -d "$MODDIR" ]; then
	    try "Copying SoapySDR modules ($i)..." cp -Rfv "$MODDIR" "$APPIMAGEROOT"/usr/lib/SoapySDR
	fi
    done
    
    RADIOS=`ldd "$APPIMAGEROOT"/usr/lib/SoapySDR/modules$SOAPYSDRVER/lib* | grep '=>' | sed 's/^.*=> \(.*\) .*$/\1/g' | tr -d '[ \t]' | sort | uniq`
    
    for i in $RADIOS; do
	name=`basename "$i"`
	if [ ! -f "$APPIMAGEROOT"/usr/lib/"$name" ] && ! echo "$i" | egrep "$EXCLUDED" > /dev/null; then
	    try "Bringing $name..." cp -L "$i" "$APPIMAGEROOT"/usr/lib
	else
	    rm -f "$APPIMAGEROOT"/usr/lib/"$name"
	    skip "Skipping $name..."
	fi
    done
}

if [ "$SIGDIGGER_SKIPBUILD" == "" ]; then
    try "Cleaning deploy directory..." rm -Rfv "$APPIMAGEROOT"
    try "Cleaning buildroot..."        rm -Rfv "$BUILDROOT"
    try "Recreating directories..."    mkdir -p "$APPIMAGEROOT" "$BUILDROOT"

    cd "$BUILDROOT"
    export PKG_CONFIG_PATH="$APPIMAGEROOT/usr/lib/pkgconfig:$PKG_CONFIG"
    export LD_LIBRARY_PATH="$APPIMAGEROOT/usr/lib:$LD_LIBRARY_PATH"

    try "Cloning sigutils..."          git clone https://github.com/BatchDrake/sigutils
    try "Cloning suscan..."            git clone https://github.com/BatchDrake/suscan
    try "Cloning SuWidgets..."         git clone https://github.com/BatchDrake/SuWidgets
    try "Cloning SigDigger..."         git clone https://github.com/BatchDrake/SigDigger
    try "Creating builddirs..."        mkdir -p sigutils/build suscan/build
    cd sigutils/build
    try "Running CMake (sigutils)..."  cmake .. -DCMAKE_INSTALL_PREFIX="$APPIMAGEROOT/usr"  -DCMAKE_BUILD_TYPE=Release -DCMAKE_SKIP_RPATH=ON -DCMAKE_SKIP_INSTALL_RPATH=ON
    cd ../../
    try "Building sigutils..."         make -j $THREADS -C sigutils/build
    try "Deploying sigutils..."        make -j $THREADS -C sigutils/build install

    cd suscan/build
    try "Running CMake (suscan)..."    cmake .. -DCMAKE_INSTALL_PREFIX="$APPIMAGEROOT/usr"  -DCMAKE_BUILD_TYPE=Release -DCMAKE_SKIP_RPATH=ON -DCMAKE_SKIP_INSTALL_RPATH=ON -DSUSCAN_PKGDIR="/usr"
    cd ../../
    try "Building suscan..."           make -j $THREADS -C suscan/build
    try "Deploying suscan..."          make -j $THREADS -C suscan/build install

    cd SuWidgets
    try "Running QMake (SuWidgets)..." qmake SuWidgetsLib.pro "CONFIG += release" SUWIDGETS_PREFIX="$APPIMAGEROOT/usr"
    try "Building SuWidgets..."        make -j $THREADS
    try "Deploying SuWidgets..."       make install
    cd ..

    cd SigDigger
    try "Running QMake (SigDigger)..." qmake SigDigger.pro "CONFIG += release" SUWIDGETS_PREFIX="$APPIMAGEROOT/usr" SIGDIGGER_PREFIX="$APPIMAGEROOT/usr"
    try "Building SigDigger..."        make -j $THREADS
    try "Deploying SigDigger..."       make install
    cd ..
else
    echo "Skipping build..."
fi

try "Creating appdir..."    mkdir -p "$APPIMAGEROOT"/usr/share/applications
try "Creating metainfo..."  mkdir -p "$APPIMAGEROOT"/usr/share/metainfo
try "Copying metainfo..."   cp "$SCRIPTDIR/SigDigger.appdata.xml" "$APPIMAGEROOT"/usr/share/metainfo
try "Creating icondir..."   mkdir -p "$APPIMAGEROOT"/usr/share/icons/hicolor/256x256/apps

try "Copying icons..." cp "$BUILDROOT"/SigDigger/icons/icon-256x256.png "$APPIMAGEROOT"/usr/share/icons/hicolor/256x256/apps/SigDigger.png
echo "[Desktop Entry]
Type=Application
Name=SigDigger
Comment=The Free Digital Signal Analyzer
Exec=SigDigger
Icon=SigDigger
Categories=HamRadio;" > "$APPIMAGEROOT"/usr/share/applications/SigDigger.desktop

try "Removing unneeded development files..." rm -Rfv "$APPIMAGEROOT"/usr/include "$APPIMAGEROOT"/usr/bin/suscan.status "$APPIMAGEROOT"/usr/lib/pkgconfig

if [ -f "$APPIMAGEROOT"/usr/bin/SigDigger.app ]; then
    try "Restoring old SigDigger executable..." cp "$APPIMAGEROOT"/usr/bin/SigDigger.app "$APPIMAGEROOT"/usr/bin/SigDigger
fi

if [ "$SIGDIGGER_EMBED_SOAPYSDR" != "" ]; then
    APPIMAGE_NAME=SigDigger-full-`uname -m`.AppImage
    embed_soapysdr
else
    APPIMAGE_NAME=SigDigger-lite-`uname -m`.AppImage
fi

try "Calling linuxdeployqt..." linuxdeployqt "$APPIMAGEROOT"/usr/share/applications/SigDigger.desktop -bundle-non-qt-libs

try "Moving SigDigger binary..." mv "$APPIMAGEROOT"/usr/bin/SigDigger "$APPIMAGEROOT"/usr/bin/SigDigger.app

if [ "$SIGDIGGER_EMBED_SOAPYSDR" != "" ]; then
    echo '#!/bin/sh
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export SUSCAN_CONFIG_PATH="${HERE}/../share/suscan/config"
export SOAPY_SDR_ROOT="${HERE}/.."
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${HERE}/../lib"
if [ "x$SIGDIGGER_SOAPY_SDR_ROOT" != "x" ]; then
  export SOAPY_SDR_ROOT="$SIGDIGGER_SOAPY_SDR_ROOT"
fi
exec "${HERE}"/SigDigger.app "$@"' > "$APPIMAGEROOT"/usr/bin/SigDigger
else
    echo '#!/bin/sh
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export SUSCAN_CONFIG_PATH="${HERE}/../share/suscan/config"
exec "${HERE}"/SigDigger.app "$@"' > "$APPIMAGEROOT"/usr/bin/SigDigger
fi
try "Setting permissions to wrapper script..." chmod a+x "$APPIMAGEROOT"/usr/bin/SigDigger
try "Calling AppImageTool and finishing..." appimagetool -n "$APPIMAGEROOT"
try "Renaming to $APPIMAGE_NAME..." mv "$SRC_APPIMAGE_NAME" "$DISTROOT/$APPIMAGE_NAME"
