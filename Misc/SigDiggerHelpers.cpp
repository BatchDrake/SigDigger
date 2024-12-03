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
#include "GlobalProperty.h"
#include <QComboBox>
#include <fstream>
#include <QMessageBox>
#include <QFileDialog>
#include <SuWidgetsHelpers.h>
#include <Suscan/MultitaskController.h>
#include <ExportSamplesTask.h>
#include <ExportCSVTask.h>
#include <sigutils/util/compat-stdlib.h>

#ifndef SIGDIGGER_PKGVERSION
#  define SIGDIGGER_PKGVERSION \
  "custom build on " __DATE__ " at " __TIME__ " (" __VERSION__ ")"
#endif /* SUSCAN_BUILD_STRING */

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
#  define sliced(...) mid(__VA_ARGS__)
#endif

using namespace SigDigger;

SigDiggerHelpers *SigDiggerHelpers::m_currInstance = nullptr;

SigDiggerHelpers *
SigDiggerHelpers::instance()
{
  if (m_currInstance == nullptr)
    m_currInstance = new SigDiggerHelpers();

  return m_currInstance;
}

QString
SigDiggerHelpers::version()
{
  return QString(SIGDIGGER_VERSION_STRING);
}

QString
SigDiggerHelpers::pkgversion()
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

bool
SigDiggerHelpers::tokenize(QString const &command, QStringList &out)
{
  QStringList result;
  int len = SCAST(int, command.size());
  bool qot = false, sqot = false;
  qsizetype argLen;

  for (int i = 0; i < len; i++) {
    int start = i;

    if (command[i] == '\"')
      qot = true;
    else if (command[i] == '\'')
      sqot = true;

    if (qot) {
      ++i;
      ++start;

      while (i < len && command[i] != '\"')
        ++i;

      if (i < len)
        qot = false;

      argLen = i - start;
      ++i;
    } else if (sqot) {
      ++i;
      ++start;

      while (i < len && command[i] != '\'')
        ++i;

      if (i < len)
        sqot = false;
      argLen = i - start;
      ++i;
    } else {
      while(i<len && command[i] != ' ')
        i++;
      argLen = i - start;
    }

    result.append(command.sliced(start, argLen));
  }

  if (qot || sqot)
    return false;

  out.clear();
  out.append(result);

  return true;
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
  else if (str == "RAW")
    return AudioDemod::RAW;

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

    case RAW:
      return "RAW";
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
              "Save samples to file",
              task->getLastError());
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

void
SigDiggerHelpers::openSaveCoherentSamplesDialog(
    QWidget *root,
    const SUCOMPLEX *channel1,
    const SUCOMPLEX *channel2,
    size_t length,
    qreal,
    int start,
    int end,
    Suscan::MultitaskController *mt)
{
  bool done = false;

  do {
    QFileDialog dialog(root);
    QStringList filters;
    dialog.setFileMode(QFileDialog::FileMode::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setWindowTitle(QString("Save coherent capture"));

    filters << "Comma separated values (*.csv)";

    dialog.setNameFilters(filters);

    if (dialog.exec()) {
      QString path = dialog.selectedFiles().first();
      ExportCSVTask *task;

      path = SuWidgetsHelpers::ensureExtension(path, "csv");

      const char *names[]     = {"H", "V"};
      const SUCOMPLEX *data[] = {channel1, channel2};

      task = new ExportCSVTask(path, 2,  names, data, length, start, end);

      if (!task->attemptOpen()) {
        QMessageBox::critical(
              root,
              "Save samples to file",
              task->getLastError());
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
SigDiggerHelpers::getGqrxPalette()
{
  static qreal color[256][3];
  if (m_gqrxPalette == nullptr) {
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

    m_gqrxPalette = new Palette("Gqrx", color);
  }

  return m_gqrxPalette;
}

void
SigDiggerHelpers::deserializePalettes()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  if (m_palettes.size() == 0) {
    m_palettes.push_back(Palette("Suscan", wf_gradient));
    m_palettes.push_back(*getGqrxPalette());
  }

  // Fill palette vector
  for (auto i = sus->getFirstPalette();
       i != sus->getLastPalette();
       i++)
    m_palettes.push_back(Palette(*i));
}

void
SigDiggerHelpers::populatePaletteCombo(QComboBox *cb)
{
  int ndx = 0;

  cb->clear();

  // Populate combo
  for (auto p : m_palettes) {
    cb->insertItem(
          ndx,
          QIcon(QPixmap::fromImage(p.getThumbnail())),
          QString::fromStdString(p.getName()),
          QVariant::fromValue(ndx));
    ++ndx;
  }
}

void
SigDiggerHelpers::populateAntennaCombo(
    Suscan::Source::Config &profile,
    QComboBox *combo)
{
  int index = 0;
  int i = 0;
  combo->clear();
  auto spec = profile.getDeviceSpec();
  auto prop = spec.properties();

  if (prop != nullptr) {
    for (auto antenna : prop->antennas()) {
      combo->addItem(QString::fromStdString(antenna));

      if (profile.getAntenna() == antenna)
        index = i;

      ++i;
    }

    combo->setEnabled(combo->count() > 0);
    if (combo->count() > 0)
      BLOCKSIG(combo, setCurrentIndex(index));
  }
}

const Palette *
SigDiggerHelpers::getPalette(int index) const
{
  if (index < 0 || index >= static_cast<int>(m_palettes.size()))
    return nullptr;

  return &m_palettes[static_cast<size_t>(index)];
}

int
SigDiggerHelpers::getPaletteIndex(std::string const &name) const
{
  unsigned int i;

  for (i = 0; i < m_palettes.size(); ++i)
    if (m_palettes[i].getName().compare(name) == 0)
      return static_cast<int>(i);

  return -1;
}

const Palette *
SigDiggerHelpers::getPalette(std::string const &name) const
{
  int index = getPaletteIndex(name);

  if (index >= 0)
    return &m_palettes[index];

  return nullptr;
}

SigDiggerHelpers::SigDiggerHelpers()
{
  const char *localTZ = getenv("TZ");

  m_haveTZvar = localTZ != nullptr;
  if (localTZ != nullptr)
    m_tzVar = localTZ;

  deserializePalettes();
}


void
SigDiggerHelpers::pushTZ(const char *tz)
{
  const std::string *front = nullptr;
  const char *prev = getenv("TZ");

  // Non-null TZ, push in saving stack
  if (prev != nullptr) {
    m_tzs.push_front(prev);
    front = &m_tzs.front();
  }

  // Push this one nonetheless
  m_tzStack.push_front(front);

  if (tz != nullptr)
    setenv("TZ", tz, 1);
  else
    unsetenv("TZ");

  tzset();
}

bool
SigDiggerHelpers::popTZ()
{
  const std::string *front;

  if (m_tzStack.empty())
    return false;

  front = m_tzStack.front();

  if (front != nullptr) {
    setenv("TZ", front->c_str(), 1);
    // Non-null TZ, pop from the saving stack
    m_tzs.pop_front();
  } else {
    unsetenv("TZ");
  }

  tzset();

  // Pop it
  m_tzStack.pop_front();

  return true;
}

void
SigDiggerHelpers::pushLocalTZ()
{
  if (m_haveTZvar)
    pushTZ(m_tzVar.c_str());
  else
    pushTZ(nullptr);
}

void
SigDiggerHelpers::pushUTCTZ()
{
  pushTZ("");
}

QString
SigDiggerHelpers::expandGlobalProperties(QString const &original)
{
  QString result = "";
  qsizetype p = 0, len = original.size();
  qsizetype propStart, propEnd;

  while (p < len) {
    QString propName;
    GlobalProperty *prop;

    propStart = original.indexOf('%', p);
    if (propStart == -1)
      break;
    propEnd = original.indexOf('%', propStart + 1);
    if (propEnd == -1)
      break;

    result += original.sliced(p, propStart - p);

    propName = original.sliced(propStart + 1, propEnd - propStart - 1);

    prop = GlobalProperty::lookupProperty(propName.toLower());
    if (prop != nullptr)
      result += prop->toString();
    else
      result += "<unknown prop " + propName + ">";
    p = propEnd + 1;
  }

  if (p < len)
    result += original.sliced(p);

  return result;
}
