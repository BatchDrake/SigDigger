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

export BUILDTYPE=Release

BUNDLEID="org.actinid.SigDigger"

BUNDLEPATH="$DEPLOYROOT/usr/bin/SigDigger.app"
PLISTPATH="$BUNDLEPATH/Contents/Info.plist"
RSRCPATH="$BUNDLEPATH/Contents/Resources"
LIBPATH="$BUNDLEPATH/Contents/Frameworks"
BINPATH="$BUNDLEPATH/Contents/MacOS"
STAGINGDIR="$DEPLOYROOT/SigDigger.dir"
DMG_NAME="$DISTFILENAME".dmg

function locate_macdeploy()
{
  locate_qt
  
  export PATH="$QT_BIN_PATH:$PATH"
  
  try "Locating hdiutil..." which hdiutil
  try "Locating macdeployqt..." which macdeployqt
}

function bundle_libs()
{
  name="$1"
  shift
  
  try "Bundling $name..." cp -RLfv "$@" "$LIBPATH"
}

#
# If at some point, SigDigger needs additional libraries, expand this function
# adding the missing dependencies with bundle_libs.
#
# bundle_libs accepts the name of the set of libraries (for informative
# purposes only) and a set of libraries.
#

function find_soapysdr()
{
    SOAPYDIRS="/usr/lib/`uname -m`-linux-gnu /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64"
    
    for i in $SOAPYDIRS; do
	MODDIR="$i/SoapySDR/modules$SOAPYSDRVER"
	if [ -d "$MODDIR" ]; then
	    echo "$MODDIR"
	    return 0
	fi
    done

    return 1
}

function excluded()
{
    excludelist="libc++.1.dylib libSystem.B.dylib"

    for ef in `echo $excludelist`; do
	if [ "$ef" == "$1" ]; then
	   return 0
	fi
    done

    return 1
}

function find_lib()
{
  SANE_DIRS="/usr/lib/`uname -m`-linux-gnu /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64 $LIBPATH"
    
  for i in $SANE_DIRS; do
	  if [ -f "$i/$1" ]; then
      echo "$i/$1"
      return 0
    fi
  done

  return 1
}

function embed_soapysdr()
{
    export SOAPYSDRVER=`pkg-config SoapySDR --modversion | sed 's/\([0-9]*\.[0-9]*\)\..*/\1/g'`
    try "Testing SoapySDR version..." [ "$SOAPYSDRVER" != "" ]
    try "Testing SoapySDR dir..." find_soapysdr

    MODDIR=`find_soapysdr`

    try "Creating SoapySDR module dir..."       mkdir -p "$LIBPATH/SoapySDR/"
    try "Copying SoapySDR modules ($MODDIR)..." cp -RLfv "$MODDIR" "$LIBPATH/SoapySDR"

    RADIODEPS=`otool -L "$MODDIR"/lib* | grep -v :$ | sed 's/ (.*)//g'`
    MY_RPATH=/usr/local/lib # FIXME
    
    for i in $RADIODEPS; do
      name=`basename "$i"`
      dirname=`dirname "$i"`
      
      if [ "$dirname" == "@rpath" ]; then
        i="$MY_RPATH/$name"
      elif [ "$dirname" == "." ]; then
        i=`find_lib "$name"`
        if [ "$i" == "" ]; then
          echo -e "[ \033[1;31mFAILED\033[0m ] Could not locate $name"
          return 1
        fi
      fi

      if [ ! -f "$LIBPATH"/"$name" ] && ! excluded "$name"; then
	  rm -f "$LIBPATH"/"$name"
          try "Bringing $name..." cp -L "$i" "$LIBPATH"
      elif excluded "$name"; then
	  rm -f "$LIBPATH"/"$name"
	  skip "Excluding $name..."
      else
          skip "Skipping $name..."
      fi
    done
    
    return 0
}

function deploy_deps()
{
  embed_soapysdr
  bundle_libs "SoapySDR libraries" /usr/local/lib/libSoapySDR*dylib
  bundle_libs "GCC support libraries"   /usr/local/opt/gcc/lib/gcc/11/libgcc_s.1.1.dylib
}

function remove_full_paths()
{
  FULLPATH="$2"
  RPATHNAME='@executable_path/../Frameworks/'`basename "$FULLPATH"`

  install_name_tool -change "$FULLPATH" "$RPATHNAME" "$1"
}

function remove_full_path_stdin () {
    while read line; do
    remove_full_paths "$1" "$line"
  done
}

function ensure_rpath()
{
  for i in "$LIBPATH"/*.dylib "$LIBPATH/SoapySDR/modules"*/*.so "$BUNDLEPATH"/Contents/MacOS/*; do
      if ! [ -L "$i" ]; then
	  chmod u+rw "$i"
	  try "Fixing "`basename $i`"..." true
	  otool -L "$i" | grep '\t/usr/local/' | tr -d '\t' | cut -f1 -d ' ' | remove_full_path_stdin "$i";
	  otool -L "$i" | grep '\t@rpath/.*\.dylib' | tr -d '\t' | cut -f1 -d ' ' | remove_full_path_stdin "$i";
      fi
  done
}

function create_dmg()
{
  try "Cleaning up old files..." rm -Rfv "$STAGINGDIR"
  try "Creating staging directory..." mkdir -p "$STAGINGDIR"
  try "Copying bundle to staging dir..."   cp -Rfv "$BUNDLEPATH" "$STAGINGDIR"
  try "Creating .dmg file and finishing..." hdiutil create -verbose -volname SigDigger -srcfolder "$STAGINGDIR" -ov -format UDZO "$DISTROOT/$DMG_NAME"
}

function fix_plist()
{
  try "Setting bundle ID..."                 plutil -replace CFBundleIdentifier -string "$BUNDLEID" "$PLISTPATH"
  try "Setting bundle name..."               plutil -replace CFBundleName -string "SigDigger" "$PLISTPATH"
  try "Setting bundle version ($RELEASE)..." plutil -replace CFBundleShortVersionString -string "$RELEASE" "$PLISTPATH"
  try "Setting bundle language..."           plutil -replace CFBundleDevelopmentRegion -string "en" "$PLISTPATH"
  try "Setting NOTE..."                      plutil -replace NOTE -string "Bundled width SigDigger's deployment script" "$PLISTPATH"
}

function deploy()
{
  locate_macdeploy

  try "Deploying via macdeployqt..." macdeployqt "$BUNDLEPATH"
  
  try "Copying Suscan data directory to bundle..." cp -Rfv "$DEPLOYROOT/usr/share/suscan" "$RSRCPATH"
  try "Copying Suscan CLI tool (suscli) to bundle..." cp -fv "$DEPLOYROOT/usr/bin/suscli" "$BINPATH"
  try "Copying SoapySDRUtil to bundle..." cp -fv `which SoapySDRUtil` "$BINPATH"
  try "Bundling built libraries..." cp -fv "$DEPLOYROOT/usr/lib/"*.dylib "$LIBPATH"

  deploy_deps
  ensure_rpath
  fix_plist
}

build
deploy
create_dmg




