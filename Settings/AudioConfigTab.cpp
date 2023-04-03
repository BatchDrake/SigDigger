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
  ui->comboBox->clear();

  AudioPlayback::enumerateDevices(m_devices);

  for (GenericAudioDevice const &p : m_devices) {
    ui->comboBox->addItem(
          QString::fromStdString(p.description),
          QVariant::fromValue(QString::fromStdString(p.devStr)));
  }

  ui->comboBox->setCurrentIndex(0);
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

  refreshUi();
}

AudioConfigTab::~AudioConfigTab()
{
  delete ui;
}

///////////////////////////////// Slots ///////////////////////////////////////
void
AudioConfigTab::onSettingsChanged()
{
  m_modified = true;
  emit changed();
}
