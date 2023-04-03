//
//    GuiConfigTab.h: GuiConfigTab.h
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

#ifndef AUDIOCONFIGTAB_H
#define AUDIOCONFIGTAB_H

#include <ConfigTab.h>
#include <AudioConfig.h>
#include <GenericAudioPlayer.h>

namespace Ui {
  class AudioConfigTab;
}

namespace SigDigger {
  class AudioConfigTab : public ConfigTab
  {
    Q_OBJECT

    AudioConfig m_audioConfig;
    std::vector<GenericAudioDevice> m_devices;
    bool m_modified = false;

    void refreshUi();
    void connectAll();

  public:
    bool hasChanged() const override;
    void save() override;

    void setAudioConfig(AudioConfig const &);
    AudioConfig getAudioConfig() const;

    explicit AudioConfigTab(QWidget *parent = nullptr);
    ~AudioConfigTab();

  public slots:
    void onSettingsChanged();

  private:
    Ui::AudioConfigTab *ui;
  };
}

#endif // AUDIOCONFIGTAB_H
