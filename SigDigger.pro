#-------------------------------------------------
#
# Project created by QtCreator 2019-07-05T21:15:25
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SigDigger
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9) {
  QMAKE_CXXFLAGS += -std=gnu++14
} else {
  CONFIG += c++14
}

CONFIG(release, debug|release): QMAKE_CXXFLAGS+=-D__FILENAME__=\\\"SigDigger\\\"
CONFIG(debug, debug|release):   QMAKE_CXXFLAGS+=-D__FILENAME__=__FILE__
CONFIG(release, debug|release): QMAKE_LFLAGS+=-s

isEmpty(SUWIDGETS_PREFIX) {
  SUWIDGETS_INSTALL_LIBS=$$[QT_INSTALL_LIBS]
  SUWIDGETS_INSTALL_HEADERS=$$[QT_INSTALL_HEADERS]/SuWidgets
} else {
  SUWIDGETS_INSTALL_LIBS=$$SUWIDGETS_PREFIX/lib
  SUWIDGETS_INSTALL_HEADERS=$$SUWIDGETS_PREFIX/include/SuWidgets
}

isEmpty(PREFIX) {
  PREFIX=/usr/local
}

target.path=$$PREFIX/bin

!isEmpty(PKGVERSION) {
  QMAKE_CXXFLAGS += "-DSIGDIGGER_PKGVERSION='\""$$PKGVERSION"\"'"
}

darwin: ICON = icons/SigDigger.icns
darwin: QMAKE_RPATHDIR += $$SUWIDGETS_INSTALL_LIBS
datwin: QMAKE_RPATHDIR += /usr/local/lib

QMAKE_SUBSTITUTES += SigDigger.desktop.in RMSViewer.desktop.in
desktop.path  = $$PREFIX/share/applications
desktop.files = SigDigger.desktop RMSViewer.desktop
icons.path    = $$PREFIX/share/icons/hicolor/256x256/apps/
icons.files   = icons/SigDigger.png
INSTALLS     += desktop icons

RC_ICONS = sigdigger_logo.ico

INCLUDEPATH += $$PWD/include $$SUWIDGETS_INSTALL_HEADERS
SOURCES += \
    App/AppConfig.cpp \
    App/Application.cpp \
    App/AppUI.cpp \
    App/ColorConfig.cpp \
    App/GuiConfig.cpp \
    App/Loader.cpp \
    App/TLESourceConfig.cpp \
    Audio/AudioFileSaver.cpp \
    Audio/AudioPlayback.cpp \
    Audio/GenericAudioPlayer.cpp \
    Components/AboutDialog.cpp \
    Components/AddTLESourceDialog.cpp \
    Components/AudioPanel.cpp \
    Components/DataSaverUI.cpp \
    Components/DeviceGain.cpp \
    Components/DeviceTweaks.cpp \
    Components/DopplerDialog.cpp \
    Components/FftPanel.cpp \
    Components/FrequencyCorrectionDialog.cpp \
    Components/GainSlider.cpp \
    Components/GenericDataSaverUI.cpp \
    Components/HistogramDialog.cpp \
    Components/InspectorPanel.cpp \
    Components/MainSpectrum.cpp \
    Components/MainWindow.cpp \
    Components/PersistentWidget.cpp \
    Components/QTimeSlider.cpp \
    Components/SamplerDialog.cpp \
    Components/SaveProfileDialog.cpp \
    Components/SourcePanel.cpp \
    Components/TimeWindow.cpp \
    Inspector/FACTab.cpp \
    Inspector/Inspector.cpp \
    Inspector/InspectorUI.cpp \
    Inspector/TVProcessorWorker.cpp \
    InspectorCtl/AfcControl.cpp \
    InspectorCtl/AskControl.cpp \
    InspectorCtl/ClockRecovery.cpp \
    InspectorCtl/EqualizerControl.cpp \
    InspectorCtl/GainControl.cpp \
    InspectorCtl/InspectorCtl.cpp \
    InspectorCtl/MfControl.cpp \
    InspectorCtl/ToneControl.cpp \
    Misc/AutoGain.cpp \
    Misc/Averager.cpp \
    Misc/Palette.cpp \
    Misc/SNREstimator.cpp \
    Misc/SigDiggerHelpers.cpp \
    Settings/ColorConfigTab.cpp \
    Settings/ConfigDialog.cpp \
    Settings/ConfigTab.cpp \
    Settings/GuiConfigTab.cpp \
    Settings/LocationConfigTab.cpp \
    Settings/ProfileConfigTab.cpp \
    Settings/TLESourceTab.cpp \
    Suscan/CancellableTask.cpp \
    Suscan/Messages/ChannelMessage.cpp \
    Suscan/Messages/GenericMessage.cpp \
    Suscan/Messages/InspectorMessage.cpp \
    Suscan/Messages/PSDMessage.cpp \
    Suscan/Messages/SamplesMessage.cpp \
    Suscan/Analyzer.cpp \
    Suscan/AnalyzerParams.cpp \
    Suscan/Config.cpp \
    Suscan/Exception.cpp \
    Suscan/Library.cpp \
    Suscan/Logger.cpp \
    Suscan/Message.cpp \
    Suscan/MQ.cpp \
    Suscan/Messages/SourceInfoMessage.cpp \
    Suscan/Messages/StatusMessage.cpp \
    Suscan/MultitaskController.cpp \
    Suscan/Object.cpp \
    Suscan/Serializable.cpp \
    Suscan/Source.cpp \
    Tasks/CarrierDetector.cpp \
    Tasks/CarrierXlator.cpp \
    Tasks/DopplerCalculator.cpp \
    Tasks/HistogramFeeder.cpp \
    Tasks/WaveSampler.cpp \
    UIMediator/AudioMediator.cpp \
    UIMediator/FftMediator.cpp \
    UIMediator/InspectorMediator.cpp \
    UIMediator/PanoramicDialogMediator.cpp \
    UIMediator/SourceMediator.cpp \
    UIMediator/SpectrumMediator.cpp \
    UIMediator/TimeSliderMediator.cpp \
    UIMediator/UIMediator.cpp \
    main.cpp \
    Components/EstimatorControl.cpp \
    Misc/GenericDataSaver.cpp \
    Misc/FileDataSaver.cpp \
    UDP/SocketForwarder.cpp \
    Components/NetForwarderUI.cpp \
    Components/WaitingSpinnerWidget.cpp \
    Components/DeviceDialog.cpp \
    UIMediator/DeviceDialogMediator.cpp \
    Components/PanoramicDialog.cpp \
    Panoramic/Scanner.cpp \
    Components/RMSViewer.cpp \
    Components/RMSViewTab.cpp \
    Components/RMSViewerSettingsDialog.cpp \
    Components/LogDialog.cpp \
    Inspector/TVProcessorTab.cpp \
    Inspector/SymViewTab.cpp \
    Inspector/WaveformTab.cpp \
    Misc/MultitaskControllerModel.cpp \
    Components/BackgroundTasksDialog.cpp \
    Tasks/ExportSamplesTask.cpp \
    Components/AddBookmarkDialog.cpp \
    Misc/BookmarkTableModel.cpp \
    Components/BookmarkManagerDialog.cpp \
    Misc/TableDelegates.cpp


HEADERS += \
    include/AddTLESourceDialog.h \
    include/AlsaPlayer.h \
    include/AudioFileSaver.h \
    include/CarrierDetector.h \
    include/CarrierXlator.h \
    include/ColorConfigTab.h \
    include/ConfigTab.h \
    include/DeviceTweaks.h \
    include/DopplerCalculator.h \
    include/DopplerDialog.h \
    include/FACTab.h \
    include/FrequencyCorrectionDialog.h \
    include/GenericAudioPlayer.h \
    include/GenericDataSaverUI.h \
    include/GuiConfigTab.h \
    include/HistogramDialog.h \
    include/HistogramFeeder.h \
    include/LocationConfigTab.h \
    include/PortAudioPlayer.h \
    include/ProfileConfigTab.h \
    include/QTimeSlider.h \
    include/SamplerDialog.h \
    include/SamplingProperties.h \
    include/SigDiggerHelpers.h \
    include/Suscan/CancellableTask.h \
    include/Suscan/Messages/ChannelMessage.h \
    include/Suscan/Messages/GenericMessage.h \
    include/Suscan/Messages/InspectorMessage.h \
    include/Suscan/Messages/PSDMessage.h \
    include/Suscan/Messages/SamplesMessage.h \
    include/Suscan/Analyzer.h \
    include/Suscan/AnalyzerParams.h \
    include/Suscan/Channel.h \
    include/Suscan/Compat.h \
    include/Suscan/Config.h \
    include/Suscan/Estimator.h \
    include/Suscan/Library.h \
    include/Suscan/Logger.h \
    include/Suscan/Message.h \
    include/Suscan/MQ.h \
    include/Suscan/Messages/SourceInfoMessage.h \
    include/Suscan/Messages/StatusMessage.h \
    include/Suscan/MultitaskController.h \
    include/Suscan/Object.h \
    include/Suscan/Serializable.h \
    include/Suscan/Source.h \
    include/Suscan/SpectrumSource.h \
    include/AboutDialog.h \
    include/AfcControl.h \
    include/AppConfig.h \
    include/Application.h \
    include/AppUI.h \
    include/AskControl.h \
    include/AudioPanel.h \
    include/AudioPlayback.h \
    include/AutoGain.h \
    include/Averager.h \
    include/ClockRecovery.h \
    include/ColorConfig.h \
    include/ConfigDialog.h \
    include/DataSaverUI.h \
    include/DefaultGradient.h \
    include/DeviceGain.h \
    include/EqualizerControl.h \
    include/FftPanel.h \
    include/GainControl.h \
    include/GainSlider.h \
    include/Inspector.h \
    include/InspectorCtl.h \
    include/InspectorPanel.h \
    include/InspectorUI.h \
    include/Loader.h \
    include/MainSpectrum.h \
    include/MainWindow.h \
    include/MfControl.h \
    include/Palette.h \
    include/PersistentWidget.h \
    include/SaveProfileDialog.h \
    include/SNREstimator.h \
    include/SourcePanel.h \
    include/TLESourceConfig.h \
    include/TLESourceTab.h \
    include/TVProcessorWorker.h \
    include/TimeWindow.h \
    include/ToneControl.h \
    include/UIMediator.h \
    include/EstimatorControl.h \
    include/GenericDataSaver.h \
    include/FileDataSaver.h \
    include/SocketForwarder.h \
    include/NetForwarderUI.h \
    include/Version.h \
    include/WaitingSpinnerWidget.h \
    include/DeviceDialog.h \
    include/PanoramicDialog.h \
    include/Scanner.h \
    include/WaveSampler.h \
    include/RMSViewer.h \
    include/RMSViewTab.h \
    include/RMSViewerSettingsDialog.h \
    include/LogDialog.h \
    include/TVProcessorTab.h \
    include/SymViewTab.h \
    include/WaveformTab.h \
    include/MultitaskControllerModel.h \
    include/BackgroundTasksDialog.h \
    include/ExportSamplesTask.h \
    include/AddBookmarkDialog.h \
    include/BookmarkTableModel.h \
    include/BookmarkManagerDialog.h \
    include/TableDelegates.h


FORMS += \
    ui/AboutDialog.ui \
    ui/AddTLESourceDialog.ui \
    ui/AfcControl.ui \
    ui/AskControl.ui \
    ui/AudioPanel.ui \
    ui/ClockRecovery.ui \
    ui/ColorConfigTab.ui \
    ui/Config.ui \
    ui/DataSaverUI.ui \
    ui/DeviceGain.ui \
    ui/DeviceTweaks.ui \
    ui/DopplerDialog.ui \
    ui/EqualizerControl.ui \
    ui/FACTab.ui \
    ui/FftPanel.ui \
    ui/FrequencyCorrectionDialog.ui \
    ui/GainControl.ui \
    ui/GainSlider.ui \
    ui/GuiConfigTab.ui \
    ui/HistogramDialog.ui \
    ui/Inspector.ui \
    ui/InspectorPanel.ui \
    ui/LocationConfigTab.ui \
    ui/MainSpectrum.ui \
    ui/MainWindow.ui \
    ui/MfControl.ui \
    ui/ProfileConfigTab.ui \
    ui/SamplerDialog.ui \
    ui/SourcePanel.ui \
    ui/TLESourceTab.ui \
    ui/TimeWindow.ui \
    ui/ToneControl.ui \
    ui/SaveProfileDialog.ui \
    ui/EstimatorControl.ui \
    ui/NetForwarderUI.ui \
    ui/DeviceDialog.ui \
    ui/PanoramicDialog.ui \
    ui/RMSViewer.ui \
    ui/RMSViewTab.ui \
    ui/RMSViewerSettingsDialog.ui \
    ui/LogDialog.ui \
    ui/TVProcessorTab.ui \
    ui/SymViewTab.ui \
    ui/WaveformTab.ui \
    ui/BackgroundTasksDialog.ui \
    ui/AddBookmarkDialog.ui \
    ui/BookmarkManagerDialog.ui

!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons/Icons.qrc

CONFIG += link_pkgconfig
PKGCONFIG += suscan fftw3f

packagesExist(libcurl) {
  PKGCONFIG += libcurl
  QMAKE_CXXFLAGS += -DHAVE_CURL
  HEADERS += include/TLEDownloaderTask.h
  SOURCES += Tasks/TLEDownloaderTask.cpp
}

packagesExist(volk) {
  PKGCONFIG += volk
}
  
# Sound API detection. We first check for system-specific audio libraries,
# which tend to be the faster ones. If they are not available, fallback
# to PortAudio.

packagesExist(alsa):!freebsd {
  PKGCONFIG += alsa
  SOURCES += Audio/AlsaPlayer.cpp
  DEFINES += SIGDIGGER_HAVE_ALSA
} else {
  packagesExist(portaudio-2.0) {
    PKGCONFIG += portaudio-2.0
    SOURCES += Audio/PortAudioPlayer.cpp
    DEFINES += SIGDIGGER_HAVE_PORTAUDIO
  }
}

LIBS += -L$$SUWIDGETS_INSTALL_LIBS -lsuwidgets

win32: LIBS += -lwsock32

DISTFILES += \
    icons/icon-alpha.png \
    icons/icon-color-about.png \
    icons/icon-color.png \
    icons/icon-pro.png \
    icons/icon.png \
    icons/overlay-channels.png \
    icons/preferences.png \
    icons/select-source.png \
    icons/splash.png \
    icons/start-capture.png \
    icons/splash.xcf \
    icons/start.png \
    icons/stop.png \
    icons/online.png \
    icons/offline.png
