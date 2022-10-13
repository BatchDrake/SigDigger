#!/bin/bash
#
#  dist-dmg.sh: Deploy SigDigger in MacOS disk image format
#
#  Copyright (C) 2021 Gonzalo José Carracedo Carballal
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

. dist-common.sh

STAGINGDIR="$DEPLOYROOT/SigDigger"

function deploy()
{
    try "Locate windeployqt..." which windeployqt
    try "Create staging dir..." mkdir -p "$STAGINGDIR"
    try "Deploying via windeployqt..." windeployqt --no-translations "$DEPLOYROOT/usr/bin/SigDigger.exe" --dir "$STAGINGDIR"
    try "Copying SigDigger to staging dir..." cp "$DEPLOYROOT/usr/bin/SigDigger.exe" "$STAGINGDIR"
    try "Copying Suscan CLI tool (suscli) to staging dir..." cp "$DEPLOYROOT/usr/bin/suscli.exe" "$STAGINGDIR"
    try "Copying data directory..." cp -R "$DEPLOYROOT/usr/share/suscan/config" "$STAGINGDIR"
    try "Copying SoapySDR modules..." cp -R /mingw64/lib/SoapySDR/modules0.8*/ "$STAGINGDIR"

    gather_dlls
}

function fetch_dll()
{
    if [ -f "$DEPLOYROOT/usr/lib/$1" ]; then
	    try "Fetching $1 (deploy root)" cp "$DEPLOYROOT/usr/lib/$1" "$STAGINGDIR/$2"
    elif [ -f "/mingw64/lib/$1" ]; then
	    try "Fetching $1 (lib)..." cp "/mingw64/lib/$1" "$STAGINGDIR/$2"
    elif [ -f "/mingw64/bin/$1" ]; then
	    try "Fetching $1 (bin)..." cp "/mingw64/bin/$1" "$STAGINGDIR/$2"
    else
            echo "$DEPLOYROOT/usr/lib/*.dll"
            echo /mingw64/bin/*.dll
            echo /mingw64/lib/*.dll
	    try "$1 NOT FOUND!" false
    fi
}

function gather_dlls()
{
    fetch_dll libsuscan.dll
    fetch_dll libsigutils.dll
    fetch_dll suwidgets0.dll
    fetch_dll libportaudio.dll
    fetch_dll libxml2-2.dll
    fetch_dll libsndfile-1.dll
    fetch_dll libSoapySDR.dll
    fetch_dll librtlsdr.dll
    fetch_dll libfftw3-3.dll
    fetch_dll libfftw3f-3.dll
    fetch_dll libcurl-4.dll
    fetch_dll libstdc++-6.dll
    fetch_dll libgcc_s_seh-1.dll
    fetch_dll libwinpthread-1.dll
    fetch_dll libdouble-conversion.dll
    fetch_dll libicuin71.dll
    fetch_dll libicuuc71.dll
    fetch_dll libicudt71.dll
    fetch_dll libpcre2-16-0.dll
    fetch_dll zlib1.dll
    fetch_dll libzstd.dll
    fetch_dll libharfbuzz-0.dll
    fetch_dll libfreetype-6.dll
    fetch_dll libbz2-1.dll
    fetch_dll libusb-1.0.dll
    fetch_dll librtlsdr.dll
    fetch_dll libbrotlidec.dll
    fetch_dll libbrotlicommon.dll
    fetch_dll libpng16-16.dll
    fetch_dll libglib-2.0-0.dll
    fetch_dll libintl-8.dll
    fetch_dll libiconv-2.dll
    fetch_dll libpcre-1.dll
    fetch_dll libgraphite2.dll
    fetch_dll libmd4c.dll
    fetch_dll libcrypto-1_1-x64.dll
    fetch_dll libidn2-0.dll
    fetch_dll libunistring-2.dll
    fetch_dll libnghttp2-14.dll
    fetch_dll libpsl-5.dll
    fetch_dll libssh2-1.dll
    fetch_dll libssl-1_1-x64.dll
    fetch_dll libFLAC.dll
    fetch_dll libssp-0.dll
    fetch_dll libogg-0.dll
    fetch_dll libopus-0.dll
    fetch_dll libvorbis-0.dll
    fetch_dll libvorbisenc-2.dll
    fetch_dll libvorbisfile-3.dll
    fetch_dll libvolk.dll
    fetch_dll liblzma-5.dll
    fetch_dll liborc-0.4-0.dll
}

function create_bundle()
{
    PARENT=`dirname "$STAGINGDIR"`
    BASE=`basename "$STAGINGDIR"`
    BUNDLEPATH="$DISTROOT/$DISTFILENAME-win32.zip"

    cd "$PARENT"
    
    try "Creating ZIP file and finishing..." zip -r "$BUNDLEPATH" "$BASE"
    
    echo "Bundle file generated in $BUNDLEPATH"
}

build
deploy
create_bundle

