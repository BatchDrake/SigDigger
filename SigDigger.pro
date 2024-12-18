#-------------------------------------------------
#
# Project created by QtCreator 2019-07-05T21:15:25
#
#-------------------------------------------------

QT           += core gui network widgets opengl
unix: QMAKE_LFLAGS   += -rdynamic
darwin: QMAKE_LFLAGS += -Wl,-export_dynamic

greaterThan(QT_MAJOR_VERSION, 5): QT += openglwidgets

TARGET   = SigDigger
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += lrelease embed_translations

TRANSLATIONS += i18n/SigDigger_en_US.ts \
                i18n/SigDigger_zh_CN.ts

equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9) {
  QMAKE_CXXFLAGS += -std=gnu++17
} else {
  CONFIG += c++1z
}

CONFIG += c++1z

CONFIG(release, debug|release): QMAKE_CXXFLAGS+=-D__FILENAME__=\\\"SigDigger\\\"
CONFIG(debug, debug|release):   QMAKE_CXXFLAGS+=-D__FILENAME__=__FILE__
CONFIG(release, debug|release): QMAKE_LFLAGS+=-s

darwin {
  CONFIG(debug, debug|release): SUWIDGETS_BUILDTYPE_SUFFIX=_debug
}

isEmpty(SUWIDGETS_PREFIX) {
  SUWIDGETS_INSTALL_LIBS=$$[QT_INSTALL_LIBS]
  SUWIDGETS_INSTALL_HEADERS=$$[QT_INSTALL_HEADERS]/SuWidgets
} else {
  SUWIDGETS_INSTALL_LIBS=$$SUWIDGETS_PREFIX/lib
  SUWIDGETS_INSTALL_HEADERS=$$SUWIDGETS_PREFIX/include/SuWidgets
}

isEmpty(PREFIX) {
  PREFIX=/usr/local
  SIGDIGGER_INSTALL_HEADERS=$$[QT_INSTALL_HEADERS]/SigDigger
} else {
  SIGDIGGER_INSTALL_HEADERS=$$PREFIX/include/SigDigger
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
    App/AudioConfig.cpp \
    App/ColorConfig.cpp \
    App/GuiConfig.cpp \
    App/Loader.cpp \
    App/RemoteControlConfig.cpp \
    App/RemoteControlServer.cpp \
    App/TLESourceConfig.cpp \
    Audio/AudioFileSaver.cpp \
    Audio/AudioPlayback.cpp \
    Audio/GenericAudioPlayer.cpp \
    Components/AboutDialog.cpp \
    Components/AddTLESourceDialog.cpp \
    Components/DataSaverUI.cpp \
    Components/DeviceGain.cpp \
    Components/DopplerDialog.cpp \
    Components/FrequencyCorrectionDialog.cpp \
    Components/GainSlider.cpp \
    Components/GenericDataSaverUI.cpp \
    Components/HistogramDialog.cpp \
    Components/MainSpectrum.cpp \
    Components/MainWindow.cpp \
    Components/PersistentWidget.cpp \
    Components/QTimeSlider.cpp \
    Components/QuickConnectDialog.cpp \
    Components/SamplerDialog.cpp \
    Components/SaveProfileDialog.cpp \
    Components/TimeWindow.cpp \
    Default/Audio/AudioProcessor.cpp \
    Default/Audio/AudioWidget.cpp \
    Default/Audio/AudioWidgetFactory.cpp \
    Default/DefaultTab/DefaultTabWidget.cpp \
    Default/DefaultTab/DefaultTabWidgetFactory.cpp \
    Default/FFT/FFTWidget.cpp \
    Default/FFT/FFTWidgetFactory.cpp \
    Default/GenericInspector/FACTab.cpp \
    Default/GenericInspector/GenericInspector.cpp \
    Default/GenericInspector/GenericInspectorFactory.cpp \
    Default/GenericInspector/InspectorCtl/AfcControl.cpp \
    Default/GenericInspector/InspectorCtl/AskControl.cpp \
    Default/GenericInspector/InspectorCtl/ClockRecovery.cpp \
    Default/GenericInspector/InspectorCtl/EqualizerControl.cpp \
    Default/GenericInspector/InspectorCtl/EstimatorControl.cpp \
    Default/GenericInspector/InspectorCtl/GainControl.cpp \
    Default/GenericInspector/InspectorCtl/InspectorCtl.cpp \
    Default/GenericInspector/InspectorCtl/MfControl.cpp \
    Default/GenericInspector/InspectorCtl/ToneControl.cpp \
    Default/GenericInspector/InspectorUI.cpp \
    Default/GenericInspector/SymViewTab.cpp \
    Default/GenericInspector/TVProcessorTab.cpp \
    Default/GenericInspector/TVProcessorWorker.cpp \
    Default/GenericInspector/WaveformTab.cpp \
    Default/Inspection/InspToolWidget.cpp \
    Default/Inspection/InspToolWidgetFactory.cpp \
    Default/RMSInspector/RMSInspector.cpp \
    Default/RMSInspector/RMSInspectorFactory.cpp \
    Default/Registration.cpp \
    Default/Source/SourceWidget.cpp \
    Default/Source/SourceWidgetFactory.cpp \
    Default/SourceConfig/DeviceTweaks.cpp \
    Default/SourceConfig/FileSourcePage.cpp \
    Default/SourceConfig/FileSourcePageFactory.cpp \
    Default/SourceConfig/SoapySDRSourcePage.cpp  \
    Default/SourceConfig/SoapySDRSourcePageFactory.cpp \
    Default/SourceConfig/StdinSourcePage.cpp \
    Default/SourceConfig/StdinSourcePageFactory.cpp \
    Default/SourceConfig/ToneGenSourcePage.cpp \
    Default/SourceConfig/ToneGenSourcePageFactory.cpp \
    Misc/AutoGain.cpp \
    Misc/Averager.cpp \
    Misc/FileViewer.cpp \
    Misc/GlobalProperty.cpp \
    Misc/Palette.cpp \
    Misc/SNREstimator.cpp \
    Misc/SigDiggerHelpers.cpp \
    Settings/AudioConfigTab.cpp \
    Settings/ColorConfigTab.cpp \
    Settings/ConfigDialog.cpp \
    Settings/ConfigTab.cpp \
    Settings/GuiConfigTab.cpp \
    Settings/LocationConfigTab.cpp \
    Settings/ProfileConfigTab.cpp \
    Settings/RemoteControlTab.cpp \
    Settings/TLESourceTab.cpp \
    Suscan/AnalyzerRequestTracker.cpp \
    Suscan/CancellableTask.cpp \
    Suscan/Device.cpp \
    Suscan/FeatureFactory.cpp \
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
    Suscan/Plugin.cpp \
    Suscan/Serializable.cpp \
    Suscan/Source.cpp \
    Tasks/AGCTask.cpp \
    Tasks/CarrierDetector.cpp \
    Tasks/CarrierXlator.cpp \
    Tasks/CostasRecoveryTask.cpp \
    Tasks/DelayedConjTask.cpp \
    Tasks/DopplerCalculator.cpp \
    Tasks/ExportCSVTask.cpp \
    Tasks/HistogramFeeder.cpp \
    Tasks/LPFTask.cpp \
    Tasks/PLLSyncTask.cpp \
    Tasks/QuadDemodTask.cpp \
    Tasks/WaveSampler.cpp \
    UIComponent/InspectionWidgetFactory.cpp \
    UIComponent/SourceConfigWidgetFactory.cpp \
    UIComponent/TabWidgetFactory.cpp \
    UIComponent/ToolWidgetFactory.cpp \
    UIComponent/UIComponentFactory.cpp \
    UIComponent/UIListenerFactory.cpp \
    UIMediator/FloatingTabWindow.cpp \
    UIMediator/InspectorMediator.cpp \
    UIMediator/PanoramicDialogMediator.cpp \
    UIMediator/SpectrumMediator.cpp \
    UIMediator/TimeSliderMediator.cpp \
    UIMediator/UIMediator.cpp \
    main.cpp \
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
    Misc/MultitaskControllerModel.cpp \
    Components/BackgroundTasksDialog.cpp \
    Tasks/ExportSamplesTask.cpp \
    Components/AddBookmarkDialog.cpp \
    Misc/BookmarkTableModel.cpp \
    Components/BookmarkManagerDialog.cpp \
    Misc/TableDelegates.cpp

INSTALL_HEADERS += \
    include/AppConfig.h \
    include/Application.h \
    include/AppUI.h \
    include/AudioConfig.h \
    include/AudioFileSaver.h \
    include/AudioPlayback.h \
    include/Averager.h \
    include/ColorConfig.h \
    include/ConfigTab.h \
    include/FeatureFactory.h \
    include/GuiConfig.h \
    include/GlobalProperty.h \
    include/InspectionWidgetFactory.h \
    include/SigDiggerHelpers.h \
    include/MainSpectrum.h \
    include/MainWindow.h \
    include/Palette.h \
    include/PersistentWidget.h \
    include/RemoteControlConfig.h \
    include/SourceConfigWidgetFactory.h \
    include/TabWidgetFactory.h \
    include/TLESourceConfig.h \
    include/ToolWidgetFactory.h \
    include/UIComponentFactory.h \
    include/UIListenerFactory.h \
    include/UIMediator.h \
    include/GenericDataSaver.h \
    include/Version.h

install_headers.path   = $$SIGDIGGER_INSTALL_HEADERS
install_headers.files += $$INSTALL_HEADERS
INSTALLS              += install_headers

SUSCAN_HEADERS += \
    include/Suscan/AnalyzerRequestTracker.h \
    include/Suscan/CancellableTask.h \
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
    include/Suscan/MultitaskController.h \
    include/Suscan/Object.h \
    include/Suscan/Plugin.h \
    include/Suscan/Serializable.h \
    include/Suscan/Source.h \
    include/Suscan/SpectrumSource.h

suscan_headers.path   = $$SIGDIGGER_INSTALL_HEADERS/Suscan
suscan_headers.files += $$SUSCAN_HEADERS
INSTALLS             += suscan_headers

SUSCAN_MSG_HEADERS += \
    include/Suscan/Messages/ChannelMessage.h \
    include/Suscan/Messages/GenericMessage.h \
    include/Suscan/Messages/InspectorMessage.h \
    include/Suscan/Messages/PSDMessage.h \
    include/Suscan/Messages/SamplesMessage.h \
    include/Suscan/Messages/SourceInfoMessage.h \
    include/Suscan/Messages/StatusMessage.h

suscan_msg_headers.path   = $$SIGDIGGER_INSTALL_HEADERS/Suscan/Messages
suscan_msg_headers.files += $$SUSCAN_MSG_HEADERS
INSTALLS                 += suscan_msg_headers

HEADERS += \
    $$INSTALL_HEADERS \
    $$SUSCAN_HEADERS \
    $$SUSCAN_MSG_HEADERS \
    Default/Audio/AudioProcessor.h \
    Default/Audio/AudioWidget.h \
    Default/Audio/AudioWidgetFactory.h \
    Default/DefaultTab/DefaultTabWidget.h \
    Default/DefaultTab/DefaultTabWidgetFactory.h \
    Default/FFT/FFTWidget.h \
    Default/FFT/FFTWidgetFactory.h \
    Default/GenericInspector/FACTab.h \
    Default/GenericInspector/GenericInspector.h \
    Default/GenericInspector/GenericInspectorFactory.h \
    Default/GenericInspector/InspectorCtl/AfcControl.h \
    Default/GenericInspector/InspectorCtl/AskControl.h \
    Default/GenericInspector/InspectorCtl/ClockRecovery.h \
    Default/GenericInspector/InspectorCtl/EqualizerControl.h \
    Default/GenericInspector/InspectorCtl/EstimatorControl.h \
    Default/GenericInspector/InspectorCtl/GainControl.h \
    Default/GenericInspector/InspectorCtl/InspectorCtl.h \
    Default/GenericInspector/InspectorCtl/MfControl.h \
    Default/GenericInspector/InspectorCtl/ToneControl.h \
    Default/GenericInspector/InspectorUI.h \
    Default/GenericInspector/SymViewTab.h \
    Default/GenericInspector/TVProcessorTab.h \
    Default/GenericInspector/TVProcessorWorker.h \
    Default/GenericInspector/WaveformTab.h \
    Default/Inspection/InspToolWidget.h \
    Default/Inspection/InspToolWidgetFactory.h \
    Default/RMSInspector/RMSInspector.h \
    Default/RMSInspector/RMSInspectorFactory.h \
    Default/Registration.h \
    Default/Source/SourceWidget.h \
    Default/Source/SourceWidgetFactory.h \
    Default/SourceConfig/DeviceTweaks.h \
    Default/SourceConfig/FileSourcePage.h \
    Default/SourceConfig/FileSourcePageFactory.h \
    Default/SourceConfig/SoapySDRSourcePage.h \
    Default/SourceConfig/SoapySDRSourcePageFactory.h \
    Default/SourceConfig/StdinSourcePage.h \
    Default/SourceConfig/StdinSourcePageFactory.h \
    Default/SourceConfig/ToneGenSourcePage.h \
    Default/SourceConfig/ToneGenSourcePageFactory.h \
    ExportCSVTask.h \
    include/AGCTask.h \
    include/AddTLESourceDialog.h \
    include/AlsaPlayer.h \
    include/AudioConfig.h \
    include/AudioConfigTab.h \
    include/CarrierDetector.h \
    include/CarrierXlator.h \
    include/ColorConfigTab.h \
    include/CostasRecoveryTask.h \
    include/DelayedConjTask.h \
    include/DopplerCalculator.h \
    include/DopplerDialog.h \
    include/FileViewer.h \
    include/FloatingTabWindow.h \
    include/FrequencyCorrectionDialog.h \
    include/GenericAudioPlayer.h \
    include/GenericDataSaverUI.h \
    include/GuiConfigTab.h \
    include/HistogramDialog.h \
    include/HistogramFeeder.h \
    include/LPFTask.h \
    include/LocationConfigTab.h \
    include/PLLSyncTask.h \
    include/PortAudioPlayer.h \
    include/ProfileConfigTab.h \
    include/QTimeSlider.h \
    include/QuadDemodTask.h \
    include/QuickConnectDialog.h \
    include/RemoteControlServer.h \
    include/RemoteControlTab.h \
    include/SamplerDialog.h \
    include/SamplingProperties.h \
    include/AboutDialog.h \
    include/AutoGain.h \
    include/ConfigDialog.h \
    include/DataSaverUI.h \
    include/DefaultGradient.h \
    include/DeviceGain.h \
    include/GainSlider.h \
    include/Loader.h \
    include/SaveProfileDialog.h \
    include/SNREstimator.h \
    include/Suscan/Device.h \
    include/TLESourceTab.h \
    include/TimeWindow.h \
    include/FileDataSaver.h \
    include/SocketForwarder.h \
    include/NetForwarderUI.h \
    include/WaitingSpinnerWidget.h \
    include/DeviceDialog.h \
    include/PanoramicDialog.h \
    include/Scanner.h \
    include/WaveSampler.h \
    include/RMSViewer.h \
    include/RMSViewTab.h \
    include/RMSViewerSettingsDialog.h \
    include/LogDialog.h \
    include/MultitaskControllerModel.h \
    include/BackgroundTasksDialog.h \
    include/ExportSamplesTask.h \
    include/AddBookmarkDialog.h \
    include/BookmarkTableModel.h \
    include/BookmarkManagerDialog.h \
    include/TableDelegates.h


FORMS += \
    Default/Audio/AudioWidget.ui \
    Default/DefaultTab/DefaultTabWidget.ui \
    Default/FFT/FFTWidget.ui \
    Default/GenericInspector/FACTab.ui \
    Default/GenericInspector/GenericInspector.ui \
    Default/GenericInspector/SymViewTab.ui \
    Default/GenericInspector/TVProcessorTab.ui \
    Default/GenericInspector/WaveformTab.ui \
    Default/Inspection/InspToolWidget.ui \
    Default/RMSInspector/RMSInspector.ui \
    Default/Source/SourceWidget.ui \
    Default/SourceConfig/DeviceTweaks.ui \
    Default/SourceConfig/FileSourcePage.ui \
    Default/SourceConfig/SoapySDRSourcePage.ui \
    Default/SourceConfig/StdinSourcePage.ui \
    Default/SourceConfig/ToneGenSourcePage.ui \
    ui/AboutDialog.ui \
    ui/AddTLESourceDialog.ui \
    ui/AfcControl.ui \
    ui/AskControl.ui \
    ui/AudioConfigTab.ui \
    ui/ClockRecovery.ui \
    ui/ColorConfigTab.ui \
    ui/Config.ui \
    ui/DataSaverUI.ui \
    ui/DeviceGain.ui \
    ui/DopplerDialog.ui \
    ui/EqualizerControl.ui \
    ui/FloatingTabWindow.ui \
    ui/FrequencyCorrectionDialog.ui \
    ui/GainControl.ui \
    ui/GainSlider.ui \
    ui/GuiConfigTab.ui \
    ui/HistogramDialog.ui \
    ui/LocationConfigTab.ui \
    ui/MainSpectrum.ui \
    ui/MainWindow.ui \
    ui/MfControl.ui \
    ui/ProfileConfigTab.ui \
    ui/QuickConnectDialog.ui \
    ui/RemoteControlTab.ui \
    ui/SamplerDialog.ui \
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


isEmpty(DISABLE_ALSA): packagesExist(alsa): ALSA_FOUND = Yes
isEmpty(DISABLE_PORTAUDIO): packagesExist(portaudio-2.0): PORTAUDIO_FOUND = Yes

!isEmpty(ALSA_FOUND):!freebsd {
  message(Note: using ALSA libraries for audio support)
  PKGCONFIG += alsa
  SOURCES += Audio/AlsaPlayer.cpp
  DEFINES += SIGDIGGER_HAVE_ALSA
} else {
  !isEmpty(PORTAUDIO_FOUND) {
    message(Note: using PortAudio libraries for audio support)
    PKGCONFIG += portaudio-2.0
    SOURCES += Audio/PortAudioPlayer.cpp
    DEFINES += SIGDIGGER_HAVE_PORTAUDIO
  } else {
    message(Note: audio support is disabled)
  }
}

LIBS += -L$$SUWIDGETS_INSTALL_LIBS


win32 {
  LIBS += -lwsock32 -lsuwidgets0
} else {
  LIBS += -lsuwidgets$$SUWIDGETS_BUILDTYPE_SUFFIX -ldl
}

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
