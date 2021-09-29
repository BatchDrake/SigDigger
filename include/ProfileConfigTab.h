//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#ifndef PROFILECONFIGTAB_H
#define PROFILECONFIGTAB_H

#include <QWidget>
#include <Suscan/AnalyzerParams.h>
#include <Suscan/Source.h>

#include <QComboBox>
#include <SaveProfileDialog.h>

#define SIGDIGGER_MIN_RADIO_FREQ  -3e11
#define SIGDIGGER_MAX_RADIO_FREQ   3e11

namespace Ui {
  class ProfileConfigTab;
}

namespace SigDigger {
  class ProfileConfigTab : public QWidget
  {
    Q_OBJECT

    Ui::ProfileConfigTab *ui;
    bool modified      = false;
    bool needsRestart  = false;
    bool refreshing    = true;
    Suscan::Source::Config profile;
    Suscan::Source::Device remoteDevice;

    int savedLocalDeviceIndex = 0;

    SaveProfileDialog saveProfileDialog;

    void connectAll(void);
    void populateCombos(void);
    void refreshAntennas(void);
    void refreshSampRates(void);
    void refreshProfileUi(void);
    void refreshFrequencyLimits(void);
    void refreshUi(void);
    void refreshAnalyzerTypeUi(void);
    void refreshUiState(void);
    void refreshTrueSampleRate(void);
    void loadProfile(Suscan::Source::Config &config);
    void guessParamsFromFileName(void);
    void updateRemoteParams(void);
    void configChanged(bool restart = false);

    int  findRemoteProfileIndex(void);
    unsigned int getSelectedSampleRate(void) const;
    void setSelectedSampleRate(unsigned int);

    static QString getSampRateString(qreal rate);
    static QString getBaseName(const QString &string);

  public:
    void save(void);

    void setUnchanged(void);
    bool hasChanged(void) const;
    bool shouldRestart(void) const;

    void setProfile(const Suscan::Source::Config &profile);
    void setAnalyzerParams(const Suscan::AnalyzerParams &params);
    void setGain(std::string const &name, float value);
    void setFrequency(qint64 freq);
    void notifySingletonChanges(void);

    bool remoteSelected(void) const;

    float getGain(std::string const &name) const;
    Suscan::Source::Config getProfile(void) const;
    Suscan::AnalyzerParams getAnalyzerParams(void);

    explicit ProfileConfigTab(QWidget *parent = nullptr);
    ~ProfileConfigTab();

    static void
    populateAntennaCombo(
        Suscan::Source::Config &profile,
        QComboBox *combo)
    {
      int index = 0;
      combo->clear();

      for (auto i = profile.getDevice().getFirstAntenna();
           i != profile.getDevice().getLastAntenna();
           ++i) {
        combo->addItem(QString::fromStdString(*i));

        if (profile.getAntenna() == *i)
          index = static_cast<int>(
                i - profile.getDevice().getFirstAntenna());
      }

      combo->setCurrentIndex(index);
    }

  signals:
    void changed(void);

  public slots:
    void onLoadProfileClicked(void);
    void onToggleSourceType(bool);
    void onDeviceChanged(int);
    void onFormatChanged(int);
    void onAntennaChanged(int);
    void onAnalyzerTypeChanged(int);
    void onCheckButtonsToggled(bool);
    void onSpinsChanged(void);
    void onBandwidthChanged(double);
    void onBrowseCaptureFile(void);
    void onSaveProfile(void);
    void onChangeConnectionType(void);
    void onRemoteParamsChanged(void);
    void onRefreshRemoteDevices(void);
    void onRemoteProfileSelected(void);
  };
}

#endif // PROFILECONFIGTAB_H
