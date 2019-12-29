//
//    UIMediator.cpp: User Interface mediator object
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this program.  If not, see
//    <http://www.gnu.org/licenses/>
//

#include <Suscan/Library.h>
#include <QFileDialog>
#include <sys/statvfs.h>

#include "UIMediator.h"

#include <QGuiApplication>
#include <QDockWidget>
#include <QMessageBox>
#include <QScreen>

#include <fstream>

using namespace SigDigger;

void
UIMediator::refreshUI(void)
{
  QString stateString;

  switch (this->state) {
    case HALTED:
      stateString = QString("Idle");
      this->ui->spectrum->setCaptureMode(MainSpectrum::UNAVAILABLE);
      this->setProcessRate(0);
      this->ui->main->actionRun->setEnabled(true);
      this->ui->main->actionRun->setChecked(false);
      this->ui->main->actionStart_capture->setEnabled(true);
      this->ui->main->actionStop_capture->setEnabled(false);
      break;

    case HALTING:
      stateString = QString("Halting...");
      this->ui->main->actionRun->setEnabled(false);
      this->ui->main->actionStart_capture->setEnabled(false);
      this->ui->main->actionStop_capture->setEnabled(false);
      break;

    case RUNNING:
      stateString = QString("Running");
      if (this->appConfig->profile.getType() == SUSCAN_SOURCE_TYPE_SDR)
        this->ui->spectrum->setCaptureMode(MainSpectrum::CAPTURE);
      else
        this->ui->spectrum->setCaptureMode(MainSpectrum::REPLAY);

      this->ui->main->actionRun->setEnabled(true);
      this->ui->main->actionRun->setChecked(true);
      this->ui->main->actionStart_capture->setEnabled(false);
      this->ui->main->actionStop_capture->setEnabled(true);
      break;

    case RESTARTING:
      stateString = QString("Restarting...");
      this->ui->main->actionRun->setEnabled(false);
      this->ui->main->actionStart_capture->setEnabled(false);
      this->ui->main->actionStop_capture->setEnabled(false);
      break;
  }

  this->owner->setWindowTitle(
        "SigDigger - "
        + QString::fromStdString(this->getProfile()->label())
        + " - " + stateString);
}

void
UIMediator::onTriggerStart(bool)
{
  emit captureStart();
}

void
UIMediator::onTriggerStop(bool)
{
  emit captureEnd();
}

void
UIMediator::onTriggerImport(bool)
{
  QString path = QFileDialog::getOpenFileName(
        this,
        "Import SigDigger profile",
        QString(),
        "SigDigger profile files (*.xml)");

  if (!path.isEmpty()) {
    std::ifstream in(
          path.toStdString().c_str(),
          std::ifstream::ate | std::ifstream::binary);

    if (!in.is_open()) {
      QMessageBox::warning(
            this->ui->main->centralWidget,
            "Cannot open file",
            "Selected profile file could not be opened. Please check its "
            "access rights or whether the file still exists and try again.",
            QMessageBox::Ok);
    } else if (in.tellg() > SIGDIGGER_PROFILE_FILE_MAX_SIZE) {
        QMessageBox::critical(
              this->ui->main->centralWidget,
              "Cannot open file",
              "Invalid profile file",
              QMessageBox::Ok);
    } else {
      std::vector<char> data;
      data.resize(static_cast<size_t>(in.tellg()));
      in.seekg(0);
      in.read(&data[0], static_cast<int>(data.size()));

      if (in.eof()) {
        QMessageBox::critical(
              this->ui->main->centralWidget,
              "Cannot open file",
              "Unexpected end of file",
              QMessageBox::Ok);
      } else {
        try {
          Suscan::Object envelope(
                path.toStdString(),
                reinterpret_cast<const uint8_t *>(data.data()),
                data.size());
          Suscan::Object profObj = envelope[0];
          QString className = QString::fromStdString(profObj.getClass());

          if (className.isEmpty()) {
            QMessageBox::warning(
                  this->ui->main->centralWidget,
                  "Cannot open file",
                  "Invalid file: serialized object class is undefined",
                  QMessageBox::Ok);
          } else if (className != "source_config") {
            QMessageBox::warning(
                  this->ui->main->centralWidget,
                  "Cannot open file",
                  "Invalid file: objects of class `" + className + "' are "
                  "not profiles.",
                  QMessageBox::Ok);
          } else {
            try {
              Suscan::Source::Config config(profObj);
              this->appConfig->profile = config;
              this->refreshProfile();
              this->refreshUI();
              emit profileChanged();
            } catch (Suscan::Exception &e) {
              QMessageBox::critical(
                    this->ui->main->centralWidget,
                    "Cannot open file",
                    "Failed to create source configuration from profile object: "
                    + QString(e.what()),
                    QMessageBox::Ok);
            }
          }
        } catch (Suscan::Exception &e) {
          QMessageBox::critical(
                this->ui->main->centralWidget,
                "Cannot open file",
                "Failed to load profile object from file: " + QString(e.what()),
                QMessageBox::Ok);
        }
      }
    }
  }
}

void
UIMediator::onTriggerExport(bool)
{
  bool done = false;

  do {
    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::FileMode::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setWindowTitle(QString("Save current profile"));
    dialog.setNameFilter(QString("SigDigger profile files (*.xml)"));

    if (dialog.exec()) {
      QString path = dialog.selectedFiles().first();
      try {
        Suscan::Object profileObj = this->getProfile()->serialize();
        Suscan::Object envelope(SUSCAN_OBJECT_TYPE_SET);

        envelope.append(profileObj);

        std::vector<char> data = envelope.serialize();
        std::ofstream of(path.toStdString().c_str(), std::ofstream::binary);

        if (!of.is_open()) {
          QMessageBox::warning(
                this->ui->main->centralWidget,
                "Cannot open file",
                "Cannote save file in the specified location. Please choose "
                "a different location and try again.",
                QMessageBox::Ok);
        } else {
          of.write(&data[0], static_cast<int>(data.size()));

          if (of.fail()) {
            QMessageBox::critical(
                  this->ui->main->centralWidget,
                  "Cannot serialize profile",
                  "Write error. Profile has not been saved. Please try again.",
                  QMessageBox::Ok);
          } else {
            done = true;
          }
        }
      } catch (Suscan::Exception &e) {
        QMessageBox::critical(
              this->ui->main->centralWidget,
              "Cannot serialize profile",
              "Serialization of profile data failed: " + QString(e.what()),
              QMessageBox::Ok);
      }
    } else {
      done = true;
    }
  } while (!done);
}

void
UIMediator::onTriggerDevices(bool)
{

}

void
UIMediator::onTriggerQuit(bool)
{
  emit uiQuit();
}

void
UIMediator::connectMainWindow(void)
{
  connect(
        this->ui->main->actionSetup,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerSetup(bool)));

  connect(
        this->ui->main->actionOptions,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerSetup(bool)));

  connect(
        this->ui->main->actionImport_profile,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerImport(bool)));

  connect(
        this->ui->main->actionExport_profile,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerExport(bool)));

  connect(
        this->ui->main->actionDevices,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerDevices(bool)));

  connect(
        this->ui->main->actionStart_capture,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerStart(bool)));

  connect(
        this->ui->main->actionStop_capture,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerStop(bool)));

  connect(
        this->ui->main->actionQuit,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerQuit(bool)));

  connect(
        this->ui->main->actionRun,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleCapture(bool)));

  connect(
        this->ui->main->actionAbout,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleAbout(bool)));

  connect(
        this->ui->main->mainTab,
        SIGNAL(tabCloseRequested(int)),
        this,
        SLOT(onCloseInspectorTab(int)));
}

UIMediator::UIMediator(QMainWindow *owner, AppUI *ui)
{
  this->owner = owner;
  this->ui = ui;

  // Configure audio preview
  this->audioPanelDock = new QDockWidget("Audio preview", owner);
  this->audioPanelDock->setWidget(this->ui->audioPanel);
  owner->addDockWidget(Qt::RightDockWidgetArea, this->audioPanelDock);

  // Configure Source Panel
  this->sourcePanelDock = new QDockWidget("Source", owner);
  this->sourcePanelDock->setWidget(this->ui->sourcePanel);
  owner->addDockWidget(Qt::RightDockWidgetArea, this->sourcePanelDock);

  // Configure Inspector Panel
  this->inspectorPanelDock = new QDockWidget("Inspection", owner);
  this->inspectorPanelDock->setWidget(this->ui->inspectorPanel);
  owner->addDockWidget(Qt::RightDockWidgetArea, this->inspectorPanelDock);

  // Configure FFT
  this->fftPanelDock = new QDockWidget("FFT", owner);
  this->fftPanelDock->setWidget(this->ui->fftPanel);
  owner->addDockWidget(Qt::RightDockWidgetArea, this->fftPanelDock);

  // Add baseband analyzer tab
  this->ui->main->mainTab->addTab(this->ui->spectrum, "Radio spectrum");

  // Sort panels
  owner->tabifyDockWidget(this->sourcePanelDock, this->inspectorPanelDock);
  owner->tabifyDockWidget(this->inspectorPanelDock, this->fftPanelDock);

  this->sourcePanelDock->raise();

  // Configure main spectrum
  this->ui->spectrum->setPaletteGradient(
        this->ui->fftPanel->getPaletteGradient());

  this->connectMainWindow();
  this->connectSpectrum();
  this->connectSourcePanel();
  this->connectFftPanel();
  this->connectAudioPanel();
  this->connectInspectorPanel();
}

void
UIMediator::setBandwidth(unsigned int bw)
{
  this->ui->spectrum->setFilterBandwidth(bw);
  this->ui->inspectorPanel->setBandwidth(bw);
}

void
UIMediator::setProcessRate(unsigned int rate)
{
  this->ui->sourcePanel->setProcessRate(rate);
}

void
UIMediator::setSampleRate(unsigned int rate)
{
  if (this->rate != rate) {
    unsigned int bw = rate / 30;

    SUFREQ audioBw = SIGDIGGER_AUDIO_INSPECTOR_BANDWIDTH;

    if (audioBw > rate / 2)
      audioBw = rate / 2;

    this->ui->fftPanel->setSampleRate(rate);
    this->ui->inspectorPanel->setBandwidthLimits(0, rate / 2);
    this->ui->spectrum->setSampleRate(rate);
    this->ui->sourcePanel->setSampleRate(rate);
    this->ui->inspectorPanel->setBandwidth(bw);
    this->ui->spectrum->setFilterBandwidth(bw);

    this->ui->audioPanel->setBandwidth(static_cast<float>(audioBw));
    this->rate = rate;
  }
}

void
UIMediator::setState(State state)
{
  this->state = state;
  this->refreshUI();
}

UIMediator::State
UIMediator::getState(void) const
{
  return this->state;
}

void
UIMediator::feedPSD(const Suscan::PSDMessage &msg)
{
  this->setSampleRate(msg.getSampleRate());
  this->averager.feed(msg);
  this->ui->spectrum->feed(
        this->averager.get(),
        static_cast<int>(this->averager.size()));
}

void
UIMediator::setCaptureSize(quint64 size)
{
  this->ui->sourcePanel->setCaptureSize(size);
}

Inspector *
UIMediator::lookupInspector(Suscan::InspectorId handle) const
{
  Inspector *entry = nullptr;

  try {
    entry = this->ui->inspectorTable.at(handle);
  } catch (std::out_of_range &) { }

  return entry;
}

QString
UIMediator::getInspectorTabTitle(Suscan::InspectorMessage const &msg)
{
  QString result = " in " + QString::number(msg.getChannel().fc) + " Hz";

  if (msg.getClass() == "psk")
    return "PSK inspector" + result;
  else if (msg.getClass() == "fsk")
    return "FSK inspector" + result;
  else if (msg.getClass() == "ask")
    return "ASK inspector" + result;

  return "Generic inspector" + result;
}

Inspector *
UIMediator::addInspectorTab(
    Suscan::InspectorMessage const &msg,
    Suscan::InspectorId &oId)
{

  int index;
  Inspector *insp = new Inspector(
        this->ui->main->mainTab,
        msg,
        *this->appConfig);
  oId = this->ui->lastId++;
  insp->setId(oId);

  index = this->ui->main->mainTab->addTab(
        insp,
        UIMediator::getInspectorTabTitle(msg));

  this->ui->inspectorTable[oId] = insp;
  this->ui->main->mainTab->setCurrentIndex(index);

  return insp;
}

void
UIMediator::detachAllInspectors()
{
  for (auto p = this->ui->inspectorTable.begin();
       p != this->ui->inspectorTable.end();
       ++p) {
    p->second->setAnalyzer(nullptr);
    p->second = nullptr;
  }
}

void
UIMediator::setRecordState(bool state)
{
  this->ui->sourcePanel->setRecordState(state);
}

void
UIMediator::setIORate(qreal rate)
{
  this->ui->sourcePanel->setIORate(rate);
}

void
UIMediator::refreshProfile(void)
{
  this->ui->sourcePanel->setProfile(&this->appConfig->profile);
  this->ui->configDialog->setProfile(this->appConfig->profile);
  this->ui->spectrum->setCenterFreq(
        static_cast<qint64>(this->appConfig->profile.getFreq()));
  this->ui->spectrum->setLnbFreq(
        static_cast<qint64>(this->appConfig->profile.getLnbFreq()));

  this->setSampleRate(this->appConfig->profile.getSampleRate());
}

Suscan::Source::Config *
UIMediator::getProfile(void) const
{
  return &this->appConfig->profile;
}

Suscan::AnalyzerParams *
UIMediator::getAnalyzerParams(void) const
{
  return &this->appConfig->analyzerParams;
}

unsigned int
UIMediator::getFftSize(void) const
{
  return this->appConfig->analyzerParams.windowSize;
}

Suscan::Serializable *
UIMediator::allocConfig(void)
{
  return this->appConfig = new AppConfig(this->ui);
}

void
UIMediator::saveGeometry(void)
{
  this->appConfig->x = this->owner->geometry().x();
  this->appConfig->y = this->owner->geometry().y();
  this->appConfig->width  = this->owner->geometry().width();
  this->appConfig->height = this->owner->geometry().height();
}

void
UIMediator::applyConfig(void)
{
  // Apply window config
  QRect rec = QGuiApplication::primaryScreen()->geometry();
  unsigned int savedBw = this->appConfig->bandwidth;

  if (this->appConfig->x == -1)
    this->appConfig->x = (rec.width() - this->appConfig->width) / 2;
  if (this->appConfig->y == -1)
    this->appConfig->y = (rec.height() - this->appConfig->height) / 2;

  this->owner->setGeometry(
        this->appConfig->x,
        this->appConfig->y,
        this->appConfig->width,
        this->appConfig->height);

  // The following controls reflect elements of the configuration that are
  // not owned by them. We need to set them manually.
  this->ui->configDialog->setColors(this->appConfig->colors);
  this->ui->spectrum->setColorConfig(this->appConfig->colors);
  this->ui->fftPanel->setWindowFunction(this->appConfig->analyzerParams.windowFunction);
  this->ui->fftPanel->setFftSize(this->appConfig->analyzerParams.windowSize);  
  this->ui->fftPanel->setRefreshRate(
        static_cast<unsigned int>(1.f / this->appConfig->analyzerParams.psdUpdateInterval));
  this->ui->fftPanel->setDefaultFftSize(SIGDIGGER_FFT_WINDOW_SIZE);
  this->ui->fftPanel->setDefaultRefreshRate(SIGDIGGER_FFT_REFRESH_RATE);

  // The rest of them are automatically deserialized
  this->ui->sourcePanel->applyConfig();
  this->ui->fftPanel->applyConfig();
  this->ui->inspectorPanel->applyConfig();
  this->ui->audioPanel->applyConfig();

  this->refreshProfile();

  // Apply loFreq and bandwidth config AFTER profile has been set.
  this->ui->spectrum->setLoFreq(this->appConfig->loFreq);
  if (savedBw > 0)
    this->setBandwidth(savedBw);

  // Artificially trigger slots to synchronize UI
  this->onPaletteChanged();
  this->onRangesChanged();
  this->onAveragerChanged();
  this->onThrottleConfigChanged();
  this->onTimeSpanChanged();
}

UIMediator::~UIMediator()
{

}

/////////////////////////////// Slots //////////////////////////////////////////
void
UIMediator::onTriggerSetup(bool)
{
  this->ui->configDialog->setProfile(*this->getProfile());
  this->ui->configDialog->setAnalyzerParams(*this->getAnalyzerParams());

  if (this->ui->configDialog->run()) {
    this->appConfig->profile = this->ui->configDialog->getProfile();

    this->appConfig->analyzerParams = this->ui->configDialog->getAnalyzerParams();
    this->ui->fftPanel->setFftSize(this->getFftSize());
    this->refreshProfile();

    this->appConfig->colors = this->ui->configDialog->getColors();
    this->ui->spectrum->setColorConfig(this->appConfig->colors);
    this->refreshUI();

    emit profileChanged();
  }
}

void
UIMediator::onToggleCapture(bool state)
{
  if (state) {
    emit captureStart();
  } else {
    emit captureEnd();
  }
}

void
UIMediator::onToggleAbout(bool)
{
  this->ui->aboutDialog->exec();
}

void
UIMediator::closeInspectorTab(Inspector *insp)
{
  if (insp != nullptr) {
    Suscan::Analyzer *analyzer = insp->getAnalyzer();
    if (analyzer != nullptr) {
      analyzer->closeInspector(insp->getHandle(), 0);
    } else {
      this->ui->main->mainTab->removeTab(
            this->ui->main->mainTab->indexOf(insp));
      this->ui->inspectorTable.erase(insp->getId());
      delete insp;
    }
  }
}

void
UIMediator::onCloseInspectorTab(int ndx)
{
  QWidget *widget = this->ui->main->mainTab->widget(ndx);

  if (widget != nullptr && widget != this->ui->spectrum) {
    Inspector *insp = static_cast<Inspector *>(widget);
    this->closeInspectorTab(insp);
  }
}

