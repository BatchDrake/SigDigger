#!/bin/bash
#
#  dist-common.sh: Build SigDigger and prepare deployment
#
#  The following environment variables adjust the behavior of this script:
#
#    SIGDIGGER_SKIPBUILD: Skip cloning and build.
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
BRANCH=develop
OSTYPE=`uname -s`
ARCH=`uname -m`
RELEASE="0.2.0"
DISTFILENAME=SigDigger-"$RELEASE"-"$ARCH"
PKGVERSION=""

if [ "$OSTYPE" == "Linux" ]; then
    SCRIPTPATH=`realpath "$0"`
else
    SCRIPTPATH="$0"
fi

SCRIPTDIR=`dirname "$SCRIPTPATH"`
DEPLOYROOT="$DISTROOT/deploy-root"
BUILDROOT="$DISTROOT/build-root"

if [ "$OSTYPE" == "Linux" ]; then
  THREADS=`cat /proc/cpuinfo | grep processor | wc -l`
elif [ "$OSTYPE" == "Darwin" ]; then
  THREADS=`sysctl -n hw.ncpu`
else
  THREADS=1
fi

function try()
{
    echo -en "[  ....  ] $1 "
    shift
    
    STDOUT="$DISTROOT/$1-$$-stdout.log"
    STDERR="$DISTROOT/$1-$$-stderr.log"
    echo "Try: $@"    >> "$STDERR"
    "$@" > "$STDOUT" 2>> "$STDERR"
    
    if [ $? != 0 ]; then
	echo -e "\r[ \033[1;31mFAILED\033[0m ]"
	echo 
  echo '--------------8<----------------------------------------'
  cat "$STDERR"
  echo '--------------8<----------------------------------------'
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

function notice()
{
    echo -e "[ \033[1;33mNOTICE\033[0m ] $1"
}

function locate_qt()
{
    if [ "x$QT_BIN_PATH" == "x" ]; then
      try "Locating Qt..." which qmake
    
      export QT_QMAKE_PATH=`which qmake`
      export QT_BIN_PATH=`dirname "$QT_QMAKE_PATH"`
    fi
}

function locate_sdk()
{
  try "Locate Git..." which git
  try "Locate Make..." which make
  try "Ensuring pkgconfig availability... " which pkg-config
  try "Locating sndfile... " pkg-config sndfile
  try "Locating libxml2..." pkg-config libxml-2.0
  try "Locating FFTW3..." pkg-config fftw3

  if ! pkg-config alsa; then
      try "Locating PortAudio..." pkg-config portaudio-2.0
  fi

  try "Locating SoapySDR..." pkg-config SoapySDR

  if ! pkg-config volk; then
    notice '*** Volk development files not found ***'
    notice '  Although the build will not be stopped because of this, it is extremely'
    notice '  recommended to install Volk to leverage CPU-specific math instructions'
    notice '  and improve overall performance.'
  fi
  
  locate_qt
  
  try "Locating CMake..." which cmake 
}

function build()
{
    if [ "$SIGDIGGER_SKIPBUILD" == "" ]; then
        locate_sdk
    
        try "Cleaning deploy directory..." rm -Rfv "$DEPLOYROOT"
        try "Cleaning buildroot..."        rm -Rfv "$BUILDROOT"
        try "Recreating directories..."    mkdir -p "$DEPLOYROOT" "$BUILDROOT"

        cd "$BUILDROOT"
        export PKG_CONFIG_PATH="$DEPLOYROOT/usr/lib/pkgconfig:$PKG_CONFIG"
        export LD_LIBRARY_PATH="$DEPLOYROOT/usr/lib:$LD_LIBRARY_PATH"

        try "Cloning sigutils..."          git clone -b "$BRANCH" https://github.com/BatchDrake/sigutils
        try "Cloning suscan..."            git clone -b "$BRANCH" https://github.com/BatchDrake/suscan
        try "Cloning SuWidgets..."         git clone -b "$BRANCH" https://github.com/BatchDrake/SuWidgets
        try "Cloning SigDigger..."         git clone -b "$BRANCH" https://github.com/BatchDrake/SigDigger
        try "Creating builddirs..."        mkdir -p sigutils/build suscan/build
        cd sigutils/build
        try "Running CMake (sigutils)..."  cmake .. -DCMAKE_INSTALL_PREFIX="$DEPLOYROOT/usr" -DPKGVERSION="$PKGVERSION" -DCMAKE_BUILD_TYPE=Release -DCMAKE_SKIP_RPATH=ON -DCMAKE_SKIP_INSTALL_RPATH=ON
        cd ../../
        try "Building sigutils..."         make -j $THREADS -C sigutils/build
        try "Deploying sigutils..."        make -j $THREADS -C sigutils/build install

        cd suscan/build
        try "Running CMake (suscan)..."    cmake .. -DCMAKE_INSTALL_PREFIX="$DEPLOYROOT/usr" -DPKGVERSION="$PKGVERSION" -DCMAKE_BUILD_TYPE=Release -DCMAKE_SKIP_RPATH=ON -DCMAKE_SKIP_INSTALL_RPATH=ON -DSUSCAN_PKGDIR="/usr"
        cd ../../
        try "Building suscan..."           make -j $THREADS -C suscan/build
        try "Deploying suscan..."          make -j $THREADS -C suscan/build install

        cd SuWidgets
        try "Running QMake (SuWidgets)..." qmake SuWidgetsLib.pro "CONFIG += release" PREFIX="$DEPLOYROOT/usr"
        try "Building SuWidgets..."        make -j $THREADS
        try "Deploying SuWidgets..."       make install
        cd ..

        cd SigDigger
        try "Running QMake (SigDigger)..." qmake SigDigger.pro "CONFIG += release" SUWIDGETS_PREFIX="$DEPLOYROOT/usr" PREFIX="$DEPLOYROOT/usr"
        try "Building SigDigger..."        make -j $THREADS
        try "Deploying SigDigger..."       make install
        cd ..
    else
        skip "Skipping build..."
    fi
}

ESCAPE=`echo -en '\033'`
echo -en '\033[0;1m'

cat << EOF
Welcome to...$ESCAPE[1;36m

    ____  _       ____  _                       
   / ___|(_) __ _|  _ \(_) __ _  __ _  ___ _ __ 
   \___ \| |/ _\` | | | | |/ _\` |/ _\` |/ _ \ \'__|
    ___) | | (_| | |_| | | (_| | (_| |  __/ |   
   |____/|_|\__, |____/|_|\__, |\__, |\___|_|   ('s)   
            |___/         |___/ |___/           

$ESCAPE[0;1m...multi platform deployment script.$ESCAPE[0m
EOF

echo
echo "Attempting deployment on $OSTYPE ($ARCH)"
echo "Date: "`date`
echo "SigDigger release to be built: $RELEASE"
echo
