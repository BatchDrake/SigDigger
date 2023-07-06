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

#include <ConfigTab.h>
#include <Suscan/AnalyzerParams.h>
#include <Suscan/Source.h>

#include <QComboBox>
#include <SaveProfileDialog.h>
#include <QHash>

#define SIGDIGGER_MIN_RADIO_FREQ  -3e11
#define SIGDIGGER_MAX_RADIO_FREQ   3e11

namespace Ui {
  class ProfileConfigTab;
}

namespace SigDigger {
  class SourceConfigWidget;

  enum SampleRateCtlHint {
    SAMPLE_RATE_CTL_HINT_LIST,
    SAMPLE_RATE_CTL_HINT_MANUAL
  };

  class ProfileConfigTab : public ConfigTab
  {
    Q_OBJECT

    Ui::ProfileConfigTab  *ui;
    bool                   m_modified      = false;
    bool                   m_needsRestart  = false;
    SampleRateCtlHint      m_rateHint = SAMPLE_RATE_CTL_HINT_LIST;

    QHash<QString, SourceConfigWidget *> m_configWidgets;
    SourceConfigWidget    *m_currentConfigWidget = nullptr;
    int                    m_currentConfigIndex  = -1;

    Suscan::Source::Config m_profile;
    Suscan::Source::Device m_remoteDevice;

    SaveProfileDialog     *m_saveProfileDialog;

    void connectAll();

    void makeConfigWidgets();
    void populateProfileCombo();
    void populateRemoteDeviceCombo();

    void populateCombos();
    void refreshSampRates();
    void refreshFrequencyLimits();
    void refreshUi();
    void refreshAnalyzerTypeUi();
    void refreshUiState();
    void refreshSampRateCtl();
    void sampRateCtlHint(SampleRateCtlHint);
    void refreshTrueSampleRate();
    void loadProfile(Suscan::Source::Config const &config);
    bool tryLeaveCurrentConfigWidget();
    void updateRemoteParams();
    void configChanged(bool restart = false);
    bool selectSourceType(std::string const &);

    unsigned getDecimation() const;
    void     setDecimation(unsigned);

    int  findRemoteProfileIndex();
    unsigned int getSelectedSampleRate() const;
    void setSelectedSampleRate(unsigned int);

    static QString getSampRateString(qreal rate);
    static QString getBaseName(const QString &string);

  public:
    void save() override;

    void setUnchanged();
    bool hasChanged() const override;
    bool shouldRestart() const;

    void setProfile(const Suscan::Source::Config &profile);
    void setAnalyzerParams(const Suscan::AnalyzerParams &params);
    void setGain(std::string const &name, float value);
    void setFrequency(qint64 freq);
    void notifySingletonChanges();

    bool remoteSelected() const;

    float getGain(std::string const &name) const;
    Suscan::Source::Config getProfile() const;
    Suscan::AnalyzerParams getAnalyzerParams();

    explicit ProfileConfigTab(QWidget *parent = nullptr);
    ~ProfileConfigTab();

  public slots:
    void onLoadProfileClicked();
    void onChangeSourceType(int);
    void onSourceConfigWidgetChanged();
    void onAnalyzerTypeChanged(int);
    void onCheckButtonsToggled(bool);
    void onSpinsChanged();
    void onSaveProfile();
    void onChangeConnectionType();
    void onRemoteParamsChanged();
    void onRefreshRemoteDevices();
    void onRemoteProfileSelected();
    void onChangeSourceTimeUTC();
    void onOverrideSampleRate();
  };
}

#endif // PROFILECONFIGTAB_H
