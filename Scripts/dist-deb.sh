#!/bin/bash
#
#  dist-dist.sh: Deploy SigDigger in a debian package
#
#  Copyright (C) 2023 Ángel Ruiz Fernández
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as
#  published by the Free Software Foundation, version 3.
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

if [ "$#" != "1" ]; then
    echo $0: Usage:
    echo "         $0 version"
    exit 1
fi

PKG_VERSION=$1

shift

# build SigDigger
. dist-common.sh

build

PKG_ARCH=`dpkg --print-architecture`
PKG_DEPENDS='libsuscan (>= 0.3.0-1), libsuwidgets (>= 0.3.0-1)'
PKG_DEV_DEPENDS='libsuscan-dev (>= 0.3.0-1), libsuwidgets-dev (>= 0.3.0-1)'

BINDIR=sigdigger_${PKG_VERSION}_${PKG_ARCH}
DEVDIR=sigdigger-dev_${PKG_VERSION}_${PKG_ARCH}
############################ Binary package ####################################
# create structure
rm -Rf $BINDIR
mkdir $BINDIR
cd $BINDIR
mkdir -p usr/bin/
mkdir -p usr/share/applications/
mkdir -p usr/share/icons/hicolor/256x256/apps/
mkdir -p DEBIAN/

# create debian thing
rm -f DEBIAN/control
cat <<EOF >>DEBIAN/control
Package: sigdigger
Version: $PKG_VERSION
Section: hamradio
Priority: optional
Architecture: $PKG_ARCH
Depends: $PKG_DEPENDS
Maintainer: arf20 <aruizfernandez05@gmail.com>
Description: The Free Digital Signal Analyzer
EOF

# copy files
cp ../SigDigger/SigDigger usr/bin/
cp ../SigDigger/SigDigger.desktop ../SigDigger/RMSViewer.desktop usr/share/applications/
cp ../SigDigger/icons/SigDigger.png usr/share/icons/hicolor/256x256/apps/

# set permissions
cd ..
chmod 755 -R $BINDIR/

# build deb
dpkg-deb --build $BINDIR

############################ Development package ###############################
# create structure
rm -Rf $DEVDIR
mkdir $DEVDIR
cd $DEVDIR
mkdir -p usr/include/x86_64-linux-gnu/qt5/SigDigger/Suscan/Messages/
mkdir -p DEBIAN/

# create debian thing
rm -f DEBIAN/control
cat <<EOF >>DEBIAN/control
Package: sigdigger-dev
Version: $PKG_VERSION
Section: libdevel
Priority: optional
Architecture: $PKG_ARCH
Depends: sigdigger (= $PKG_VERSION), $PKG_DEV_DEPENDS, pkg-config
Maintainer: arf20 <aruizfernandez05@gmail.com>
Description: The Free Digital Signal Analyzer development files
EOF

# copy files
cp ../SigDigger/include/AppConfig.h ../SigDigger/include/Application.h ../SigDigger/include/AppUI.h ../SigDigger/include/AudioFileSaver.h ../SigDigger/include/AudioPlayback.h ../SigDigger/include/Averager.h ../SigDigger/include/ColorConfig.h ../SigDigger/include/ConfigTab.h ../SigDigger/include/FeatureFactory.h ../SigDigger/include/GuiConfig.h ../SigDigger/include/InspectionWidgetFactory.h ../SigDigger/include/SigDiggerHelpers.h ../SigDigger/include/MainSpectrum.h ../SigDigger/include/MainWindow.h ../SigDigger/include/Palette.h ../SigDigger/include/PersistentWidget.h ../SigDigger/include/TabWidgetFactory.h ../SigDigger/include/TLESourceConfig.h ../SigDigger/include/ToolWidgetFactory.h ../SigDigger/include/UIComponentFactory.h ../SigDigger/include/UIListenerFactory.h ../SigDigger/include/UIMediator.h ../SigDigger/include/GenericDataSaver.h ../SigDigger/include/Version.h  usr/include/x86_64-linux-gnu/qt5/SigDigger/
cp ../SigDigger/include/Suscan/AnalyzerRequestTracker.h ../SigDigger/include/Suscan/CancellableTask.h ../SigDigger/include/Suscan/Analyzer.h ../SigDigger/include/Suscan/AnalyzerParams.h ../SigDigger/include/Suscan/Channel.h ../SigDigger/include/Suscan/Compat.h ../SigDigger/include/Suscan/Config.h ../SigDigger/include/Suscan/Estimator.h ../SigDigger/include/Suscan/Library.h ../SigDigger/include/Suscan/Logger.h ../SigDigger/include/Suscan/Message.h ../SigDigger/include/Suscan/MQ.h ../SigDigger/include/Suscan/MultitaskController.h ../SigDigger/include/Suscan/Object.h ../SigDigger/include/Suscan/Plugin.h ../SigDigger/include/Suscan/Serializable.h ../SigDigger/include/Suscan/Source.h ../SigDigger/include/Suscan/SpectrumSource.h usr/include/x86_64-linux-gnu/qt5/SigDigger/Suscan/
cp ../SigDigger/include/Suscan/Messages/ChannelMessage.h ../SigDigger/include/Suscan/Messages/GenericMessage.h ../SigDigger/include/Suscan/Messages/InspectorMessage.h ../SigDigger/include/Suscan/Messages/PSDMessage.h ../SigDigger/include/Suscan/Messages/SamplesMessage.h ../SigDigger/include/Suscan/Messages/SourceInfoMessage.h ../SigDigger/include/Suscan/Messages/StatusMessage.h  usr/include/x86_64-linux-gnu/qt5/SigDigger/Suscan/Messages/

# set permissions
cd ..
chmod 755 -R $DEVDIR

# build deb
dpkg-deb --build $DEVDIR
