//
//    FileSourcePage.cpp: description
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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

#include "FileSourcePage.h"
#include "ui_FileSourcePage.h"
#include "SigDiggerHelpers.h"

#include <QFileDialog>
#include <QFileInfo>

using namespace SigDigger;

FileSourcePage::FileSourcePage(
    SourceConfigWidgetFactory *factory,
    QWidget *parent) :
  SourceConfigWidget(factory, parent)
{
  ui = new Ui::FileSourcePage();

  ui->setupUi(this);

  connectAll();
}

FileSourcePage::~FileSourcePage()
{
  delete ui;
}

void
FileSourcePage::connectAll()
{
  connect(
        ui->browseButton,
        SIGNAL(clicked()),
        this,
        SLOT(onBrowseCaptureFile()));

  connect(
        ui->loopCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onLoopChanged()));

  connect(
        ui->formatCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onFormatChanged(int)));
}

void
FileSourcePage::refreshUi()
{
  int index = 0;

  if (m_config == nullptr)
    return;

  BLOCKSIG(ui->pathEdit, setText(QString::fromStdString(m_config->getPath())));
  BLOCKSIG(ui->loopCheck, setChecked(m_config->getLoop()));

  switch (m_config->getFormat()) {
    case SUSCAN_SOURCE_FORMAT_AUTO:
      index = 0;
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_FLOAT32:
      index = 1;
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_UNSIGNED8:
      index = 2;
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_SIGNED8:
      index = 3;
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_SIGNED16:
      index = 4;
      break;

    case SUSCAN_SOURCE_FORMAT_WAV:
      index = 5;
      break;

    case SUSCAN_SOURCE_FORMAT_SIGMF:
      index = 6;
      break;
  }

  BLOCKSIG(ui->formatCombo, setCurrentIndex(index));
}

bool
FileSourcePage::guessParamsFromFileName()
{
  suscan_source_metadata meta;
  bool changes = false;
  bool refresh = false;

  if (m_config == nullptr)
    return false;

  if (m_config->guessMetadata(meta)) {
    auto st = m_config->getStartTime();
    if ((meta.guessed & SUSCAN_SOURCE_CONFIG_GUESS_START_TIME)
        && (st.tv_sec != meta.start_time.tv_sec || st.tv_usec != meta.start_time.tv_usec)) {
      m_config->setStartTime(meta.start_time);
      changes = true;
    }

    if ((meta.guessed & SUSCAN_SOURCE_CONFIG_GUESS_SAMP_RATE)
        && m_config->getSampleRate() != meta.sample_rate) {
      m_config->setSampleRate(meta.sample_rate);
      changes = true;
    }

    if (meta.guessed & SUSCAN_SOURCE_CONFIG_GUESS_FREQ) {
      qreal shiftedFc = meta.frequency + m_config->getLnbFreq();
      if (!sufeq(m_config->getFreq(), shiftedFc, 1)) {
        m_config->setFreq(shiftedFc);
        changes = true;
      }
    }

    if ((meta.guessed & SUSCAN_SOURCE_CONFIG_GUESS_FORMAT)
        && meta.format != m_config->getFormat()) {
      m_config->setFormat(meta.format);
      changes = refresh = true;
    }
  }

  if (refresh)
    refreshUi();

  return changes;
}

uint64_t
FileSourcePage::getCapabilityMask() const
{
  uint64_t perms = SUSCAN_ANALYZER_PERM_ALL;

  perms &= ~SUSCAN_ANALYZER_PERM_SET_BW;
  perms &= ~SUSCAN_ANALYZER_PERM_SET_ANTENNA;
  perms &= ~SUSCAN_ANALYZER_PERM_SET_PPM;
  perms &= ~SUSCAN_ANALYZER_PERM_SET_AGC;

  return perms;
}

void
FileSourcePage::activateWidget()
{
  /* if (guessParamsFromFileName())
    emit changed(); */
}

void
FileSourcePage::setConfigRef(Suscan::Source::Config &cfg)
{
  m_config = &cfg;
  refreshUi();
}

////////////////////////////////// Slots ///////////////////////////////////////
void
FileSourcePage::onBrowseCaptureFile()
{
  QString title;
  QFileInfo fi(this->ui->pathEdit->text());
  QStringList formats;
  QString selected = "All files (*)";

  switch (m_config->getFormat()) {
    case SUSCAN_SOURCE_FORMAT_AUTO:
      title = "Open capture file";
      formats
          << "Raw complex 32-bit float (*.raw *.cf32)"
          << "Raw complex 8-bit unsigned (*.u8 *.cu8)"
          << "Raw complex 8-bit signed (*.s8 *.cs8)"
          << "Raw complex 16-bit signed (*.s16 *.cs16)"
          << "WAV files (*.wav)"
          << "SigMF signal recordings (*.sigmf-data *.sigmf-meta)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_FLOAT32:
      title = "Open I/Q file";
      formats
          << "Raw complex 32-bit float (*.raw *.cf32)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_UNSIGNED8:
      title = "Open I/Q file";
      formats
          << "Raw complex 8-bit unsigned (*.u8 *.cu8)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_SIGNED8:
      title = "Open I/Q file";
      formats
          << "Raw complex 8-bit signed (*.s8 *.cs8)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_SIGNED16:
      title = "Open I/Q file";
      formats
          << "Raw complex 16-bit signed (*.s16 *.cs16)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_SIGMF:
      title = "Open SigMF recording";
      formats
          << "SigMF signal recordings (*.sigmf-data *.sigmf-meta)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_WAV:
      title = "Open WAV file";
      formats
          << "WAV files (*.wav)"
          << "All files (*)";
      break;
  }

  for (auto p : formats)
    if (p.contains("*." + fi.suffix()))
      selected = p;

  QString path = QFileDialog::getOpenFileName(
         this,
         title,
         fi.absolutePath(),
         formats.join(";;"),
         &selected);

  if (!path.isEmpty()) {
    ui->pathEdit->setText(path);
    m_config->setPath(path.toStdString());
    guessParamsFromFileName();
    emit changed();
  }
}
void
FileSourcePage::onLoopChanged()
{
  if (m_config == nullptr)
    return;

  ui->loopCheck->setChecked(m_config->getLoop());
  emit changed();
}

void
FileSourcePage::onFormatChanged(int index)
{
  switch (index) {
    case 0:
      m_config->setFormat(SUSCAN_SOURCE_FORMAT_AUTO);
      break;

    case 1:
      m_config->setFormat(SUSCAN_SOURCE_FORMAT_RAW_FLOAT32);
      break;

    case 2:
      m_config->setFormat(SUSCAN_SOURCE_FORMAT_RAW_UNSIGNED8);
      break;

    case 3:
      m_config->setFormat(SUSCAN_SOURCE_FORMAT_RAW_SIGNED8);
      break;

    case 4:
      m_config->setFormat(SUSCAN_SOURCE_FORMAT_RAW_SIGNED16);
      break;

    case 5:
      m_config->setFormat(SUSCAN_SOURCE_FORMAT_WAV);
      break;

    case 6:
      m_config->setFormat(SUSCAN_SOURCE_FORMAT_SIGMF);
      break;
  }

  emit changed();
}
