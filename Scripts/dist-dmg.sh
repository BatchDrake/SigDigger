#!/bin/bash
#
#  dist-dmg.sh: Deploy SigDigger in MacOS disk image format
#
#  The following environment variables adjust the behavior of this script:
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

. dist-common.sh

BUNDLEPATH="$DEPLOYROOT/usr/bin/SigDigger.app"
RSRCPATH="$BUNDLEPATH/Contents/Resources"
LIBPATH="$BUNDLEPATH/Contents/Frameworks"
STAGINGDIR="$DEPLOYROOT/SigDigger.dir"

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
  
  try "Bundling $name..." cp -Rfv "$@" "$LIBPATH"
}

#
# If at some point, SigDigger needs additional libraries, expand this function
# adding the missing dependencies with bundle_libs.
#
# bundle_libs accepts the name of the set of libraries (for informative
# purposes only) and a set of libraries.
#

function deploy_deps()
{
  bundle_libs "SoapySDR libraries" /usr/local/lib/libSoapySDR*dylib
}

function remove_full_paths()
{
  FULLPATH="$2"
  RPATHNAME='@rpath/'`basename "$FULLPATH"`
   
  install_name_tool -change "$FULLPATH" "$RPATHNAME" "$1"
}

function remove_full_path_stdin () {
  while read line; do
    remove_full_paths "$1" "$line"
  done
}

function ensure_rpath()
{
  for i in "$LIBPATH"/*.dylib; do
    if ! [ -L "$i" ]; then
      try "Fixing "`basename $i`"..." true
      otool -L "$i" | grep '\t/usr/local/' | tr -d '\t' | cut -f1 -d ' ' | remove_full_path_stdin "$i";
    fi
  done
}

function create_dmg()
{
  try "Cleaning up old files..." rm -Rfv "$STAGINGDIR"
  try "Creating staging directory..." mkdir -p "$STAGINGDIR"
  try "Copying bundle to staging dir..."   cp -Rfv "$BUNDLEPATH" "$STAGINGDIR"
  try "Creating .dmg file and finishing..." hdiutil create -volname SigDigger -srcfolder "$STAGINGDIR" -ov -format UDZO "$DISTROOT"/SigDigger.dmg
}

function deploy()
{
  locate_macdeploy
  try "Deploying via macdeployqt..." macdeployqt "$BUNDLEPATH"
  try "Copying Suscan data directory to bundle..." cp -Rfv "$DEPLOYROOT/usr/share/suscan" "$RSRCPATH" 
  try "Bundling built libraries..." cp -Rfv "$DEPLOYROOT/usr/lib/"*.dylib "$LIBPATH"
  
  deploy_deps
  ensure_rpath
}

build
deploy
create_dmg




