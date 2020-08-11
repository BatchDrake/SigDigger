//
//    SymViewTab.cpp: Inspector's symbol view tab
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

#include <SuWidgets/ThrottleableWidget.h>
#include <SuWidgets/SuWidgetsHelpers.h>
#include <QMessageBox>
#include <QFileDialog>

#include "SymViewTab.h"
#include "ui_SymViewTab.h"

using namespace SigDigger;

SymViewTab::SymViewTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SymViewTab)
{
  ui->setupUi(this);

#ifdef __APPLE__
  // Qt for MacOS X does not now how to handle proper button styling. We
  // just get rid of it for the sake of clarity.
  this->ui->recordButton->setStyleSheet("");
#endif // __APPLE__

  this->connectAll();
}

SymViewTab::~SymViewTab()
{
  delete ui;
}

void
SymViewTab::connectAll(void)
{
  connect(
        this->ui->symView,
        SIGNAL(zoomChanged(unsigned int)),
        this,
        SLOT(onSymViewZoomChanged(unsigned int)));

  connect(
        this->ui->symView,
        SIGNAL(offsetChanged(unsigned int)),
        this,
        SLOT(onOffsetChanged(unsigned int)));

  connect(
        this->ui->symView,
        SIGNAL(hOffsetChanged(int)),
        this,
        SLOT(onHOffsetChanged(int)));

  connect(
        this->ui->symView,
        SIGNAL(strideChanged(unsigned int)),
        this,
        SLOT(onStrideChanged(unsigned int)));

  connect(
        this->ui->symViewScrollBar,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onScrollBarChanged(int)));

  connect(
        this->ui->symViewHScrollBar,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onHScrollBarChanged(int)));
  connect(
        this->ui->recordButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->reverseButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->recordButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->autoScrollButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->autoFitButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->widthSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->offsetSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->zoomSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onZoomChanged()));

  connect(
        this->ui->resetZoomButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onZoomReset()));

  connect(
        this->ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveSymView()));

  connect(
        this->ui->clearButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onClearSymView()));
}

void
SymViewTab::setEnabled(bool enabled)
{
  this->ui->recordButton->setEnabled(enabled);
}

void
SymViewTab::feed(const Symbol *data, unsigned int size)
{
  this->ui->symView->feed(data, size);
  this->refreshSizes();
}

unsigned int
SymViewTab::getVScrollPageSize(void) const
{
  return
      (this->ui->symView->getStride()
       * static_cast<unsigned>(this->ui->symView->height()))
      / this->ui->symView->getZoom();
}

unsigned int
SymViewTab::getHScrollOffset(void) const
{
  return static_cast<unsigned>(this->ui->symViewHScrollBar->value());
}

void
SymViewTab::refreshHScrollBar(void) const
{
  unsigned int visible =
      static_cast<unsigned>(this->ui->symView->width()) /
      this->ui->symView->getZoom();

  if (visible < this->ui->symView->getStride()) {
    unsigned int max = this->ui->symView->getStride() - visible;
    this->ui->symViewHScrollBar->setPageStep(static_cast<int>(visible));
    this->ui->symViewHScrollBar->setMaximum(static_cast<int>(max));
    this->ui->symViewHScrollBar->setVisible(true);
  } else {
    this->ui->symViewHScrollBar->setPageStep(static_cast<int>(0));
    this->ui->symViewHScrollBar->setMaximum(static_cast<int>(0));
    this->ui->symViewHScrollBar->setVisible(false);
  }

  if (!this->ui->autoFitButton->isChecked())
    this->ui->symViewHScrollBar->setEnabled(
          this->ui->symView->getLength() >= visible);
  else
    this->ui->symViewHScrollBar->setEnabled(false);
}

void
SymViewTab::refreshVScrollBar(void) const
{
  unsigned int pageSize = this->getVScrollPageSize();
  unsigned long lines =
      (this->ui->symView->getLength() + this->ui->symView->getStride() - 1) /
      this->ui->symView->getStride();
  unsigned long max = lines * this->ui->symView->getStride();

  if (max > pageSize) {
    this->ui->symViewScrollBar->setPageStep(static_cast<int>(pageSize));
    this->ui->symViewScrollBar->setMaximum(static_cast<int>(max - pageSize));
    this->ui->symViewScrollBar->setVisible(true);
  } else {
    this->ui->symViewScrollBar->setPageStep(0);
    this->ui->symViewScrollBar->setMaximum(0);
    this->ui->symViewScrollBar->setVisible(false);
  }

  this->ui->symViewScrollBar->setSingleStep(
        static_cast<int>(this->ui->symView->getStride()));

  if (!this->ui->autoScrollButton->isChecked())
    this->ui->symViewScrollBar->setEnabled(
          this->ui->symView->getLength() >= pageSize);
  else
    this->ui->symViewScrollBar->setEnabled(false);
}

void
SymViewTab::setBitsPerSymbol(unsigned int bps)
{
  this->bps = bps;
}

void
SymViewTab::refreshSizes(void)
{
  this->ui->sizeLabel->setText(
        "Capture size: " +
        SuWidgetsHelpers::formatQuantity(
          this->ui->symView->getLength(),
          "sym"));

  this->ui->dataSizeLabel->setText(
        "Data size: " +
        SuWidgetsHelpers::formatQuantity(
          this->ui->symView->getLength() * this->bps,
          "bits")
        + " (" +
        SuWidgetsHelpers::formatBinaryQuantity(
          this->ui->symView->getLength() * this->bps >> 3,
          "B") + ")");

  this->ui->saveButton->setEnabled(this->ui->symView->getLength() > 0);

  this->refreshVScrollBar();
}

void
SymViewTab::setThrottleControl(ThrottleControl *control)
{
  this->ui->symView->setThrottleControl(control);
}

/////////////////////////////////// Slots /////////////////////////////////////
void
SymViewTab::onScrollBarChanged(int offset)
{
  int relStart = this->ui->symView->getOffset() % this->ui->symView->getStride();
  int alignedOffset = this->ui->symView->getStride() * (
        offset / this->ui->symView->getStride());

  this->scrolling = true;

  this->ui->symView->setOffset(
        static_cast<unsigned int>(alignedOffset + relStart));

  this->scrolling = false;
}

void
SymViewTab::onHScrollBarChanged(int offset)
{
  this->scrolling = true;

  this->ui->symView->setHOffset(offset);
  this->scrolling = false;
}

void
SymViewTab::onOffsetChanged(unsigned int offset)
{
  if (!this->scrolling)
    this->ui->symViewScrollBar->setValue(static_cast<int>(offset));

  this->ui->offsetSpin->setValue(static_cast<int>(offset));
}

void
SymViewTab::onHOffsetChanged(int offset)
{
  if (!this->scrolling)
    this->ui->symViewHScrollBar->setValue(offset);
}

void
SymViewTab::onStrideChanged(unsigned int stride)
{
  this->ui->widthSpin->setValue(static_cast<int>(stride));
  this->refreshHScrollBar();
}

void
SymViewTab::onSymViewControlsChanged(void)
{
  bool autoStride = this->ui->autoFitButton->isChecked();
  bool autoScroll = this->ui->autoScrollButton->isChecked();

  this->demodulating = this->ui->recordButton->isChecked();

  this->ui->symView->setAutoStride(autoStride);
  this->ui->symView->setAutoScroll(autoScroll);
  this->ui->widthSpin->setEnabled(!autoStride);
  this->ui->offsetSpin->setEnabled(!autoScroll);

  this->refreshVScrollBar();
  this->refreshHScrollBar();

  if (!autoStride)
    this->ui->symView->setStride(
        static_cast<unsigned int>(this->ui->widthSpin->value()));

  if (!autoScroll)
    this->ui->symView->setOffset(
        static_cast<unsigned int>(this->ui->offsetSpin->value()));

  this->ui->symView->setReverse(this->ui->reverseButton->isChecked());
}

void
SymViewTab::onSaveSymView(void)
{
  QFileDialog dialog(this->ui->symView);
  QStringList filters;
  enum SymView::FileFormat fmt = SymView::FILE_FORMAT_TEXT;

  filters << "Text file (*.txt)"
          << "Binary file (*.bin)"
          << "C source file (*.c)"
          << "Microsoft Windows Bitmap (*.bmp)"
          << "PNG Image (*.png)"
          << "JPEG Image (*.jpg)"
          << "Portable Pixel Map (*.ppm)";

  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setWindowTitle(QString("Save current symbol capture as..."));
  dialog.setNameFilters(filters);

  if (dialog.exec()) {
    // This sucks
    QString filter = dialog.selectedNameFilter();
    QString path = dialog.selectedFiles().first();
    QFileInfo fi(path);
    QString ext = fi.suffix().size() > 0
        ? fi.suffix()
        : SuWidgetsHelpers::extractFilterExtension(filter);

    if (ext == "txt")
      fmt = SymView::FILE_FORMAT_TEXT;
    else if (ext == "bin")
      fmt = SymView::FILE_FORMAT_RAW;
    else if (ext == "c" || ext == "h" || ext == "cpp")
      fmt = SymView::FILE_FORMAT_C_ARRAY;
    else if (ext == "bmp")
      fmt = SymView::FILE_FORMAT_BMP;
    else if (ext == "png")
      fmt = SymView::FILE_FORMAT_PNG;
    else if (ext == "jpg" || ext == "jpeg")
      fmt = SymView::FILE_FORMAT_JPEG;
    else if (ext == "ppm")
      fmt = SymView::FILE_FORMAT_PPM;

    try {
      this->ui->symView->save(
            SuWidgetsHelpers::ensureExtension(path, ext),
            fmt);
    } catch (std::ios_base::failure const &) {
      (void) QMessageBox::critical(
            this->ui->symView,
            "Save symbol file",
            "Failed to save file in the specified location. Please verify if "
            "permission and disk space allow this operation.",
            QMessageBox::Close);
    }
  }
}

void
SymViewTab::onClearSymView(void)
{
  this->ui->symView->clear();
  this->onOffsetChanged(0);
  this->refreshVScrollBar();
  this->refreshSizes();
}

void
SymViewTab::onZoomChanged(void)
{
  this->ui->symView->setZoom(
        static_cast<unsigned int>(this->ui->zoomSpin->value()));
  this->refreshVScrollBar();
  this->refreshHScrollBar();
}

void
SymViewTab::onZoomReset(void)
{
  this->ui->zoomSpin->setValue(1);
  this->ui->symView->setZoom(1);
  this->refreshVScrollBar();
  this->refreshHScrollBar();
}

void
SymViewTab::onSymViewZoomChanged(unsigned int zoom)
{
  this->ui->zoomSpin->setValue(static_cast<int>(zoom));
  this->refreshVScrollBar();
  this->refreshHScrollBar();
}
