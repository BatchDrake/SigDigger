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
OSTYPE=`uname -s`
ARCH=`uname -m`
RELEASE="0.3.0"
DISTFILENAME=SigDigger-"$RELEASE"-"$ARCH"
PKGVERSION=""
MAKE="make"
CMAKE_SUSCAN_EXTRA_ARGS=""
QMAKE_SIGDIGGER_EXTRA_ARGS=""

if [ "x$BRANCH" == "x" ]; then
    BRANCH=develop
fi

if [ "x$BUILDTYPE" == "x" ]; then
    BUILDTYPE="Release"
fi

function is_mingw()
{
    echo "$OSTYPE" | grep MINGW > /dev/null
    return $?
}

function help()
{
  echo "$1: SigDigger's build script"
  echo "Usage:"
  echo "  $1 [OPTIONS] [PLUGINS]"
  echo
  echo "Options:"
  echo "  --disable-alsa:      Explicitly disable ALSA support"
  echo "  --disable-portaudio: Explicitly disable PortAudio support"
  echo
  echo "  --help               This help"
  echo
}

opt=$1
SCRIPTNAME="$0"
PLUGINS=""

while [ "$opt" != "" ]; do
    case "$opt" in
      --disable-alsa)
        CMAKE_SUSCAN_EXTRA_ARGS="$CMAKE_SUSCAN_EXTRA_ARGS -DENABLE_ALSA=OFF"
        QMAKE_SIGDIGGER_EXTRA_ARGS="$QMAKE_SIGDIGGER_EXTRA_ARGS DISABLE_ALSA=1"
      ;;
    
      --disable-portaudio)
        CMAKE_SUSCAN_EXTRA_ARGS="$CMAKE_SUSCAN_EXTRA_ARGS -DENABLE_PORTAUDIO=OFF"
        QMAKE_SIGDIGGER_EXTRA_ARGS="$QMAKE_SIGDIGGER_EXTRA_ARGS DISABLE_PORTAUDIO=1"
      ;;
    
      --help)
        help "$SCRIPTNAME"
        exit 0
      ;;

      -*)
        echo "$SCRIPTNAME: unrecognized option $opt"
        help "$SCRIPTNAME"
        exit 1
      ;;

      *)
      PLUGINS="$PLUGINS $opt"
      ;;
    esac

    shift
    opt="$1"
done

if [ "$OSTYPE" == "Linux" ]; then
    SCRIPTPATH=`realpath "$0"`
elif is_mingw; then
    SCRIPTPATH=`realpath "$0"`
    MAKE="mingw32-make"
else
    SCRIPTPATH="$0"
fi

SCRIPTDIR=`dirname "$SCRIPTPATH"`
DEPLOYROOT="$DISTROOT/deploy-root"
BUILDROOT="$DISTROOT/build-root"

if [ "$OSTYPE" == "Linux" ] || is_mingw; then
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
    echo "CWD: $PWD"  >> "$STDERR"
    
    "$@" > "$STDOUT" 2>> "$STDERR"
    
    if [ $? != 0 ]; then
	echo -e "\r[ \033[1;31mFAILED\033[0m ]"
	echo 
	echo '--------------8<----------------------------------------'
	cat "$STDERR"
	echo '--------------8<----------------------------------------'

	if [ "$BUILDTYPE" == "Debug" ]; then
	    echo
	    echo 'Debug mode - attatching standard output:'
	    echo '--------------8<----------------------------------------'
	    cat "$STDOUT"
	    echo '--------------8<----------------------------------------'
	    echo ''
	    echo 'Deploy root filelist: '
	    echo '--------------8<----------------------------------------'
	    find "$DEPLOYROOT"
	    echo '--------------8<----------------------------------------'
	    echo ''
	fi
	
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
  if is_mingw; then
      try "Probing MinGW64..." test -d /mingw64/bin
      export PATH="/mingw64/bin:$PATH:/mingw64/bin"
      export LD_LIBRARY_PATH="/mingw64/lib:/mingw64/bin:$LD_LIBRARY_PATH:/mingw64/lib:/mingw64/bin"
      export CMAKE_EXTRA_OPTS='-GMinGW Makefiles'
  else
    export CMAKE_EXTRA_OPTS='-GUnix Makefiles'
  fi

  try "Locate Make..." which $MAKE
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

function build_plugins()
{
  PLUGINDIR="$BUILDROOT/plugins"
  PLUGINTARGET="$DEPLOYROOT/usr/share/suscan/plugins"
  try "Creating plugin source dir..." mkdir -p "$PLUGINDIR"
  try "Creating plugin dir..." mkdir -p "$PLUGINTARGET"

  for plugin in $PLUGINS; do
    cd "$PLUGINDIR"
    
    if [ -d "$plugin" ]; then
      cd "$plugin"
      try "Attempting to pull latest changes from $plugin" git pull origin master
    else
      try "Attempting to clone plugin: $plugin..." git clone https://github.com/BatchDrake/"$plugin"
      cd "$plugin"
    fi
    
    if [ ! -f "$plugin".pro ]; then
      notice "Project does not look like a SigDigger plugin, skipped"
      continue
    fi

    try "  Running qmake ($plugin)..." qmake "$plugin".pro $QMAKE_SIGDIGGER_EXTRA_ARGS "CONFIG += $QMAKE_BUILDTYPE" SUWIDGETS_PREFIX="$DEPLOYROOT/usr" SIGDIGGER_PREFIX="$DEPLOYROOT/usr" PLUGIN_DIRECTORY="$PLUGINTARGET"
    try "  Building ($plugin)... " $MAKE -j $THREADS
    try "  Installing ($plugin)... " $MAKE install

  done
}

function build()
{
    if [ "$BUILDTYPE" == "Debug" ]; then
      notice 'Build with debug symbols is ON!'
      CMAKE_BUILDTYPE=Debug
      QMAKE_BUILDTYPE=debug
      export VERBOSE=1
    else
      CMAKE_BUILDTYPE=Release
      QMAKE_BUILDTYPE=release
    fi

    if [ "$SIGDIGGER_SKIPBUILD" == "" ]; then
        locate_sdk
        export PKG_CONFIG_PATH="$DEPLOYROOT/usr/lib/pkgconfig:$PKG_CONFIG_PATH"
        export LD_LIBRARY_PATH="$DEPLOYROOT/usr/lib:$LD_LIBRARY_PATH"
        try "Cleaning deploy directory..." rm -Rfv "$DEPLOYROOT"
        try "Cleaning buildroot..."        rm -Rfv "$BUILDROOT"
        try "Recreating directories..."    mkdir -p "$DEPLOYROOT" "$BUILDROOT"

        cd "$BUILDROOT"
        
        try "Cloning sigutils (${BRANCH})..."          git clone -b "$BRANCH" https://github.com/BatchDrake/sigutils
        try "Cloning suscan (${BRANCH})..."            git clone -b "$BRANCH" https://github.com/BatchDrake/suscan
        try "Cloning SuWidgets (${BRANCH})..."         git clone -b "$BRANCH" https://github.com/BatchDrake/SuWidgets
	      try "Cloning SigDigger (${BRANCH})..."         git clone -b "$BRANCH" https://github.com/BatchDrake/SigDigger
        try "Creating builddirs..."        mkdir -p sigutils/build suscan/build
        cd sigutils/build
        try "Running CMake (sigutils)..."  cmake .. -DCMAKE_INSTALL_PREFIX="$DEPLOYROOT/usr" -DPKGVERSION="$PKGVERSION" -DCMAKE_BUILD_TYPE=$CMAKE_BUILDTYPE "$CMAKE_EXTRA_OPTS" -DCMAKE_SKIP_RPATH=ON -DCMAKE_SKIP_INSTALL_RPATH=ON
        cd ../../
        try "Building sigutils..."         $MAKE -j $THREADS -C sigutils/build
        try "Deploying sigutils..."        $MAKE -j $THREADS -C sigutils/build install
        if [ "$BUILDTYPE" == "Debug" ]; then
            try "Testing deplyment of sigutils..." pkg-config sigutils
            _headers=`pkg-config sigutils --cflags`
            _libs=`pkg-config sigutils --libs`
            notice "Sigutils headers:   ${_headers}"
            notice "Sigutils libraries: ${_libs}"
        fi
	
        cd suscan/build
        try "Running CMake (suscan)..."    cmake .. $CMAKE_SUSCAN_EXTRA_ARGS -DCMAKE_INSTALL_PREFIX="$DEPLOYROOT/usr" -DPKGVERSION="$PKGVERSION" -DCMAKE_BUILD_TYPE=$CMAKE_BUILDTYPE "$CMAKE_EXTRA_OPTS" -DCMAKE_SKIP_RPATH=ON -DCMAKE_SKIP_INSTALL_RPATH=ON -DSUSCAN_PKGDIR="/usr"
        cd ../../
        try "Building suscan..."           $MAKE -j $THREADS -C suscan/build
        try "Deploying suscan..."          $MAKE -j $THREADS -C suscan/build install

        if [ "$BUILDTYPE" == "Debug" ]; then
            try "Testing deplyment of suscan..." pkg-config suscan
            _headers=`pkg-config suscan --cflags`
            _libs=`pkg-config suscan --libs`
            notice "Suscan headers:   ${_headers}"
            notice "Suscan libraries: ${_libs}"
        fi
	
        cd SuWidgets
        try "Running QMake (SuWidgets)..." qmake SuWidgetsLib.pro "CONFIG += $QMAKE_BUILDTYPE" PREFIX="$DEPLOYROOT/usr"
        try "Building SuWidgets..."        $MAKE -j $THREADS
        try "Deploying SuWidgets..."       $MAKE install
        cd ..

        cd SigDigger
        try "Running QMake (SigDigger)..." qmake SigDigger.pro $QMAKE_SIGDIGGER_EXTRA_ARGS "CONFIG += $QMAKE_BUILDTYPE" SUWIDGETS_PREFIX="$DEPLOYROOT/usr" PREFIX="$DEPLOYROOT/usr"
        try "Building SigDigger..."        $MAKE -j $THREADS
        try "Deploying SigDigger..."       $MAKE install
        cd ..
    else
        skip "Skipping build..."
    fi

    build_plugins
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
echo "Attempting deployment on $OSTYPE ($ARCH) with $THREADS threads"
echo "Date: "`date`
echo "SigDigger release to be built: $RELEASE"
echo
