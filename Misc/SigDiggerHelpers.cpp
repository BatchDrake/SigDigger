//
//    SigDiggerHelpers.cpp: Various helping functions
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#include "SigDiggerHelpers.h"
#include "DefaultGradient.h"
#include "Version.h"
#include <QComboBox>
#include <fstream>
#include <QMessageBox>
#include <QFileDialog>
#include <SuWidgetsHelpers.h>
#include <Suscan/MultitaskController.h>
#include <ExportSamplesTask.h>
#include <util/compat-stdlib.h>

#ifndef SIGDIGGER_PKGVERSION
#  define SIGDIGGER_PKGVERSION \
  "custom build on " __DATE__ " at " __TIME__ " (" __VERSION__ ")"
#endif /* SUSCAN_BUILD_STRING */

using namespace SigDigger;

SigDiggerHelpers *SigDiggerHelpers::currInstance = nullptr;

SigDiggerHelpers *
SigDiggerHelpers::instance(void)
{
  if (currInstance == nullptr)
    currInstance = new SigDiggerHelpers();

  return currInstance;
}

QString
SigDiggerHelpers::version(void)
{
  return QString(SIGDIGGER_VERSION_STRING);
}

QString
SigDiggerHelpers::pkgversion(void)
{
  return QString(SIGDIGGER_PKGVERSION);
}

void
SigDiggerHelpers::timerdup(struct timeval *tv)
{
  tv->tv_sec  <<= 1;
  tv->tv_usec <<= 1;
  if (tv->tv_usec >= 1000000) {
    tv->tv_sec  += 1;
    tv->tv_usec -= 1000000;
  }
}

AudioDemod
SigDiggerHelpers::strToDemod(std::string const &str)
{
  if (str == "AM")
    return AudioDemod::AM;
  else if (str == "FM")
    return AudioDemod::FM;
  else if (str == "USB")
    return AudioDemod::USB;
  else if (str == "LSB")
    return AudioDemod::LSB;

  return AudioDemod::AM;
}

std::string
SigDiggerHelpers::demodToStr(AudioDemod demod)
{
  switch (demod) {
    case AM:
      return "AM";

    case FM:
      return "FM";

    case USB:
      return "USB";

    case LSB:
      return "LSB";
  }

  return "AM"; // Default
}

void
SigDiggerHelpers::openSaveSamplesDialog(
    QWidget *root,
    const SUCOMPLEX *data,
    size_t length,
    qreal fs,
    int start,
    int end,
    Suscan::MultitaskController *mt)
{
  bool done = false;

  do {
    QFileDialog dialog(root);
    QStringList filters;
    QString format;

    dialog.setFileMode(QFileDialog::FileMode::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setWindowTitle(QString("Save capture"));

    filters << "Audio file (*.wav)"
            << "Raw I/Q data (*.raw)"
            << "MATLAB/Octave script (*.m)"
            << "MATLAB 5.0 MAT-file (*.mat)";

    dialog.setNameFilters(filters);

    if (dialog.exec()) {
      QString path = dialog.selectedFiles().first();
      QString filter = dialog.selectedNameFilter();
      ExportSamplesTask *task;

      if (strstr(filter.toStdString().c_str(), ".mat") != nullptr)
        format = "mat";
      else if (strstr(filter.toStdString().c_str(), ".m") != nullptr)
        format = "m";
      else if (strstr(filter.toStdString().c_str(), ".raw") != nullptr)
        format = "raw";
      else
        format = "wav";

      path = SuWidgetsHelpers::ensureExtension(path, format);

      task = new ExportSamplesTask(path, format, data, length, fs, start, end);

      if (!task->attemptOpen()) {
        QMessageBox::critical(
              root,
              task->getLastError(),
              "Save samples to file");
        delete task;
      } else {
        QFileInfo info(path);

        // TODO: Decide whether to send to multitask controller or to
        // run in the current thread according to data size.

        mt->pushTask(task, "Save samples to " + info.fileName());
        done = true;
      }
    } else {
      done = true;
    }
  } while (!done);
}


Palette *
SigDiggerHelpers::getGqrxPalette(void)
{
  static qreal color[256][3];
  if (this->gqrxPalette == nullptr) {
    for (int i = 0; i < 256; i++) {
      if (i < 20) { // level 0: black background
        color[i][0] = color[i][1] = color[i][2] = 0;
      } else if ((i >= 20) && (i < 70)) { // level 1: black -> blue
        color[i][0] = color[i][1] = 0;
        color[i][2] = (140*(i-20)/50) / 255.;
      } else if ((i >= 70) && (i < 100)) { // level 2: blue -> light-blue / greenish
        color[i][0] = (60*(i-70)/30) / 255.;
        color[i][1] = (125*(i-70)/30) / 255.;
        color[i][2] = (115*(i-70)/30 + 140) / 255.;
      } else if ((i >= 100) && (i < 150)) { // level 3: light blue -> yellow
        color[i][0] = (195*(i-100)/50 + 60) / 255.;
        color[i][1] = (130*(i-100)/50 + 125) / 255.;
        color[i][2] = (255-(255*(i-100)/50)) / 255.;
      } else if ((i >= 150) && (i < 250)) { // level 4: yellow -> red
        color[i][0] = 1;
        color[i][1] = (255-255*(i-150)/100) / 255.;
        color[i][2] = 0;
      } else if (i >= 250) { // level 5: red -> white
        color[i][0] = 1;
        color[i][1] = (255*(i-250)/5) / 255.;
        color[i][2] = (255*(i-250)/5) / 255.;
      }
    }

    gqrxPalette = new Palette("Gqrx", color);
  }

  return gqrxPalette;
}

void
SigDiggerHelpers::deserializePalettes(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  if (this->palettes.size() == 0) {
    this->palettes.push_back(Palette("Suscan", wf_gradient));
    this->palettes.push_back(*this->getGqrxPalette());
  }

  // Fill palette vector
  for (auto i = sus->getFirstPalette();
       i != sus->getLastPalette();
       i++)
    this->palettes.push_back(Palette(*i));
}

void
SigDiggerHelpers::populatePaletteCombo(QComboBox *cb)
{
  int ndx = 0;

  cb->clear();

  // Populate combo
  for (auto p : this->palettes) {
    cb->insertItem(
          ndx,
          QIcon(QPixmap::fromImage(p.getThumbnail())),
          QString::fromStdString(p.getName()),
          QVariant::fromValue(ndx));
    ++ndx;
  }
}

const Palette *
SigDiggerHelpers::getPalette(int index) const
{
  if (index < 0 || index >= static_cast<int>(this->palettes.size()))
    return nullptr;

  return &this->palettes[static_cast<size_t>(index)];
}

int
SigDiggerHelpers::getPaletteIndex(std::string const &name) const
{
  unsigned int i;

  for (i = 0; i < this->palettes.size(); ++i)
    if (this->palettes[i].getName().compare(name) == 0)
      return static_cast<int>(i);

  return -1;
}

const Palette *
SigDiggerHelpers::getPalette(std::string const &name) const
{
  int index = this->getPaletteIndex(name);

  if (index >= 0)
    return &this->palettes[index];

  return nullptr;
}

SigDiggerHelpers::SigDiggerHelpers()
{
  const char *localTZ = getenv("TZ");

  this->haveTZvar = localTZ != nullptr;
  if (localTZ != nullptr)
    this->tzVar = localTZ;

  this->deserializePalettes();
}


void
SigDiggerHelpers::pushTZ(const char *tz)
{
  const std::string *front = nullptr;
  const char *prev = getenv("TZ");

  // Non-null TZ, push in saving stack
  if (prev != nullptr) {
    this->tzs.push_front(prev);
    front = &this->tzs.front();
  }

  // Push this one nonetheless
  this->tzStack.push_front(front);

  if (tz != nullptr)
    setenv("TZ", tz, 1);
  else
    unsetenv("TZ");

  tzset();
}

bool
SigDiggerHelpers::popTZ(void)
{
  const std::string *front;

  if (this->tzStack.empty())
    return false;

  front = this->tzStack.front();

  if (front != nullptr) {
    setenv("TZ", front->c_str(), 1);
    // Non-null TZ, pop from the saving stack
    this->tzs.pop_front();
  } else {
    unsetenv("TZ");
  }

  tzset();

  // Pop it
  this->tzStack.pop_front();

  return true;
}

void
SigDiggerHelpers::pushLocalTZ(void)
{
  if (this->haveTZvar)
    this->pushTZ(this->tzVar.c_str());
  else
    this->pushTZ(nullptr);
}

void
SigDiggerHelpers::pushUTCTZ(void)
{
  this->pushTZ("");
}
