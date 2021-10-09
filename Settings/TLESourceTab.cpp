//
//    TLESourceTab.cpp: TLE source tab
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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
#include "TLESourceTab.h"
#include "ui_TLESourceTab.h"
#include <QMessageBox>

#if HAVE_CURL
#include <TLEDownloaderTask.h>
#endif // HAVE_CURL

using namespace SigDigger;

void
TLESourceTab::save()
{
  this->tleSourceConfig.autoDownloadOnStartup =
      this->ui->autoDownloadCheck->isChecked();
}

void
TLESourceTab::refreshUi()
{
  this->ui->autoDownloadCheck->setChecked(this->tleSourceConfig.autoDownloadOnStartup);
  this->ui->addTLESourceButton->setEnabled(!this->downloading);
  this->ui->removeTLESourceButton->setEnabled(
        this->ui->tleSourceTable->currentRow() >= 0);
  this->ui->downloadButton->setEnabled(
        !this->downloading && this->ui->tleSourceTable->rowCount() > 0);
  this->ui->abortDownloadButton->setEnabled(this->downloading);
}

void
TLESourceTab::setTleSourceConfig(TLESourceConfig const &config)
{
  this->tleSourceConfig = config;
  this->refreshUi();
  this->modified = false;

  if (!this->configApplied) {
    this->configApplied = true;
    if (this->tleSourceConfig.autoDownloadOnStartup)
      this->triggerDownloadTLEs();
  }
}

TLESourceConfig
TLESourceTab::getTleSourceConfig(void) const
{
  return this->tleSourceConfig;
}

bool
TLESourceTab::hasChanged(void) const
{
  return this->modified;
}

void
TLESourceTab::connectAll(void)
{
  connect(
        this->ui->autoDownloadCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged(void)));

  connect(
        this->ui->addTLESourceButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onAddTLESource(void)));

  connect(
        this->ui->removeTLESourceButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRemoveTLESource(void)));

  connect(
        this->ui->downloadButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onDownloadStart(void)));

  connect(
        this->ui->abortDownloadButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onDownloadCancel(void)));

  connect(
        this->ui->tleSourceTable,
        SIGNAL(cellClicked(int, int)),
        this,
        SLOT(onTLESelectionChanged(void)));

  connect(
        this->taskController,
        SIGNAL(cancelling(void)),
        this,
        SLOT(onTaskCancelling(void)));

  connect(
        this->taskController,
        SIGNAL(progress(qreal, QString)),
        this,
        SLOT(onTaskProgress(qreal, QString)));

  connect(
        this->taskController,
        SIGNAL(done(void)),
        this,
        SLOT(onTaskDone(void)));

  connect(
        this->taskController,
        SIGNAL(cancelled(void)),
        this,
        SLOT(onTaskCancelled(void)));

  connect(
        this->taskController,
        SIGNAL(error(QString)),
        this,
        SLOT(onTaskError(QString)));
}

void
TLESourceTab::populateTLESourceTable(void)
{
  auto sus = Suscan::Singleton::get_instance();
  for (auto src : sus->getTLESourceMap()) {
    int index = this->ui->tleSourceTable->rowCount();
    this->ui->tleSourceTable->insertRow(index);
    this->ui->tleSourceTable->setItem(
          index,
          0,
          new QTableWidgetItem(QString::fromStdString(src.name)));

    this->ui->tleSourceTable->setItem(
          index,
          1,
          new QTableWidgetItem(QString::fromStdString(src.url)));
  }
  this->ui->tleSourceTable->resizeColumnsToContents();
}

void
TLESourceTab::triggerDownloadTLEs(void)
{
  auto sus = Suscan::Singleton::get_instance();

  this->currSrc = sus->getFirstTLESource();
  this->endSrc  = sus->getLastTLESource();

  if (this->currSrc != this->endSrc) {
    this->srcNum    = 1;
    this->srcFailed = 0;
    this->srcCount  = static_cast<unsigned>(sus->getTLESourceMap().count());
    this->pushDownloadTask();
    this->refreshDownloadStatus();
    this->ui->downloadProgress->setFormat("Starting update...");
    this->ui->downloadProgress->setValue(0);
    this->ui->downloadProgress->setEnabled(true);
    this->downloading = true;
  } else {
    this->ui->downloadStatusLabel->setText("Ready");
    this->ui->downloadProgress->setFormat("%p%");
    this->ui->downloadProgress->setValue(0);
    this->ui->downloadProgress->setEnabled(false);
    this->downloading = false;
  }

  this->refreshUi();
}

void
TLESourceTab::downloadNext(void)
{
  if (this->currSrc != this->endSrc)
    ++this->currSrc;

  if (this->currSrc != this->endSrc) {
    ++this->srcNum;
    this->pushDownloadTask();
    this->refreshDownloadStatus();
    this->ui->downloadProgress->setFormat("Connecting to next server");
    this->ui->downloadProgress->setValue(0);
    this->downloading = true;
  } else {
    if (this->srcFailed == this->srcCount) {
      this->ui->downloadStatusLabel->setText("All TLE sources failed to download");
    } else if (this->srcFailed > 0) {
      this->ui->downloadStatusLabel->setText(
            QString::number(this->srcNum - this->srcFailed) +
            " of " + QString::number(this->srcCount) +
            " sources updated, " + QString::number(this->srcFailed) + " failed");
    } else {
      this->ui->downloadStatusLabel->setText(
            QString::number(this->srcNum) +
            " of " + QString::number(this->srcCount) +
            " sources updated");
    }

    this->ui->downloadProgress->setFormat("%p%");
    this->ui->downloadProgress->setValue(0);
    this->ui->downloadProgress->setEnabled(false);
    this->downloading = false;
  }

  this->refreshUi();
}

bool
TLESourceTab::pushDownloadTask(void)
{
#ifdef HAVE_CURL
  TLEDownloaderTask *task = new TLEDownloaderTask(
        QString::fromStdString(this->currSrc->url));
  this->taskController->process(
        "Download TLEs (" + QString::fromStdString(this->currSrc->name) + ")",
        task);
  return true;
#else  // HAVE_CURL
  QMessageBox::critical(
        this,
        "Download support was disabled at compile time.",
        "Download TLEs");
  return false;
#endif // HAVE_CURL
}

void
TLESourceTab::refreshDownloadStatus(void)
{
  if (this->srcFailed > 0) {
    this->ui->downloadStatusLabel->setText(
          "Downloading " + QString::number(this->srcNum) +
          " of " + QString::number(this->srcCount) +
          " (" + QString::fromStdString(this->currSrc->name) + "), " +
          QString::number(this->srcFailed) + " failed");
  } else {
    this->ui->downloadStatusLabel->setText(
          "Downloading " + QString::number(this->srcNum) +
          " of " + QString::number(this->srcCount) +
          " (" + QString::fromStdString(this->currSrc->name) + ")");
  }
}
TLESourceTab::TLESourceTab(QWidget *parent) :
  ConfigTab(parent, "TLE Sources"),
  ui(new Ui::TLESourceTab)
{
  ui->setupUi(this);

  this->addDialog = new AddTLESourceDialog(this);
  this->taskController = new Suscan::CancellableController(this);

  this->populateTLESourceTable();
  this->connectAll();
  this->refreshUi();
}

TLESourceTab::~TLESourceTab()
{
  delete ui;
}

////////////////////////////////// Slots ///////////////////////////////////////
void
TLESourceTab::onConfigChanged(void)
{
  this->modified = true;
  emit changed();
}

void
TLESourceTab::onAddTLESource(void)
{
  if (this->addDialog->run()) {
    auto sus = Suscan::Singleton::get_instance();
    Suscan::TLESource src = this->addDialog->getTLESource();
    if (sus->registerTLESource(src)) {
      int index = this->ui->tleSourceTable->rowCount();
      this->ui->tleSourceTable->insertRow(index);
      this->ui->tleSourceTable->setItem(
            index,
            0,
            new QTableWidgetItem(QString::fromStdString(src.name)));

      this->ui->tleSourceTable->setItem(
            index,
            1,
            new QTableWidgetItem(QString::fromStdString(src.url)));

      this->ui->tleSourceTable->resizeColumnsToContents();
      this->ui->tleSourceTable->horizontalHeader()->setStretchLastSection(true);
      this->ui->tleSourceTable->scrollToBottom();
      this->refreshUi();
    } else {
      QMessageBox::warning(
            this,
            "There is another source named " + QString::fromStdString(src.name)
            + ". please choose a different name.",
            "Add TLE source");
    }
  }
}

void
TLESourceTab::onRemoveTLESource(void)
{
  if (this->ui->tleSourceTable->currentRow() >= 0) {
    auto sus = Suscan::Singleton::get_instance();

    QString text = this->ui->tleSourceTable->itemAt(
          0,
          this->ui->tleSourceTable->currentRow())->text();
    if (sus->removeTLESource(text.toStdString())) {
      this->ui->tleSourceTable->removeRow(
            this->ui->tleSourceTable->currentRow());
      this->refreshUi();
    } else {
      QMessageBox::warning(
            this,
            "Source " + text + " is a default source and cannot be removed.",
            "Remove TLE source");
    }
  }
}

void
TLESourceTab::onDownloadStart(void)
{
  this->triggerDownloadTLEs();
}

void
TLESourceTab::onDownloadCancel(void)
{
  this->taskController->cancel();
}


void
TLESourceTab::onTLESelectionChanged(void)
{
  this->refreshUi();
}

void
TLESourceTab::onTaskCancelling(void)
{
  this->ui->downloadProgress->setEnabled(false);
  this->ui->downloadStatusLabel->setText("Cancelling...");
}

void
TLESourceTab::onTaskProgress(qreal progress, QString string)
{
  this->refreshDownloadStatus();
  this->ui->downloadProgress->setFormat(string + " (%p%)");
  this->ui->downloadProgress->setValue(static_cast<int>(progress * 100));
}

void
TLESourceTab::onTaskDone(void)
{
  this->downloadNext();
}

void
TLESourceTab::onTaskCancelled(void)
{
  this->ui->downloadProgress->setEnabled(false);
  this->ui->downloadStatusLabel->setText("Ready");
  this->ui->downloadProgress->setFormat("%p%");
  this->ui->downloadProgress->setValue(0);
  this->ui->downloadProgress->setEnabled(false);
  this->downloading = false;
  this->refreshUi();
}

void
TLESourceTab::onTaskError(QString)
{
  ++this->srcFailed;
  this->downloadNext();
}
