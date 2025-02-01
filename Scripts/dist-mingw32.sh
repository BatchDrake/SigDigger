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
LOGFILE="$PWD/dll.log"
WINDEPLOYQT_CMD=""

function locate_windeployqt()
{
    if [ "x$WINDEPLOYQT_CMD" == "x" ]; then
	if cmd_exists windeployqt6; then
	    export WINDEPLOYQT_CMD=windeployqt6
	elif cmd_exists windeployqt-qt6; then
	    export WINDEPLOYQT_CMD=windeployqt-qt6
	elif cmd_exists windeployqt; then
	    export WINDEPLOYQT_CMD=windeployqt
	elif cmd_exists windeployqt5; then
	    export WINDEPLOYQT_CMD=windeployqt5
	elif cmd_exists windeployqt-qt5; then
	    export WINDEPLOYQT_CMD=windeployqt-qt5
	else
	    return 1
	fi
    else
	if ! cmd_exists "$WINDEPLOYQT_CMD"; then
	    return 1
	fi
    fi

    return 0
}

function enum_dlls() {
    strace SoapySDRUtil --find > "$LOGFILE" 2>&1
    pushd "$STAGINGDIR" > /dev/null
    strace SigDigger --help >> "$LOGFILE" 2>&1
    result=$?
    popd > /dev/null
    return $result
}

function deploy()
{
    try "Locate windeployqt..." locate_windeployqt
    notice "WINDEPLOYQT_CMD=$WINDEPLOYQT_CMD"
    try "Create staging dir..." mkdir -p "$STAGINGDIR"
    try "Deploying via windeployqt..." "$WINDEPLOYQT_CMD" --no-translations "$DEPLOYROOT/usr/bin/SigDigger.exe" --dir "$STAGINGDIR"
    try "Copying SigDigger to staging dir..." cp "$DEPLOYROOT/usr/bin/SigDigger.exe" "$STAGINGDIR"
    try "Copying Suscan CLI tool (suscli) to staging dir..." cp "$DEPLOYROOT/usr/bin/suscli.exe" "$STAGINGDIR"
    try "Copying data directory..." cp -R "$DEPLOYROOT/usr/share/suscan/config" "$STAGINGDIR"
    try "Copying SoapySDR modules..." cp -R /mingw64/lib/SoapySDR/modules0.8*/ "$STAGINGDIR"

    gather_dlls
}

function fetch_dll()
{
    if [ -f "$1" ]; then
	try "Fetch $1" cp "$1" "$STAGINGDIR/$2"
    elif [ -f "$DEPLOYROOT/usr/lib/$1" ]; then
	try "Fetching $1 (deploy root lib)" cp "$DEPLOYROOT/usr/lib/$1" "$STAGINGDIR/$2"
    elif [ -f "$DEPLOYROOT/usr/bin/$1" ]; then
	try "Fetching $1 (deploy root bin)" cp "$DEPLOYROOT/usr/bin/$1" "$STAGINGDIR/$2"
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


function bring_discovered_dlls() {
    cat "$LOGFILE" | grep loaded | grep dll | grep -iv ':\\Windows\\' | sed 's/^.*\([C-Z]\):\\/\\\1\\/g' | sed 's/\.dll.*/\.dll/g' | tr '\\' '/' > "dll-list.txt"
    for dll in `cat dll-list.txt`; do
	filename=`basename "$dll"`
	pathname=`dirname "$dll"`

	if echo "$pathname" | grep "$DEPLOYROOT" > /dev/null; then
	    skip "Skipping $filename..."
	else
	    try "Bringing $filename..." cp "$dll" "$STAGINGDIR"
	fi
    done
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
    
    try "Enumerate SigDigger DLLs..." enum_dlls
    bring_discovered_dlls
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

