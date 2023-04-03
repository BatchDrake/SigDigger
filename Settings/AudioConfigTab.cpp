#include "AudioConfigTab.h"
#include "ui_AudioConfigTab.h"
#include <AudioPlayback.h>

using namespace SigDigger;

void
AudioConfigTab::save()
{

}

void
AudioConfigTab::refreshUi()
{
  int index = -1;
  QString audioLib = AudioPlayback::audioLibrary();

  ui->comboBox->clear();

  if (audioLib == "alsa")
    audioLib = "Advanced Linux Sound Architecture (ALSA)";
  else if (audioLib == "portaudio")
    audioLib = "PortAudio";

  ui->audioLibraryLabel->setText(audioLib);

  if (!AudioPlayback::enumerateDevices(m_devices)) {
    ui->comboBox->addItem("Default device", QString(""));
    ui->comboBox->setEnabled(false);
    return;
  }

  for (GenericAudioDevice const &p : m_devices) {
    if (index == -1)
      if (p.devStr == m_audioConfig.devStr)
        index = ui->comboBox->count();

    ui->comboBox->addItem(
          QString::fromStdString(p.description),
          QVariant::fromValue(QString::fromStdString(p.devStr)));
  }

  if (index == -1)
    index = 0;

  ui->comboBox->setCurrentIndex(index);
  ui->comboBox->setEnabled(true);
}

void
AudioConfigTab::connectAll()
{
  connect(
        ui->comboBox,
        SIGNAL(activated(int)),
        this,
        SLOT(onSettingsChanged()));
}

void
AudioConfigTab::setAudioConfig(const AudioConfig &config)
{
  m_audioConfig = config;
  refreshUi();
  m_modified = false;
  ui->warningLabel->setText("");
}

AudioConfig
AudioConfigTab::getAudioConfig() const
{
  return m_audioConfig;
}

bool
AudioConfigTab::hasChanged() const
{
  return m_modified;
}

AudioConfigTab::AudioConfigTab(QWidget *parent) :
  ConfigTab(parent, "Audio output"),
  ui(new Ui::AudioConfigTab)
{
  ui->setupUi(this);

  m_audioConfig.devStr = AudioPlayback::getDefaultDevice();

  refreshUi();

  connectAll();
}

AudioConfigTab::~AudioConfigTab()
{
  delete ui;
}

///////////////////////////////// Slots ///////////////////////////////////////
void
AudioConfigTab::onSettingsChanged()
{
  m_audioConfig.devStr = ui->comboBox->currentData().value<QString>().toStdString();
  m_audioConfig.description = ui->comboBox->currentText().toStdString();
  m_modified = true;
  ui->warningLabel->setText(
        "Note: in order for these changes to take effect, any currently running "
        "audio playback must be restarted.");
  emit changed();
}
