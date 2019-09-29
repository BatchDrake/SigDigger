#-------------------------------------------------
#
# Project created by QtCreator 2019-07-05T21:15:25
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SigDigger
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9) {
  QMAKE_CXXFLAGS += -std=gnu++14
} else {
  CONFIG += c++14
}

INCLUDEPATH += $$PWD/include /usr/include/SuWidgets
SOURCES += \
    App/AppConfig.cpp \
    App/Application.cpp \
    App/AppUI.cpp \
    App/ColorConfig.cpp \
    App/Loader.cpp \
    Audio/AudioPlayback.cpp \
    Components/AboutDialog.cpp \
    Components/AudioPanel.cpp \
    Components/ConfigDialog.cpp \
    Components/DataSaverUI.cpp \
    Components/DeviceGain.cpp \
    Components/FftPanel.cpp \
    Components/GainSlider.cpp \
    Components/InspectorPanel.cpp \
    Components/MainSpectrum.cpp \
    Components/MainWindow.cpp \
    Components/PersistentWidget.cpp \
    Components/SaveProfileDialog.cpp \
    Components/SourcePanel.cpp \
    Inspector/Inspector.cpp \
    Inspector/InspectorUI.cpp \
    InspectorCtl/AfcControl.cpp \
    InspectorCtl/AskControl.cpp \
    InspectorCtl/ClockRecovery.cpp \
    InspectorCtl/EqualizerControl.cpp \
    InspectorCtl/GainControl.cpp \
    InspectorCtl/InspectorCtl.cpp \
    InspectorCtl/MfControl.cpp \
    InspectorCtl/ToneControl.cpp \
    Misc/AsyncDataSaver.cpp \
    Misc/AutoGain.cpp \
    Misc/Averager.cpp \
    Misc/Palette.cpp \
    Misc/SNREstimator.cpp \
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
    Suscan/Object.cpp \
    Suscan/Serializable.cpp \
    Suscan/Source.cpp \
    UIMediator/AudioMediator.cpp \
    UIMediator/FftMediator.cpp \
    UIMediator/InspectorMediator.cpp \
    UIMediator/SourceMediator.cpp \
    UIMediator/SpectrumMediator.cpp \
    UIMediator/UIMediator.cpp \
    main.cpp \
    Components/EstimatorControl.cpp \
    Decoder/Decoder.cpp \
    Decoder/DecoderStack.cpp \
    Suscan/DecoderFactory.cpp \
    Decoder/DecoderDialog.cpp \
    Decoder/Builtin/SymbolInverter.cpp \
    Decoder/Builtin/SymbolInverterFactory.cpp \
    Decoder/Builtin/BuiltinDecoderCollection.cpp \
    Decoder/Builtin/ViterbiDecoderUI.cpp \
    Decoder/DecoderTab.cpp \
    Decoder/Builtin/HexTapUI.cpp \
    Decoder/Builtin/HexTap.cpp \
    Decoder/Builtin/HexTapFactory.cpp \
    Decoder/Builtin/SymbolDifferentiator.cpp \
    Decoder/Builtin/SymbolDifferentiatorFactory.cpp \
    Decoder/Builtin/FrameSync.cpp \
    Decoder/Builtin/FrameSyncUI.cpp \
    Decoder/Builtin/FrameSyncFactory.cpp \
    Decoder/Builtin/chopper.c \
    Decoder/Builtin/FACFrameSyncUI.cpp \
    Decoder/Builtin/FACFrameSync.cpp \
    Misc/FACSyncCracker.cpp \
    Decoder/Builtin/FACFrameSyncFactory.cpp


HEADERS += \
    include/Suscan/Messages/ChannelMessage.h \
    include/Suscan/Messages/GenericMessage.h \
    include/Suscan/Messages/InspectorMessage.h \
    include/Suscan/Messages/PSDMessage.h \
    include/Suscan/Messages/SamplesMessage.h \
    include/Suscan/Analyzer.h \
    include/Suscan/AnalyzerParams.h \
    include/Suscan/AutoGain_copy.h \
    include/Suscan/Channel.h \
    include/Suscan/Compat.h \
    include/Suscan/Config.h \
    include/Suscan/Estimator.h \
    include/Suscan/Library.h \
    include/Suscan/Logger.h \
    include/Suscan/Message.h \
    include/Suscan/MQ.h \
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
    include/AsyncDataSaver.h \
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
    include/ToneControl.h \
    include/UIMediator.h \
    include/EstimatorControl.h \
    include/Decoder.h \
    include/DecoderStack.h \
    include/Suscan/DecoderFactory.h \
    include/DecoderDialog.h \
    Decoder/Builtin/ViterbiDecoderUI.h \
    include/DecoderTab.h \
    include/HexTapUI.h \
    include/HexTap.h \
    include/HexTapFactory.h \
    include/SymbolDifferentiator.h \
    include/SymbolDifferentiatorFactory.h \
    include/FrameSync.h \
    include/FrameSyncUI.h \
    include/FrameSyncFactory.h \
    include/chopper.h \
    include/FACFrameSyncUI.h \
    include/FACFrameSync.h \
    include/FACSyncCracker.h \
    include/FACFrameSyncFactory.h
    include/BuiltinDecoderCollection.h


FORMS += \
    ui/AboutDialog.ui \
    ui/AfcControl.ui \
    ui/AskControl.ui \
    ui/AudioPanel.ui \
    ui/ClockRecovery.ui \
    ui/Config.ui \
    ui/DataSaverUI.ui \
    ui/DeviceGain.ui \
    ui/EqualizerControl.ui \
    ui/FftPanel.ui \
    ui/GainControl.ui \
    ui/GainSlider.ui \
    ui/Inspector.ui \
    ui/InspectorPanel.ui \
    ui/MainSpectrum.ui \
    ui/MainWindow.ui \
    ui/MfControl.ui \
    ui/SourcePanel.ui \
    ui/ToneControl.ui \
    ui/SaveProfileDialog.ui \
    ui/EstimatorControl.ui \
    ui/DecoderDialog.ui \
    ui/ViterbiDecoderUI.ui \
    ui/DecoderTab.ui \
    ui/HexTap.ui \
    ui/FrameSyncUI.ui \
    ui/FACFrameSyncUI.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons/Icons.qrc

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += suscan alsa

unix: LIBS += -lsuwidgets

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
    icons/splash.xcf
