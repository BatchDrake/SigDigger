#include "FileViewer.h"
#include <QFileDialog>
#include <QMessageBox>
#include <SigDiggerHelpers.h>
#include <TimeWindow.h>
#include <QEventLoop>

using namespace SigDigger;

FileViewer::FileViewer(QObject *parent)
  : QObject{parent}
{
  QStringList formats;

  m_dialog = new QFileDialog(nullptr);

  formats
      << "Raw complex 32-bit float (*.raw *.cf32)"
      << "Raw complex 8-bit unsigned (*.u8 *.cu8)"
      << "Raw complex 8-bit signed (*.s8 *.cs8)"
      << "Raw complex 16-bit signed (*.s16 *.cs16)"
      << "WAV files (*.wav)"
      << "All files (*)";

  m_dialog->setFileMode(QFileDialog::ExistingFile);
  m_dialog->setNameFilters(formats);
  m_dialog->setAcceptMode(QFileDialog::AcceptOpen);
  m_dialog->setWindowTitle("Open capture file");

  connect(
        m_dialog,
        SIGNAL(accepted()),
        this,
        SLOT(onFileOpened()));

  m_dialog->show();
}

void
FileViewer::processFile(QString path)
{
  SigDiggerHelpers *hlp = SigDiggerHelpers::instance();
  CaptureFileParams params;

  if (!hlp->guessCaptureFileParams(params, path)) {
    QMessageBox::warning(
          nullptr,
          "Unrecognized file",
          "The selected file has not been recognized as a well-known capture file. "
          "Arbitrary capture files are not yet supported.");
    return;
  }

  if (!params.isRaw) {
    QMessageBox::warning(
          nullptr,
          "Unsupported file format",
          "The selected file has been recognized, but its storage format is not raw. "
          "FileViewer currently relies on memory-mapped files to display large files, "
          "and non-native sample formats cannot be converted on the go.");
    return;
  }

  if (!params.haveFs) {
    QMessageBox::warning(
          nullptr,
          "Unsupported file format",
          "The selected file has been recognized, but some parameters are undefined. "
          "Capture files need their sample format and rate to be defined prior processing.");
    return;
  }

  QFile file(path);

  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(
          nullptr,
          "Failed to open file",
          "The selected file could not be opened. " + file.errorString());
    return;
  }

  const SUCOMPLEX *data = reinterpret_cast<SUCOMPLEX *>(file.map(
        0,
        file.size(),
        QFileDevice::MemoryMapFlag::MapPrivateOption));

  if (data == nullptr) {
    QMessageBox::critical(
          nullptr,
          "Failed to map file",
          "The selected file could not be mapper. " + file.errorString());
    return;
  }

  TimeWindow *window = new TimeWindow;

  window->postLoadInit();

  window->setData(data, file.size() / sizeof(SUCOMPLEX), params.fs, params.fs);
  window->setCenterFreq(params.fc);

  window->show();
  window->raise();
  window->activateWindow();
  window->setWindowState(Qt::WindowState::WindowActive);
  window->onFit();

  QEventLoop loop;

  connect(window, SIGNAL(closed()), &loop, SLOT(quit()));

  loop.exec();

  window->deleteLater();
}

FileViewer::~FileViewer()
{
  m_dialog->deleteLater();
}

///////////////////////////////// Slots ////////////////////////////////////////
void
FileViewer::onFileOpened()
{
  QStringList files = m_dialog->selectedFiles();

  if (files.size() > 0) {
    processFile(files[0]);
  }
}
